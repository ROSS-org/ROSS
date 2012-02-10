#ifndef INC_clcg4_h
#define INC_clcg4_h

typedef long * tw_seed;

typedef enum SeedType SeedType;

struct tw_rng
{
	/*
	 * equals a[i]^{m[i]-2} mod m[i]
	 */
	long long	b[4];

	/*
	 * a[j]^{2^w} et a[j]^{2^{v+w}}.
	 */
	long	m[4];
	long	a[4];
	long	aw[4];
	long	avw[4];

	// the seed..
	long	seed[4];
};

enum SeedType
{
	InitialSeed, LastSeed, NewSeed
};

struct tw_rng_stream
{
	long	 Ig[4];
	long	 Lg[4];
	long	 Cg[4];

	//tw_rng	*rng;

#ifdef RAND_NORMAL
	double	 tw_normal_u1;
	double	 tw_normal_u2;
	int	 tw_normal_flipflop;
#endif
};

extern tw_rng	*rng_init(int v, int w);
extern void     rng_set_initial_seed();
extern void     rng_init_generator(tw_rng_stream * g, SeedType Where);
extern void     rng_set_seed(tw_rng_stream * g, long * s);
extern void     rng_get_state(tw_rng_stream * g, long * s);
extern void     rng_write_state(tw_rng_stream * g, FILE *f);
extern double   rng_gen_val(tw_rng_stream * g);
extern double   rng_gen_reverse_val(tw_rng_stream * g);

#endif
