#include <ross.h>
#include <stdint.h>
#include <stddef.h>
#include <limits.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

/**
 * @file rand-clcg4.c
 * @brief RNG Implementation module
 *
 * Random number generator, provides all of the features GTW requires
 * by default.  Chris hacked this pretty well, he would know the
 * features better.
 *
 */

/*********************************************************************
 *
 * clcg4 private data/routines
 *
 ********************************************************************/

// H = 2^15 : use in MultModM.           
#define H   32768

// One RNG per PE
static tw_rng	*rng = NULL;
// Core RNG (For tiebreaker etc.)
static tw_rng *core_rng = NULL;

/* Read-only, demand-paged quantum bit pool. Model RNG streams only. */
static const unsigned char *quantum_data = NULL;
static size_t quantum_file_size = 0;
static uint64_t quantum_values_per_stream = 0;
static uint64_t quantum_total_streams = 0;
static uint64_t *quantum_block_for_stream = NULL;
extern unsigned int ross_quantum_rng_debug;

void
ross_quantum_rng_close(void)
{
    if (quantum_data != NULL) {
        munmap((void *)quantum_data, quantum_file_size);
        quantum_data = NULL;
    }
    free(quantum_block_for_stream);
    quantum_block_for_stream = NULL;
    quantum_file_size = 0;
    quantum_total_streams = 0;
    quantum_values_per_stream = 0;
}

/* Decode one MSB-first packed 53-bit value without allocating a buffer. */
static double
quantum_read_value(uint64_t value_index)
{
    uint64_t bit_index = value_index * UINT64_C(53);
    uint64_t first_byte = bit_index / 8;
    unsigned offset = (unsigned)(bit_index % 8);
    uint64_t word = 0;
    /* At most eight bytes cover any 53 consecutive bits. */
    for (unsigned i = 0; i < 8; ++i) {
        uint64_t byte_index = first_byte + i;
        word = (word << 8) |
               (byte_index < quantum_file_size ? quantum_data[byte_index] : 0u);
    }
    uint64_t bits = (word >> (11u - offset)) &
                    UINT64_C(0x1fffffffffffff);
    return (double)bits * (1.0 / 9007199254740992.0);
}

/* Called collectively after global LP count and model stream count exist. */
void
ross_quantum_rng_open(const char *filename)
{
    if (filename == NULL || filename[0] == '\0')
        return;
    if (quantum_data != NULL)
        tw_error(TW_LOC, "Quantum RNG initialized twice");
    if (g_tw_nRNG_per_lp == 0 || g_tw_total_lps == 0 ||
        (uint64_t)g_tw_total_lps > UINT64_MAX / g_tw_nRNG_per_lp)
        tw_error(TW_LOC, "Invalid global model RNG stream count");

    quantum_total_streams = (uint64_t)g_tw_total_lps * g_tw_nRNG_per_lp;
    int fd = open(filename, O_RDONLY);
    if (fd < 0)
        tw_error(TW_LOC, "Unable to open quantum pool: %s", filename);
    struct stat st;
    if (fstat(fd, &st) != 0 || st.st_size <= 0 ||
        (uint64_t)st.st_size > SIZE_MAX)
        tw_error(TW_LOC, "Invalid quantum pool size: %s", filename);
    quantum_file_size = (size_t)st.st_size;
    if ((uint64_t)quantum_file_size > UINT64_MAX / 8)
        tw_error(TW_LOC, "Quantum pool too large for bit addressing");
    uint64_t total_values = ((uint64_t)quantum_file_size * 8) / 53;
    quantum_values_per_stream = total_values / quantum_total_streams;
    if (ross_quantum_rng_debug && g_tw_mynode == g_tw_masternode) {
        fprintf(stderr,
                "[quantum-rng] pool_bytes=%zu usable_values=%llu "
                "global_streams=%llu values_per_block=%llu "
                "unused_values=%llu\n",
                quantum_file_size,
                (unsigned long long)total_values,
                (unsigned long long)quantum_total_streams,
                (unsigned long long)quantum_values_per_stream,
                (unsigned long long)(total_values % quantum_total_streams));
    }
    if (quantum_values_per_stream == 0)
        tw_error(TW_LOC, "Quantum pool has fewer 53-bit values than model RNG streams");

    void *mapped = mmap(NULL, quantum_file_size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);
    if (mapped == MAP_FAILED)
        tw_error(TW_LOC, "mmap failed for quantum pool: %s", filename);
    quantum_data = (const unsigned char *)mapped;

    if (quantum_total_streams > SIZE_MAX / sizeof(uint64_t))
        tw_error(TW_LOC, "Quantum block permutation too large");
    quantum_block_for_stream = (uint64_t *)malloc(
            (size_t)quantum_total_streams * sizeof(uint64_t));
    if (!quantum_block_for_stream)
        tw_error(TW_LOC, "Cannot allocate quantum block permutation");
    for (uint64_t i = 0; i < quantum_total_streams; ++i)
        quantum_block_for_stream[i] = i;

    /* Every rank derives the same shuffle using model CLCG4 seed 0. */
    tw_rng_stream selector = {0};
    tw_rand_initial_seed(&selector, 0, rng);
    for (uint64_t remaining = quantum_total_streams; remaining > 1; --remaining) {
        /* Avoid float-index rounding; use a 53-bit integer with rejection. */
        const uint64_t range = UINT64_C(9007199254740992);
        if (remaining > range)
            tw_error(TW_LOC, "More model streams than CLCG4 selector range");
        uint64_t cutoff = range - range % remaining;
        uint64_t sample;
        do {
            sample = (uint64_t)(rng_gen_val(&selector) * (double)range);
        } while (sample >= cutoff);
        uint64_t j = sample % remaining;
        uint64_t tmp = quantum_block_for_stream[remaining - 1];
        quantum_block_for_stream[remaining - 1] = quantum_block_for_stream[j];
        quantum_block_for_stream[j] = tmp;
    }
}

