#include "phold.h"

tw_peid phold_map(tw_lpid gid) { return (tw_peid)gid / g_tw_nlp; }

void phold_init(phold_state *s, tw_lp *lp) {
  (void)s;

  tw_stime stagger_offset = 0.0;
  if (stagger) {
    stagger_offset = lp->gid % (unsigned int)g_tw_ts_end;
  }
  for (int i = 0; i < g_phold_start_events; i++) {
    tw_event_send(tw_event_new(
        lp->gid,
        tw_rand_exponential(lp->rng, mean) + lookahead + stagger_offset, lp));
  }
}

void phold_pre_run(phold_state *s, tw_lp *lp) {
  (void)s;
  tw_lpid dest;

  if (tw_rand_unif(lp->rng) <= percent_remote) {
    dest = tw_rand_integer(lp->rng, 0, ttl_lps - 1);
  } else {
    dest = lp->gid;
  }

  if (dest >= (g_tw_nlp * tw_nnodes())) {
    tw_error(TW_LOC, "bad dest");
  }

  tw_event_send(
      tw_event_new(dest, tw_rand_exponential(lp->rng, mean) + lookahead, lp));
}

void phold_event_handler(phold_state *s, tw_bf *bf, phold_message *m,
                         tw_lp *lp) {
  (void)s;
  (void)m;
  bf->c1 = 0;
  bf->c2 = 0;
  tw_lpid dest = lp->gid;

  if (tw_rand_unif(lp->rng) <= percent_remote) {
    bf->c1 = 1;
    dest = tw_rand_integer(lp->rng, 0, ttl_lps - 1);
    // Makes PHOLD non-deterministic across processors! Don't uncomment
    /* dest += offset_lpid; */
    /* if(dest >= ttl_lps) */
    /*  dest -= ttl_lps; */
  }

  if (dest >= (g_tw_nlp * tw_nnodes())) {
    tw_error(TW_LOC, "bad dest");
  }

  tw_event_send(
      tw_event_new(dest, tw_rand_exponential(lp->rng, mean) + lookahead, lp));

#ifdef TRIGGER_BY_MODEL
  // trigger GVT hook around every 1000 events
  int const random_occurence = tw_rand_integer(lp->rng, 0, 1000);
  if (lp->gid == 0 && random_occurence == 0) {
    bf->c2 = 1;
    tw_trigger_gvt_hook_now(lp);
  }
#endif
}

void phold_event_handler_rc(phold_state *s, tw_bf *bf, phold_message *m,
                            tw_lp *lp) {
  (void)s;
  (void)m;
  tw_rand_reverse_unif(lp->rng);
  tw_rand_reverse_unif(lp->rng);

  if (bf->c1) {
    tw_rand_reverse_unif(lp->rng);
  }

#ifdef TRIGGER_BY_MODEL
  tw_rand_reverse_unif(lp->rng);
  if (bf->c2) {
    tw_trigger_gvt_hook_now_rev(lp);
  }
#endif
}

void phold_commit(phold_state *s, tw_bf *bf, phold_message *m, tw_lp *lp) {
  (void)s;
  (void)bf;
  (void)m;
  (void)lp;
}

void phold_finish(phold_state *s, tw_lp *lp) {
  (void)s;
  (void)lp;
}

tw_lptype mylps[] = {
    {(init_f)phold_init,
     /* (pre_run_f) phold_pre_run, */
     (pre_run_f)NULL, (event_f)phold_event_handler,
     (revent_f)phold_event_handler_rc, (commit_f)phold_commit,
     (final_f)phold_finish, (map_f)phold_map, sizeof(phold_state)},
    {0},
};

