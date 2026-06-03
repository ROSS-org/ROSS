/*
 * Splay queue ordering driver.
 *
 * Schedules a known set of self-events on a single LP with a mix of
 * distinct and tied timestamps in non-sorted enqueue order, then
 * verifies the splay queue delivers them with monotonically
 * non-decreasing recv_ts. The exact order of tied events depends on
 * USE_RAND_TIEBREAKER and the LP's RNG state, so we don't pin that --
 * only the bedrock priority-queue invariant (no smaller-ts event
 * delivered after a larger-ts one) plus exact delivery count.
 *
 * Sequential only (--synch=1). The event handler mutates LP state
 * without a reverse handler, so replay under --synch=4/6 would
 * scramble the test's own bookkeeping.
 */

#undef NDEBUG  /* assertions must run regardless of build type */
#include <assert.h>

#include <ross.h>

typedef struct splay_state {
    tw_stime last_ts;
    int      delivery_count;
} splay_state;

typedef struct splay_message {
    int sched_index;
} splay_message;

/* 20 events: mix of distinct timestamps (so ordering is checkable)
 * and ties (so the comparator's tie path is exercised), enqueued in
 * deliberately non-sorted order to force the splay tree to balance
 * across both branches on each insert. */
static const double schedule[] = {
    5.0, 1.0, 9.0, 3.0, 7.0, 2.0, 8.0, 4.0, 6.0, 10.0,
    5.0, 1.0, 3.0, 7.0, 1.0, 5.0, 9.0, 3.0, 1.0, 5.0
};
#define N_EVENTS (sizeof(schedule) / sizeof(schedule[0]))

static void
splay_init(splay_state *s, tw_lp *lp)
{
    s->last_ts = 0.0;
    s->delivery_count = 0;

    for (unsigned int i = 0; i < N_EVENTS; i++) {
        tw_event *e = tw_event_new(lp->gid, schedule[i], lp);
        splay_message *m = (splay_message *) tw_event_data(e);
        m->sched_index = (int) i;
        tw_event_send(e);
    }
}

static void
splay_event(splay_state *s, tw_bf *bf, splay_message *m, tw_lp *lp)
{
    (void) bf;
    (void) m;

    tw_stime now = tw_now(lp);
    assert(now >= s->last_ts && "splay delivered event out of timestamp order");
    s->last_ts = now;
    s->delivery_count++;
}

static void
splay_final(splay_state *s, tw_lp *lp)
{
    (void) lp;
    assert(s->delivery_count == (int) N_EVENTS
        && "splay lost or duplicated events");
}

static tw_peid
splay_map(tw_lpid gid)
{
    return (tw_peid) gid / g_tw_nlp;
}

static tw_lptype splay_lp_type[] = {
    {(init_f)    splay_init,
     (pre_run_f) NULL,
     (event_f)   splay_event,
     (revent_f)  NULL,
     (commit_f)  NULL,
     (final_f)   splay_final,
     (map_f)     splay_map,
     sizeof(splay_state)},
    {0},
};

int
main(int argc, char **argv)
{
    tw_init(&argc, &argv);

    tw_define_lps(1, sizeof(splay_message));
    tw_lp_settype(0, &splay_lp_type[0]);

    tw_run();
    tw_end();

    return 0;
}