static void
quantum_assign_stream(tw_rng_stream *stream, uint64_t global_id)
{
    if (global_id >= quantum_total_streams)
        tw_error(TW_LOC, "Quantum stream ID exceeds global stream count");
    uint64_t block = quantum_block_for_stream[global_id];
    stream->quantum_start = block * quantum_values_per_stream;
    stream->quantum_cursor = 0;
    stream->quantum_limit = quantum_values_per_stream;
    stream->quantum_enabled = 1;
}

// default RNG seed
int32_t seed[4] = { 11111111, 22222222, 33333333, 44444444 };

/**
 * FindB 
 * @brief Find B to run CLCG4 backwards
 * 
 * B is \f$ a_[i]^{m_[i] - 2} \mathrm{mod m_[i]} \f$                   
 * which is used in running the CLCG4 backwards                        
 * Added by Chris Carothers, 5/15/98                          
 */
long long
FindB(long long a, long long k, long long m)
{
  long long sqrs[32];
  long long power_of_2;
  long long b;

  int i;

  sqrs[0] = a;
  for(i = 1; i < 32; i++)
    {
      sqrs[i] =(sqrs[i - 1] * sqrs[i - 1]) % m;
    }

  power_of_2 = 1;
  b = 1;
  for(i = 0; i < 32; i++)
    {
      if(!(power_of_2 & k))
	{
	  sqrs[i] = 1;

	}
      b =(b * sqrs[i]) % m;
      power_of_2 = power_of_2 * 2;
    }

  return(b);
}

/**
 * MultiModM 
 * @brief Returns(s*t) MOD M                                                        
 * 
 * See L'Ecuyer and Cote(1991).                                    
 *
 * Returns(s*t) MOD M.  Assumes that -M < s < M and -M < t < M.    
 */

int32_t
MultModM(int32_t s, int32_t t, int32_t M)
{
  int32_t R, S0, S1, q, qh, rh, k;

  if(s < 0)
    s += M;
  if(t < 0)
    t += M;
  if(s < H)
    {
      S0 = s;
      R = 0;
    }
  else
    {
      S1 = s / H;
      S0 = s - H * S1;
      qh = M / H;
      rh = M - H * qh;

      if(S1 >= H)
	{
	  S1 -= H;
	  k = t / qh;
	  R = H *(t - k * qh) - k * rh;
	  while(R < 0)
	    R += M;
	}
      else
	R = 0;

      if(S1 != 0)
	{
	  q = M / S1;
	  k = t / q;
	  R -= k *(M - S1 * q);
	  if(R > 0)
	    R -= M;
	  R += S1 *(t - k * q);
	  while(R < 0)
	    R += M;
	}
      k = R / qh;
      R = H *(R - k * qh) - k * rh;
      while(R < 0)
	R += M;
    }
  if(S0 != 0)
    {
      q = M / S0;
      k = t / q;
      R -= k *(M - S0 * q);
      if(R > 0)
	R -= M;
      R += S0 *(t - k * q);
      while(R < 0)
	R += M;
    }
  return R;
}

