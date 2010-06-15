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
 * IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *
 */
#ifndef INC_tw_rand_h
#define	INC_tw_rand_h

#define tw_opi 6.28318530718
#define tw_rand_unif(G)			rng_gen_val(G)
#define tw_rand_reverse_unif(G)	rng_gen_reverse_val(G)

FWD(struct, tw_rng);
FWD(struct, tw_rng_stream);

/*
 * Public Function Prototypes
 */
extern tw_rng	*tw_rand_init(uint64_t v, uint64_t w);
extern void	tw_rand_initial_seed(tw_rng_stream * g, tw_lpid id);
extern long     tw_rand_integer(tw_rng_stream * g, long low, long high);
extern long     tw_rand_binomial(tw_rng_stream * g, long N, double P);
extern double   tw_rand_exponential(tw_rng_stream * g, double Lambda);
extern double   tw_rand_pareto(tw_rng_stream * g, double scale, double shape);
extern double   tw_rand_gamma(tw_rng_stream * g, double shape, double scale);
extern long     tw_rand_geometric(tw_rng_stream * g, double P);
extern double   tw_rand_normal01(tw_rng_stream * g, unsigned int *rng_calls);
extern double   tw_rand_normal_sd(tw_rng_stream * g, double Mu, double Sd, unsigned int *rng_calls);
extern long     tw_rand_poisson(tw_rng_stream * g, double Lambda);
extern double   tw_rand_weibull(tw_rng_stream * g, double mean, double shape);

#endif
