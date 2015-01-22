#ifndef INC_tw_rand_h
#define	INC_tw_rand_h

#define tw_opi 6.28318530718
#define tw_rand_unif(G)			rng_gen_val(G)
#define tw_rand_reverse_unif(G)	rng_gen_reverse_val(G)

typedef struct tw_rng tw_rng;
typedef struct tw_rng_stream tw_rng_stream;

/*
 * Public Function Prototypes
 */
extern tw_rng	*tw_rand_init(uint32_t v, uint32_t w);
extern void	tw_rand_initial_seed(tw_rng_stream * g, tw_lpid id);
extern long     tw_rand_integer(tw_rng_stream * g, long low, long high);
extern unsigned long tw_rand_ulong(tw_rng_stream * g, unsigned long low, unsigned long high);
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