void gvt_hook(tw_pe * pe, bool past_end_time) {
#ifdef USE_RAND_TIEBREAKER
    tw_event_sig gvt_sig = pe->GVT_sig;
    tw_stime gvt = gvt_sig.recv_ts;
#else
    tw_stime gvt = pe->GVT;
#endif

  if (g_tw_mynode == 0) {
    printf("Current GVT time %f\n", gvt);
  }

#ifdef TRIGGER_AT_TIMESTAMP
  static float trigger_at = 1.0; // initial value is 1.0, then 2.0, 4, 8, 16, ...
  trigger_at *= 2;
  tw_trigger_gvt_hook_at(trigger_at);
#endif
}

const tw_optdef app_opt[] = {
    TWOPT_GROUP("PHOLD Model"),
    TWOPT_DOUBLE("remote", percent_remote, "desired remote event rate"),
    TWOPT_UINT("nlp", nlp_per_pe, "number of LPs per processor"),
    TWOPT_DOUBLE("mean", mean, "exponential distribution mean for timestamps"),
    TWOPT_DOUBLE("mult", mult, "multiplier for event memory allocation"),
    TWOPT_DOUBLE("lookahead", lookahead, "lookahead for events"),
    TWOPT_UINT("start-events", g_phold_start_events,
               "number of initial messages per LP"),
    TWOPT_UINT("stagger", stagger,
               "Set to 1 to stagger event uniformly across 0 to end time."),
    TWOPT_UINT("memory", optimistic_memory, "additional memory buffers"),
    TWOPT_CHAR("run", run_id, "user supplied run name"),
    TWOPT_END()};

int main(int argc, char **argv) {
#ifdef TEST_COMM_ROSS
  // Init outside of ROSS
  MPI_Init(&argc, &argv);
  // Split COMM_WORLD in half even/odd
  int mpi_rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
  MPI_Comm split_comm;
  MPI_Comm_split(MPI_COMM_WORLD, mpi_rank % 2, mpi_rank, &split_comm);
  if (mpi_rank % 2 == 1) {
    // tests should catch any MPI_COMM_WORLD collectives
    MPI_Finalize();
    return 0;
  }
  // Allows ROSS to function as normal
  tw_comm_set(split_comm);
#endif

  // set a min lookahead of 1.0
  lookahead = 1.0;
  tw_opt_add(app_opt);
  tw_init(&argc, &argv);

  if (lookahead > 1.0) {
    tw_error(TW_LOC, "Lookahead > 1.0 .. needs to be less\n");
  }

  // setting up GVT hook
  g_tw_gvt_hook = gvt_hook;
#ifdef TRIGGER_BY_MODEL
  tw_trigger_gvt_hook_when_model_calls();
#else
#ifdef TRIGGER_AT_TIMESTAMP
  tw_trigger_gvt_hook_at(1.0);
#else // by default, the GVT function is called every 500 GVT operations
  tw_trigger_gvt_hook_every(500);
#endif
#endif

  // reset mean based on lookahead
  mean = mean - lookahead;

  offset_lpid = g_tw_mynode * nlp_per_pe;
  ttl_lps = tw_nnodes() * nlp_per_pe;
  g_tw_events_per_pe =
      (mult * nlp_per_pe * g_phold_start_events) + optimistic_memory;
  // g_tw_rng_default = TW_FALSE;
  g_tw_lookahead = lookahead;

  tw_define_lps(nlp_per_pe, sizeof(phold_message));

  for (unsigned int i = 0; i < g_tw_nlp; i++) {
    tw_lp_settype(i, &mylps[0]);
  }

  if (g_tw_mynode == 0) {
    printf("========================================\n");
    printf("PHOLD Model Configuration..............\n");
    printf("   Lookahead..............%lf\n", lookahead);
    printf("   Start-events...........%u\n",  g_phold_start_events);
    printf("   stagger................%u\n",  stagger);
    printf("   Mean...................%lf\n", mean);
    printf("   Mult...................%lf\n", mult);
    printf("   Memory.................%u\n",  optimistic_memory);
    printf("   Remote.................%lf\n", percent_remote);
    printf("========================================\n\n");
  }

  tw_run();
  tw_end();

#ifdef TEST_COMM_ROSS
  MPI_Finalize();
#endif

  return 0;
}
