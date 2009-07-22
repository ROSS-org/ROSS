/*********************************************************************
 *
 * clcg4.c   Implementation module
 *
 * Random number generator, provides all of the features GTW requires
 * by default.  Chris hacked this pretty well, he would know the
 * features better.
 *
 ********************************************************************/

#include <ross.h>

/*********************************************************************
 *
 * clcg4 private data/routines
 *
 ********************************************************************/

/*
 * H = 2^15 : use in MultModM.           
 */
#define H   32768

/*
 * a[j]^{2^w} et a[j]^{2^{v+w}}.     
 */
static long aw[4], avw[4], a[4] = { 45991, 207707, 138556, 49689 }, m[4] =
{ 2147483647, 2147483543, 2147483423, 2147483323};

/*
 * equals a[i]^{m[i]-2} mod m[i] 
 */
static long long b[4] = { 0, 0, 0, 0 };
static long seed[4] = { 11111111, 22222222, 33333333, 44444444 };
 
/*
 * static long Ig[4][Maxgen+1], Lg[4][Maxgen+1], Cg[4][Maxgen+1]; 
 */

/*
 * variables for normal distribution
 */

/*extern volatile double *g_tw_normal_u1;
extern volatile double *g_tw_normal_u2;
extern volatile int *g_tw_normal_flipflop;*/


/*
 * Initial seed, previous seed, and current seed. 
 */
static short i, j;
static int Maxgen = 0;



/*
 * FindB -- Find B which is a[i]^{m[i] - 2} mod m[i]                   
 *
 * Used in running the CLCG4 backwards                        
 * Added by Chris Carothers, 5/15/98                          
 */

static long long
FindB(long long a, long long k, long long m)
{
  int i;
  long long sqrs[32];
  long long power_of_2;
  long long b;

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

/*
 * MultiModM                                                           
 * See L'Ecuyer and Cote(1991).                                    
 *
 * Returns(s*t) MOD M.  Assumes that -M < s < M and -M < t < M.    
 */

static inline long
MultModM(long s, long t, long M)
{
  long R, S0, S1, q, qh, rh, k;

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
rng_set_seed(tw_generator * g, long s[4])
{
  for(j = 0; j < 4; j++)
    g->Ig[j] = s[j];
  rng_init_generator(g, InitialSeed);
}

/*
 * rng_write_state                                                       
 */

void
rng_write_state(tw_generator * g)
{
  for(j = 0; j < 4; j++)
    printf("%lu ", g->Cg[j]);
  printf("\n");
}

/*
 * rng_get_state                                                         
 */

void
rng_get_state(tw_generator * g, long s[4])
{
  for(j = 0; j < 4; j++)
    s[j] = g->Cg[j];
}


/*
 * rng_get_state                                                         
 */

void
rng_put_state(tw_generator * g, long s[4])
{
  for(j = 0; j < 4; j++)
    g->Cg[j] = s[j];
}

/*
 * rng_init_generator                                                    
 */

void
rng_init_generator(tw_generator * g, SeedType Where)
{
  for(j = 0; j < 4; j++)
    {
      switch(Where)
	{
	case InitialSeed:
	  g->Lg[j] = g->Ig[j];
	  break;
	case NewSeed:
	  g->Lg[j] = MultModM(aw[j], g->Lg[j], m[j]);
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
tw_rand_initial_seed(tw_generator * g, tw_lpid id)
{
	tw_lpid mask_bit = 1;

	long Ig_t[4];
	long avw_t[4];

	int positions = ((sizeof(tw_lpid)) * 8) - 1;
	//int positions = ((sizeof(uintptr_t)) * 8) - 1;

	//seed for zero
	for(j = 0; j < 4; j++)
		Ig_t[j] = seed[j];

	mask_bit <<= positions;

	do
	{
		if(id & mask_bit)
		{
			for(j = 0; j < 4; j++)
			{
				avw_t[j] = avw[j];

				// exponentiate modulus
				for(i = 0; i < positions; i++)
					avw_t[j] = MultModM(avw_t[j], avw_t[j], m[j]);

				Ig_t[j] = MultModM(avw_t[j], Ig_t[j], m[j]);
			}
		}

		mask_bit >>= 1;
		positions--;
	} while(positions > 0);

	if(id % 2)
	{
		for(j = 0; j < 4; j++)
			Ig_t[j] = MultModM(avw[j], Ig_t[j], m[j]);
	}

	for(j = 0; j < 4; j++)
		g->Ig[j] = Ig_t[j];

	rng_init_generator(g, InitialSeed);
	//rng_write_state(g);
}

/*
 * rng_init                                                             
 */
void
rng_init(int v, int w, long *sd, tw_lpid nrng)
{
	if(nrng)
		Maxgen = nrng;
	else
		Maxgen = g_tw_nlp * g_tw_nRNG_per_lp;

	if(NULL != sd)
	{
		for(j = 0; j < 4; j++)
			seed[j] = sd[j];
	}

	for(j = 0; j < 4; j++)
		aw[j] = a[j];

	for(j = 0; j < 4; j++)
	{
		for(i = 1; i <= w; i++)
			aw[j] = MultModM(aw[j], aw[j], m[j]);

		avw[j] = aw[j];

		for(i = 1; i <= v; i++)
			avw[j] = MultModM(avw[j], avw[j], m[j]);
	}

	for(j = 0; j < 4; j++)
		b[j] = FindB(a[j],(m[j] - 2), m[j]);
}

/*
 * rng_gen_val                                                              
 */
double
rng_gen_val(tw_generator * g)
{
  long k, s;
  double u;

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
rng_gen_reverse_val(tw_generator * g)
{
  long long s;
  double u;

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