/********************************************************************
 *
 * clcg4 public interface
 *
 *******************************************************************/

#define S0_MAX 2147483646
#define S1_MAX 2147483542
#define S2_MAX 2147483422
#define S3_MAX 2147483322

/**
 * These seeds MUST adhere to these requirements.
 * This is explicitly stated in L'Ecuyer and Andres (1997)
 */
void clamp_seed(uint32_t s[4])
{
    if (s[0] < 1)
        s[0] = 1;
    if (s[0] > S0_MAX)
        s[0] = S0_MAX;

    if (s[1] < 1)
        s[1] = 1;
    if (s[1] > S1_MAX)
        s[1] = S1_MAX;

    if (s[2] < 1)
        s[2] = 1;
    if (s[2] > S2_MAX)
        s[2] = S2_MAX;

    if (s[3] < 1)
        s[3] = 1;
    if (s[3] > S3_MAX)
        s[3] = S3_MAX;
}

/*
 * rng_set_seed
 */

void
rng_set_seed(tw_rng_stream * g, uint32_t s[4], tw_rng * the_rng)
{
	int	j;

    clamp_seed(s);

	for(j = 0; j < 4; j++)
		g->Ig[j] = s[j];

	rng_init_generator(g, InitialSeed, the_rng);
}

/*
 * rng_write_state                                                       
 */

void
rng_write_state(tw_rng_stream * g, FILE *f)
{
	int	j;

	for(j = 0; j < 4; j++)
	  fprintf( f, "%u ", g->Cg[j]);
	fprintf( f, "\n");
}

/*
 * rng_get_state                                                         
 */

void
rng_get_state(tw_rng_stream * g, uint32_t s[4])
{
	int	j;

	for(j = 0; j < 4; j++)
		s[j] = g->Cg[j];
}


/*
 * rng_get_state                                                         
 */

void
rng_put_state(tw_rng_stream * g, uint32_t s[4])
{
	int	j;

	for(j = 0; j < 4; j++)
		g->Cg[j] = s[j];
}

/*
 * rng_init_generator                                                    
 */

void
rng_init_generator(tw_rng_stream * g, SeedType Where, tw_rng * the_rng)
{
	int	j;

	for(j = 0; j < 4; j++)
	{
		switch(Where)
		{
			case InitialSeed:
				g->Lg[j] = g->Ig[j];
				break;
			case NewSeed:
				g->Lg[j] = MultModM(the_rng->aw[j], g->Lg[j], the_rng->m[j]);
				break;
			case LastSeed:
				break;
		}

		g->Cg[j] = g->Lg[j];
	}
}

/*
 * tw_rand_initial_seed
 * Takes in pointer to main RNG with which to seed the supplied tw_rng_stream *g
 */
void
tw_rand_initial_seed(tw_rng_stream * g, tw_lpid id, tw_rng * the_rng)
{
  //If no specific PE rng is specified, use the default globally defiend PE rng
  if (the_rng == NULL)
    the_rng = rng;

	tw_lpid mask_bit = 1;

	uint32_t Ig_t[4];
	uint32_t avw_t[4];

	int i;
	int j;
	int positions = ((sizeof(tw_lpid)) * 8) - 1;

	//seed for zero
	for(j = 0; j < 4; j++)
		Ig_t[j] = the_rng->seed[j];

	mask_bit <<= positions;

        g->count = 0;

	do
	{
		if(id & mask_bit)
		{
			for(j = 0; j < 4; j++)
			{
				avw_t[j] = the_rng->avw[j];

				// exponentiate modulus
				for(i = 0; i < positions; i++)
					avw_t[j] = MultModM(avw_t[j], avw_t[j], the_rng->m[j]);

				Ig_t[j] = MultModM(avw_t[j], Ig_t[j], the_rng->m[j]);
			}
		}

		mask_bit >>= 1;
		positions--;
	} while(positions > 0);

	if(id % 2)
	{
		for(j = 0; j < 4; j++)
			Ig_t[j] = MultModM(the_rng->avw[j], Ig_t[j], the_rng->m[j]);
	}

    clamp_seed(Ig_t);

	for(j = 0; j < 4; j++)
		g->Ig[j] = Ig_t[j];

	rng_init_generator(g, InitialSeed, the_rng);
	//rng_write_state(g);
}



