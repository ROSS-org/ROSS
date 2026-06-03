/*
 * Lifecycle smoke driver: tw_init -> register one minimal LP -> tw_run
 * with zero scheduled events -> tw_end. The only assertion is the exit
 * code -- if any of init / run / end aborts, ctest fails. Caught here
 * means caught the moment it appears, not when phold's teardown
 * happens to expose it.
 */

#include "test_lp_minimal.h"

int
main(int argc, char **argv)
{
    tw_init(&argc, &argv);

    tw_define_lps(1, sizeof(test_lp_minimal_message));
    tw_lp_settype(0, &test_lp_minimal_type[0]);

    tw_run();
    tw_end();

    return 0;
}
