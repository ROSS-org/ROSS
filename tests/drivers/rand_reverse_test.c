/*
 * rand-clcg4 reverse-symmetry driver.
 *
 * Forward draws on a tw_rng_stream are reversed via tw_rand_reverse_unif
 * (the only public reverse; tw_rand_integer/exponential/ulong each cost
 * one unif draw, so they reverse with one reverse_unif). After N forward
 * + N reverse calls the stream's current seed (Cg) and draw counter must
 * be byte-identical to the pre-draw snapshot. Asserting this directly
 * catches rand-clcg4 arithmetic regressions that phold's specific draw
 * pattern might not happen to tickle.
 *
 * Run before tw_run() so the engine's own RNG use doesn't enter the
 * picture -- this isolates the test to rand-clcg4 itself.
 */

#undef NDEBUG  /* assertions must run regardless of build type */
#include <assert.h>
#include <string.h>

#include "test_lp_minimal.h"

typedef struct rng_snapshot {
    int32_t Cg[4];
    unsigned long count;
} rng_snapshot;

static void
snapshot_rng(rng_snapshot *out, const tw_rng_stream *g)
{
    memcpy(out->Cg, g->Cg, sizeof(out->Cg));
    out->count = g->count;
}

static int
rng_matches(const rng_snapshot *before, const tw_rng_stream *now)
{
    return memcmp(before->Cg, now->Cg, sizeof(before->Cg)) == 0
        && before->count == now->count;
}

int
main(int argc, char **argv)
{
    tw_init(&argc, &argv);

    tw_define_lps(1, sizeof(test_lp_minimal_message));
    tw_lp_settype(0, &test_lp_minimal_type[0]);

    tw_rng_stream *rng = &g_tw_lp[0]->rng[0];
    rng_snapshot before;

    /* Bulk unif <-> reverse_unif symmetry. The bedrock invariant. */
    snapshot_rng(&before, rng);
    for (int i = 0; i < 1000; i++)
        tw_rand_unif(rng);
    for (int i = 0; i < 1000; i++)
        tw_rand_reverse_unif(rng);
    assert(rng_matches(&before, rng) && "tw_rand_unif/reverse_unif asymmetry");

    /* Each of these is documented to cost exactly one rng_gen_val call;
     * the assertion catches a regression where one of them silently
     * starts drawing more (or fewer) than once. */
    snapshot_rng(&before, rng);
    (void) tw_rand_integer(rng, 0, 1000);
    tw_rand_reverse_unif(rng);
    assert(rng_matches(&before, rng) && "tw_rand_integer is not one unif draw");

    snapshot_rng(&before, rng);
    (void) tw_rand_exponential(rng, 1.0);
    tw_rand_reverse_unif(rng);
    assert(rng_matches(&before, rng) && "tw_rand_exponential is not one unif draw");

    snapshot_rng(&before, rng);
    (void) tw_rand_ulong(rng, 0, 1000);
    tw_rand_reverse_unif(rng);
    assert(rng_matches(&before, rng) && "tw_rand_ulong is not one unif draw");

    tw_run();
    tw_end();

    return 0;
}
