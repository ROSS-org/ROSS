#include <ross.h>

/*
 * tw_rand_init
 */
tw_rng	*
tw_rand_init(uint32_t v, uint32_t w)
{
	return rng_init(v, w);
}

/*
 * tw_rand_integer
 *
 * For LP # gen, return a uniform rn from low to high 
 */
/**
 * NOTE: Don't pass negative values to low!
 */
long 
tw_rand_integer(tw_rng_stream * g, long low, long high)
{
	long safe_high = high;

	if (safe_high != LONG_MAX) {
		safe_high += 1;
	}

	if (safe_high <= low) {
		return (0);
	} else {
		return (low + (long)(tw_rand_unif(g) * (safe_high - low)));
	}
}

unsigned long
tw_rand_ulong(tw_rng_stream * g, unsigned long low, unsigned long high)
{
	unsigned long safe_high = high;

	if (safe_high != ULONG_MAX) {
		safe_high += 1;
	}

    if (safe_high < low) {
        return (0);
    } else {
        return (low + (unsigned long)(tw_rand_unif(g) * (safe_high - low)));
    }
}

long 
tw_rand_binomial(tw_rng_stream * g, long N, double P)
{
	long            sucesses, trials;

	sucesses = 0;

	for (trials = 0; trials < N; trials++)
	{
		if (tw_rand_unif(g) <= P)
			sucesses++;
	}

	return (sucesses);
}

double 
tw_rand_exponential(tw_rng_stream * g, double Lambda)
{
	return (-Lambda * log(tw_rand_unif(g)));
}

double 
tw_rand_pareto(tw_rng_stream * g, double shape, double scale)
{
  return( scale * 1.0/pow(tw_rand_unif(g), 1/shape) );
}

double 
tw_rand_gamma(tw_rng_stream * g, double shape, double scale)
{
	double          a, b, q, phi, d;

	if (shape > 1)
	{
		a = 1 / sqrt(2 * shape - 1);
		b = shape - log(4);
		q = shape + 1 / a;
		phi = 4.5;
		d = 1 + log(phi);

		while (1)
		{
			double          U_One = tw_rand_unif(g);
			double          U_Two = tw_rand_unif(g);
			double          V = a * log(U_One / (1 - U_One));
			double          Y = shape * exp(V);
			double          Z = U_One * U_One * U_Two;
			double          W = b + q * V - Y;

			double          temp1 = W + d - phi * Z;
			double          temp2 = log(Z);

			if (temp1 >= 0 || W >= temp2)
				return (scale * Y);

		}
	} else if (shape == 1)
	{
		return (tw_rand_exponential(g, scale));
	} else
	{
		b = (exp(1) + shape) / exp(1);

		while (1)
		{
			double          U_One = tw_rand_unif(g);
			double          P = b * U_One;

			if (P <= 1)
			{
				double          Y = pow(P, (1 / shape));
				double          U_Two = tw_rand_unif(g);

				if (U_Two <= exp(-Y))
					return (scale * Y);
			} else
			{
				double          Y = -log((b - P) / shape);
				double          U_Two = tw_rand_unif(g);

				if (U_Two <= pow(Y, (shape - 1)))
					return (scale * Y);
			}
		}
	}
}

long 
tw_rand_geometric(tw_rng_stream * g, double P)
{
	int             count = 1;

	while (tw_rand_unif(g) > P)
		count++;

	return (count);
}

double 
tw_rand_normal01(tw_rng_stream * g, unsigned int *rng_calls)
{
#ifndef RAND_NORMAL
	tw_error(TW_LOC, "Please compile using -DRAND_NORMAL!");
#endif

#ifdef RAND_NORMAL
	*rng_calls = 0;
	g->tw_normal_flipflop = !g->tw_normal_flipflop;

  if ((g->tw_normal_flipflop)  || 
      (g->tw_normal_u1< 0.0)   || 
      (g->tw_normal_u1 >= 1.0) || 
      (g->tw_normal_u2 < 0.0)  || 
      (g->tw_normal_u2 > 1.0))
    {
      g->tw_normal_u1 = tw_rand_unif(g);
      g->tw_normal_u2 = tw_rand_unif(g);
      *rng_calls = 2;

      return (sqrt(-2.0 * log(g->tw_normal_u1)) * sin(tw_opi * g->tw_normal_u2));
    } 
  else
    {
      return (sqrt(-2.0 * log(g->tw_normal_u1)) * cos(tw_opi * g->tw_normal_u2));
    }
#endif
}

double 
tw_rand_normal_sd(tw_rng_stream * g, double Mu, double Sd, unsigned int *rng_calls)
{
  return ( Mu + (tw_rand_normal01(g, rng_calls) * Sd));
}

long 
tw_rand_poisson(tw_rng_stream * g, double Lambda)
{
  double          a, b;
  long            count;
  
  a = exp(-Lambda);
  b = 1;
  count = 0;
  
  b = b * tw_rand_unif(g);
  
  while (b >= a)
    {
      count++;
      b = b * tw_rand_unif(g);
    }
  
  return (count);
}

double
tw_rand_lognormal(tw_rng_stream * g, double mean, double sd, unsigned int *rng_calls)
{
  return (exp( mean + sd * tw_rand_normal01(g, rng_calls)));
}

double
tw_rand_weibull(tw_rng_stream * g, double mean, double shape)
{
  double scale = mean /  tgamma( ((double)1.0 + (double)1.0/shape));
  return(scale * pow(-log( tw_rand_unif(g)), (double)1.0/shape));
}
