#include <ross.h>

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

/*
 * rng_set_seed
 */

void
rng_set_seed(tw_rng_stream * g, uint32_t s[4])
{
	int	j;

	for(j = 0; j < 4; j++)
		g->Ig[j] = s[j];

	rng_init_generator(g, InitialSeed);
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
rng_init_generator(tw_rng_stream * g, SeedType Where)
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
				g->Lg[j] = MultModM(rng->aw[j], g->Lg[j], rng->m[j]);
				break;
			case LastSeed:
				break;
		}

		g->Cg[j] = g->Lg[j];
	}
}

/*
 * rng_set_initial_seed                                                   
 */
void
tw_rand_initial_seed(tw_rng_stream * g, tw_lpid id)
{
	tw_lpid mask_bit = 1;

	uint32_t Ig_t[4];
	uint32_t avw_t[4];

	int i;
	int j;
	int positions = ((sizeof(tw_lpid)) * 8) - 1;

	//seed for zero
	for(j = 0; j < 4; j++)
		Ig_t[j] = rng->seed[j];

	mask_bit <<= positions;

        g->count = 0;

	do
	{
		if(id & mask_bit)
		{
			for(j = 0; j < 4; j++)
			{
				avw_t[j] = rng->avw[j];

				// exponentiate modulus
				for(i = 0; i < positions; i++)
					avw_t[j] = MultModM(avw_t[j], avw_t[j], rng->m[j]);

				Ig_t[j] = MultModM(avw_t[j], Ig_t[j], rng->m[j]);
			}
		}

		mask_bit >>= 1;
		positions--;
	} while(positions > 0);

	if(id % 2)
	{
		for(j = 0; j < 4; j++)
			Ig_t[j] = MultModM(rng->avw[j], Ig_t[j], rng->m[j]);
	}

	for(j = 0; j < 4; j++)
		g->Ig[j] = Ig_t[j];

	rng_init_generator(g, InitialSeed);
	//rng_write_state(g);
}

void
tw_rand_init_streams(tw_lp * lp, unsigned int nstreams)
{
	int i;

	if(nstreams > g_tw_nRNG_per_lp)
            tw_error(TW_LOC, "LP %lu asked for more RNG streams (%d) than the global maximum (g_tw_nRNG_per_lp:%d)\n", lp->gid, nstreams, g_tw_nRNG_per_lp);

        lp->rng = (tw_rng_stream *) tw_calloc(TW_LOC, "LP RNG Streams", sizeof(*lp->rng), nstreams);

	for(i = 0; i < nstreams; i++) {
            tw_rand_initial_seed(&lp->rng[i], (lp->gid * g_tw_nRNG_per_lp) + i);
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
		for(j = 0; j < 4; j++)
			rng->seed[j] = *g_tw_rng_seed[j];
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
 * rng_gen_val                                                              
 */
double
rng_gen_val(tw_rng_stream * g)
{
  int32_t k, s;
  double u;

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
 */

double
rng_gen_reverse_val(tw_rng_stream * g)
{
  long long *b = rng->b;
  int32_t *m = rng->m;
  int32_t s;
  double u;

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
