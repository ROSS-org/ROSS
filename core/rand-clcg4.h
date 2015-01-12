#ifndef INC_clcg4_h
#define INC_clcg4_h

typedef int32_t * tw_seed;

struct tw_rng
{
	/*
	 * equals a[i]^{m[i]-2} mod m[i]
	 */
	long long	b[4];

	/*
	 * a[j]^{2^w} et a[j]^{2^{v+w}}.
	 */
	int32_t	m[4];
	int32_t	a[4];
	int32_t	aw[4];
	int32_t	avw[4];

	// the seed..
	int32_t	seed[4];
};

enum SeedType
{
	InitialSeed, LastSeed, NewSeed
};

typedef enum SeedType SeedType;

struct tw_rng_stream
{
    unsigned long count;
	int32_t	 Ig[4];
	int32_t	 Lg[4];
	int32_t	 Cg[4];

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
extern void     rng_set_seed(tw_rng_stream * g, uint32_t * s);
extern void     rng_get_state(tw_rng_stream * g, uint32_t * s);
extern void     rng_write_state(tw_rng_stream * g, FILE *f);
extern double   rng_gen_val(tw_rng_stream * g);
extern double   rng_gen_reverse_val(tw_rng_stream * g);

#endif
