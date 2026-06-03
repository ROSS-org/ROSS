#include "test_lp_minimal.h"

static void
test_lp_minimal_init(test_lp_minimal_state *s, tw_lp *lp)
{
    (void) s;
    (void) lp;
}

static void
test_lp_minimal_pre_run(test_lp_minimal_state *s, tw_lp *lp)
{
    (void) s;
    (void) lp;
}

static void
test_lp_minimal_event_handler(test_lp_minimal_state *s, tw_bf *bf,
                              test_lp_minimal_message *m, tw_lp *lp)
{
    (void) s;
    (void) bf;
    (void) m;
    (void) lp;
}

static void
test_lp_minimal_event_handler_rc(test_lp_minimal_state *s, tw_bf *bf,
                                 test_lp_minimal_message *m, tw_lp *lp)
{
    (void) s;
    (void) bf;
    (void) m;
    (void) lp;
}

static void
test_lp_minimal_commit(test_lp_minimal_state *s, tw_bf *bf,
                       test_lp_minimal_message *m, tw_lp *lp)
{
    (void) s;
    (void) bf;
    (void) m;
    (void) lp;
}

static void
test_lp_minimal_final(test_lp_minimal_state *s, tw_lp *lp)
{
    (void) s;
    (void) lp;
}

static tw_peid
test_lp_minimal_map(tw_lpid gid)
{
    return (tw_peid) gid / g_tw_nlp;
}

tw_lptype test_lp_minimal_type[] = {
    {(init_f)    test_lp_minimal_init,
     (pre_run_f) test_lp_minimal_pre_run,
     (event_f)   test_lp_minimal_event_handler,
     (revent_f)  test_lp_minimal_event_handler_rc,
     (commit_f)  test_lp_minimal_commit,
     (final_f)   test_lp_minimal_final,
     (map_f)     test_lp_minimal_map,
     sizeof(test_lp_minimal_state)},
    {0},
};