void
tw_rand_init_streams(tw_lp * lp, unsigned int nstreams, unsigned int n_core_streams)
{
  unsigned int i;
  unsigned int j;

  if(nstreams > g_tw_nRNG_per_lp)
            tw_error(TW_LOC, "LP %lu asked for more RNG streams (%d) than the global maximum (g_tw_nRNG_per_lp:%d)\n", lp->gid, nstreams, g_tw_nRNG_per_lp);

  if(n_core_streams > g_tw_nRNG_core_per_lp)
            tw_error(TW_LOC, "LP %lu asked for more RNG streams (%d) than the global maximum for core (g_tw_nRNG_core_per_lp:%d)\n", lp->gid, n_core_streams, g_tw_nRNG_core_per_lp);

  lp->rng = (tw_rng_stream *) tw_calloc(TW_LOC, "LP RNG Streams", sizeof(*lp->rng), nstreams);
  lp->core_rng = (tw_rng_stream *) tw_calloc(TW_LOC, "LP Core RNG Streams", sizeof(*lp->core_rng), n_core_streams);

  unsigned int total_num_noncore_rngs = g_tw_nRNG_per_lp * g_tw_total_lps;

  for(i = 0; i < nstreams; i++) {
            tw_rand_initial_seed(&lp->rng[i], (lp->gid * g_tw_nRNG_per_lp) + i, rng);
            if (quantum_data != NULL) {
                uint64_t global_stream_id =
                    (uint64_t)lp->gid * g_tw_nRNG_per_lp + i;
                quantum_assign_stream(&lp->rng[i], global_stream_id);
                if (ross_quantum_rng_debug) {
                    uint64_t block = quantum_block_for_stream[global_stream_id];
                    uint64_t start = lp->rng[i].quantum_start;
                    uint64_t limit = lp->rng[i].quantum_limit;
                    fprintf(stderr,
                            "[quantum-rng] rank=%llu lp=%llu stream=%u "
                            "global_stream=%llu block=%llu "
                            "start_value=%llu end_value_exclusive=%llu "
                            "block_values=%llu\n",
                            (unsigned long long)g_tw_mynode,
                            (unsigned long long)lp->gid,
                            i,
                            (unsigned long long)global_stream_id,
                            (unsigned long long)block,
                            (unsigned long long)start,
                            (unsigned long long)(start + limit),
                            (unsigned long long)limit);
                }
            }
  }

  //ROSS core rng streams should be seeded "after" lp rng streams so adjusting number of core streams doesn't change seeds of the lp rng streams
  for(j = 0; j < n_core_streams; j++) {
            tw_rand_initial_seed(&lp->core_rng[j], total_num_noncore_rngs + (lp->gid * (g_tw_nRNG_core_per_lp)) + j, core_rng);
  }
}

/*
 * rng_init                                                             
 */
tw_rng	*
rng_init(int v, int w)
{
	int	 i;
	int	 j;

	rng = (tw_rng *) tw_calloc(TW_LOC, "RNG", sizeof(*rng), 1);

	rng->m[0] = 2147483647;
	rng->m[1] = 2147483543;
	rng->m[2] = 2147483423;
	rng->m[3] = 2147483323;

	rng->a[0] = 45991;
	rng->a[1] = 207707;
	rng->a[2] = 138556;
	rng->a[3] = 49689;
	
  if(g_tw_rng_seed)
  {
    if (!g_tw_mynode)
        printf("Clamping Provided LP Seeds\n");
    clamp_seed((uint32_t*)g_tw_rng_seed);
    for(j = 0; j < 4; j++)
      rng->seed[j] = g_tw_rng_seed[j];
  } else
  {
    rng->seed[0] = 11111111;
    rng->seed[1] = 22222222;
    rng->seed[2] = 33333333;
    rng->seed[3] = 44444444;
  }

	for(j = 0; j < 4; j++)
		rng->aw[j] = rng->a[j];

	for(j = 0; j < 4; j++)
	{
		for(i = 1; i <= w; i++)
			rng->aw[j] = MultModM(rng->aw[j], rng->aw[j], rng->m[j]);

		rng->avw[j] = rng->aw[j];

		for(i = 1; i <= v; i++)
			rng->avw[j] = MultModM(rng->avw[j], rng->avw[j], rng->m[j]);
	}

	for(j = 0; j < 4; j++)
		rng->b[j] = FindB(rng->a[j],(rng->m[j] - 2), rng->m[j]);

	return rng;
}

/*
 * rng_core_init
 * Sets up the main ROSS Core RNG
 */
