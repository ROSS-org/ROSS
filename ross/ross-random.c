/*
 * ROSS: Rensselaer's Optimistic Simulation System.
 * Copyright (c) 1999-2003 Rensselaer Polytechnic Instutitute.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *
 *      This product includes software developed by David Bauer,
 *      Dr. Christopher D.  Carothers, and Shawn Pearce of the
 *      Department of Computer Science at Rensselaer Polytechnic
 *      Institute.
 *
 * 4. Neither the name of the University nor of the developers may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * 5. The use or inclusion of this software or its documentation in
 *    any commercial product or distribution of this software to any
 *    other party without specific, written prior permission is
 *    prohibited.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS
 * IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDINg, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * REgENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAgES
 * (INCLUDINg, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE gOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDINg NEgLIgENCE OR OTHERWISE)
 * ARISINg IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAgE.
 *
 *
 */

#include <ross.h>

/*
 * defines                                                             
 */
#ifdef RAND_NORMAL
tw_volatile double *g_tw_normal_u1;
tw_volatile double *g_tw_normal_u2;
tw_volatile int *g_tw_normal_flipflop;
#endif

/*
 * tw_rand_init
 */
tw_rng	*
tw_rand_init(uint64_t v, uint64_t w)
{
	return rng_init(v, w);
}

/*
 * tw_rand_integer
 *
 * For LP # gen, return a uniform rn from low to high 
 */

long 
tw_rand_integer(tw_rng_stream * g, long low, long high)
{
	if (high < low)
		return (0);
	else
		return (low + (long)(tw_rand_unif(g) * (high + 1 - low)));
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
tw_rand_normal01(tw_rng_stream * g)
{
#if !RAND_NORMAL
	tw_error(TW_LOC, "Please compile using -DRAND_NORMAL!");
#endif

#if RAND_NORMAL
  g_tw_normal_flipflop[g->id] = !g_tw_normal_flipflop[g->id];

  if ((g_tw_normal_flipflop[g->id]) || 
      (g_tw_normal_u1[g->id] < 0.0) || 
      (g_tw_normal_u1[g->id] >= 1.0) || 
      (g_tw_normal_u2[g->id] < 0.0) || 
      (g_tw_normal_u2[g->id] > 1.0))
    {
      g_tw_normal_u1[g->id] = tw_rand_unif(g);
      g_tw_normal_u2[g->id] = tw_rand_unif(g);
      return (sqrt(-2.0 * log(g_tw_normal_u1[g->id])) * sin(tw_opi * g_tw_normal_u2[g->id]));
    } 
  else
    {
      return (sqrt(-2.0 * log(g_tw_normal_u1[g->id])) * cos(tw_opi * g_tw_normal_u2[g->id]));
    }
#endif
}

/*
  double 
  tw_rand_normal01(tw_rng_stream * g)
  {
  static int      FlipFlop = 0;
  static double   u1, u2;
  
  FlipFlop = !FlipFlop;
  if ((FlipFlop) || (u1 < 0.0) || (u1 >= 1.0) || (u2 < 0.0) || (u2 > 1.0))
  {
  u1 = tw_rand_unif(g);
  u2 = tw_rand_unif(g);
  return (sqrt(-2.0 * log(u1)) * sin(tw_opi * u2));
  } else
  {
  return (sqrt(-2.0 * log(u1)) * cos(tw_opi * u2));
  }
  }
*/

double 
tw_rand_normal_sd(tw_rng_stream * g, double Mu, double Sd)
{
	return (tw_rand_normal01(g) * Sd + Mu);
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
tw_rand_lognormal(tw_rng_stream * g, double mean, double sd)
{
  return (exp( mean + sd * tw_rand_normal01(g)));
}

double
tw_rand_weibull(tw_rng_stream * g, double mean, double shape)
{
  double scale = mean /  tgamma( ((double)1.0 + (double)1.0/shape));
  return(scale * pow(-log( tw_rand_unif(g)), (double)1.0/shape));
}