tw_rng	*
rng_core_init(int v, int w)
{
  int i;
  int j;

  //Do the same for the ROSS Core RNG (Ross Engine Hidden RNGs) as was done for the regular main RNG
  core_rng = (tw_rng *) tw_calloc(TW_LOC, "CORE_RNG", sizeof(*rng), 1);

  //For RC to work properly, these should all have the same values as the main RNG
  //If this changes, it will break reverse computation of the RNG. See rng_gen_reverse_val()
  memcpy(core_rng, rng, sizeof(tw_rng));

  //But the seeds should be different
  if(g_tw_core_rng_seed)
  {
    if(!g_tw_mynode)
      printf("Clamping Provided Core Seeds\n");
    clamp_seed((uint32_t*)g_tw_core_rng_seed);
    for(j = 0; j < 4; j++)
      core_rng->seed[j] = g_tw_core_rng_seed[j];
  } else
  {
    core_rng->seed[0] = 55555555;
    core_rng->seed[1] = 66666666;
    core_rng->seed[2] = 77777777;
    core_rng->seed[3] = 88888888;
  }

  return core_rng;
}

/*
 * rng_gen_val                                                              
 */
double
rng_gen_val(tw_rng_stream * g)
{
  int32_t k, s;
  double u;

  if (g->quantum_enabled) {
      if (g->quantum_cursor >= g->quantum_limit)
          tw_error(TW_LOC, "Quantum RNG stream exhausted at %llu draws",
                   (unsigned long long)g->quantum_limit);
      double result = quantum_read_value(g->quantum_start + g->quantum_cursor);
      ++g->quantum_cursor;
      ++g->count;
      return result;
  }

  g->count++;

  u = 0.0;

  s = g->Cg[0];
  k = s / 46693;
  s = 45991 *(s - k * 46693) - k * 25884;
  if(s < 0)
    s = s + 2147483647;
  g->Cg[0] = s;
  u = u + 4.65661287524579692e-10 * s;

  s = g->Cg[1];
  k = s / 10339;
  s = 207707 *(s - k * 10339) - k * 870;
  if(s < 0)
    s = s + 2147483543;
  g->Cg[1] = s;
  u = u - 4.65661310075985993e-10 * s;
  if(u < 0)
    u = u + 1.0;

  s = g->Cg[2];
  k = s / 15499;
  s = 138556 *(s - k * 15499) - k * 3979;
  if(s < 0.0)
    s = s + 2147483423;
  g->Cg[2] = s;
  u = u + 4.65661336096842131e-10 * s;
  if(u >= 1.0)
    u = u - 1.0;

  s = g->Cg[3];
  k = s / 43218;
  s = 49689 *(s - k * 43218) - k * 24121;
  if(s < 0)
    s = s + 2147483323;
  g->Cg[3] = s;
  u = u - 4.65661357780891134e-10 * s;
  if(u < 0)
    u = u + 1.0;

  return u;
}

/*
 * rng_gen_reverse_val
 *
 * computes the reverse sequence, however does not 
 * return the uniform value computed as a performance 
 * optimization -- Chris Carothers 5/15/98            
 *
 * Because the ROSS Core RNGs have the exact same b & m
 * values as the main RNG, this is safe for use on the
 * core RNG streams. Will be an issue if this is ever
 * not the case.
 */

double
rng_gen_reverse_val(tw_rng_stream * g)
{
  long long *b = rng->b;
  int32_t *m = rng->m;
  int32_t s;
  double u;

  if (g->quantum_enabled) {
      if (g->quantum_cursor == 0)
          tw_error(TW_LOC, "Quantum RNG reverse before start of stream");
      --g->quantum_cursor;
      --g->count;
      return quantum_read_value(g->quantum_start + g->quantum_cursor);
  }

  g->count--;

  u = 0.0;

  if(b[0] == 0)
    tw_error(TW_LOC, "b values not calculated \n");

  s = g->Cg[0];
  s =(b[0] * s) % m[0];
  g->Cg[0] = s;
  u = u + 4.65661287524579692e-10 * s;

  s = g->Cg[1];
  s =(b[1] * s) % m[1];
  g->Cg[1] = s;
  u = u - 4.65661310075985993e-10 * s;
  if(u < 0)
    u = u + 1.0;

  s = g->Cg[2];
  s =(b[2] * s) % m[2];
  g->Cg[2] = s;
  u = u + 4.65661336096842131e-10 * s;
  if(u >= 1.0)
    u = u - 1.0;

  s = g->Cg[3];
  s =(b[3] * s) % m[3];
  g->Cg[3] = s;
  u = u - 4.65661357780891134e-10 * s;
  if(u < 0)
    u = u + 1.0;

  return u;
}
