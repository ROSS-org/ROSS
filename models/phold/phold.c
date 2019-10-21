#include "phold.h"


tw_peid
phold_map(tw_lpid gid)
{
	return (tw_peid) gid / g_tw_nlp;
}

void
phold_init(phold_state * s, tw_lp * lp)
{
    (void) s;
	int              i;

	if( stagger )
	  {
	    for (i = 0; i < g_phold_start_events; i++)
	      {
		tw_event_send(
			      tw_event_new(lp->gid,
					   tw_rand_exponential(lp->rng, mean) + lookahead + (tw_stime)(lp->gid % (unsigned int)g_tw_ts_end),
					   lp));
	      }
	  }
	else
	  {
	    for (i = 0; i < g_phold_start_events; i++)
	      {
		tw_event_send(
			      tw_event_new(lp->gid,
					   tw_rand_exponential(lp->rng, mean) + lookahead,
					   lp));
	      }
	  }
}

void
phold_pre_run(phold_state * s, tw_lp * lp)
{
    (void) s;
    tw_lpid	 dest;

	if(tw_rand_unif(lp->rng) <= percent_remote)
	{
		dest = tw_rand_integer(lp->rng, 0, ttl_lps - 1);
	} else
	{
		dest = lp->gid;
	}

	if(dest >= (g_tw_nlp * tw_nnodes()))
		tw_error(TW_LOC, "bad dest");

	tw_event_send(tw_event_new(dest, tw_rand_exponential(lp->rng, mean) + lookahead, lp));
}

void
phold_event_handler(phold_state * s, tw_bf * bf, phold_message * m, tw_lp * lp)
{
    (void) s;
    (void) m;
	tw_lpid	 dest;

	if(tw_rand_unif(lp->rng) <= percent_remote)
	{
		bf->c1 = 1;
		dest = tw_rand_integer(lp->rng, 0, ttl_lps - 1);
		// Makes PHOLD non-deterministic across processors! Don't uncomment
		/* dest += offset_lpid; */
		/* if(dest >= ttl_lps) */
		/* 	dest -= ttl_lps; */
	} else
	{
		bf->c1 = 0;
		dest = lp->gid;
	}

	if(dest >= (g_tw_nlp * tw_nnodes()))
		tw_error(TW_LOC, "bad dest");

	tw_event_send(tw_event_new(dest, tw_rand_exponential(lp->rng, mean) + lookahead, lp));
}

void
phold_event_handler_rc(phold_state * s, tw_bf * bf, phold_message * m, tw_lp * lp)
{
    (void) s;
    (void) m;
	tw_rand_reverse_unif(lp->rng);
	tw_rand_reverse_unif(lp->rng);

	if(bf->c1 == 1)
		tw_rand_reverse_unif(lp->rng);
}

void phold_commit(phold_state * s, tw_bf * bf, phold_message * m, tw_lp * lp)
{
    (void) s;
    (void) bf;
    (void) m;
    (void) lp;
}

void
phold_finish(phold_state * s, tw_lp * lp)
{
    (void) s;
    (void) lp;
}

tw_lptype       mylps[] = {
	{(init_f) phold_init,
     /* (pre_run_f) phold_pre_run, */
     (pre_run_f) NULL,
	 (event_f) phold_event_handler,
	 (revent_f) phold_event_handler_rc,
	 (commit_f) phold_commit,
	 (final_f) phold_finish,
	 (map_f) phold_map,
	sizeof(phold_state)},
	{0},
};

void event_trace(phold_message *m, tw_lp *lp, char *buffer, int *collect_flag)
{
    (void) m;
    (void) lp;
    (void) buffer;
    (void) collect_flag;
    return;
}

void phold_stats_collect(phold_state *s, tw_lp *lp, char *buffer)
{
    (void) s;
    (void) lp;
    (void) buffer;
    return;
}

st_model_types model_types[] = {
    {(ev_trace_f) event_trace,
     0,
    (model_stat_f) phold_stats_collect,
    sizeof(int),
    NULL, //(sample_event_f)
    NULL, //(sample_revent_f)
    0},
    {0}
};

const tw_optdef app_opt[] =
{
	TWOPT_GROUP("PHOLD Model"),
	TWOPT_DOUBLE("remote", percent_remote, "desired remote event rate"),
	TWOPT_UINT("nlp", nlp_per_pe, "number of LPs per processor"),
	TWOPT_DOUBLE("mean", mean, "exponential distribution mean for timestamps"),
	TWOPT_DOUBLE("mult", mult, "multiplier for event memory allocation"),
	TWOPT_DOUBLE("lookahead", lookahead, "lookahead for events"),
	TWOPT_UINT("start-events", g_phold_start_events, "number of initial messages per LP"),
	TWOPT_UINT("stagger", stagger, "Set to 1 to stagger event uniformly across 0 to end time."),
	TWOPT_UINT("memory", optimistic_memory, "additional memory buffers"),
	TWOPT_CHAR("run", run_id, "user supplied run name"),
	TWOPT_END()
};

int
main(int argc, char **argv)
{

#ifdef TEST_COMM_ROSS
    // Init outside of ROSS
    MPI_Init(&argc, &argv);
    // Split COMM_WORLD in half even/odd
    int mpi_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
    MPI_Comm split_comm;
    MPI_Comm_split(MPI_COMM_WORLD, mpi_rank%2, mpi_rank, &split_comm);
    if(mpi_rank%2 == 1){
        // tests should catch any MPI_COMM_WORLD collectives
        MPI_Finalize();
    }
    // Allows ROSS to function as normal
    tw_comm_set(split_comm);
#endif

	unsigned int i;

	// set a min lookahead of 1.0
	lookahead = 1.0;
	tw_opt_add(app_opt);
	tw_init(&argc, &argv);

#ifdef USE_DAMARIS
    if(g_st_ross_rank)
    { // only ross ranks should run code between here and tw_run()
#endif
	if( lookahead > 1.0 )
	  tw_error(TW_LOC, "Lookahead > 1.0 .. needs to be less\n");

	//reset mean based on lookahead
        mean = mean - lookahead;

	offset_lpid = g_tw_mynode * nlp_per_pe;
	ttl_lps = tw_nnodes() * nlp_per_pe;
	g_tw_events_per_pe = (mult * nlp_per_pe * g_phold_start_events) +
				optimistic_memory;
	//g_tw_rng_default = TW_FALSE;
	g_tw_lookahead = lookahead;

	tw_define_lps(nlp_per_pe, sizeof(phold_message));

	for(i = 0; i < g_tw_nlp; i++)
    {
		tw_lp_settype(i, &mylps[0]);
        st_model_settype(i, &model_types[0]);
    }

        if( g_tw_mynode == 0 )
	  {
	    printf("========================================\n");
	    printf("PHOLD Model Configuration..............\n");
	    printf("   Lookahead..............%lf\n", lookahead);
	    printf("   Start-events...........%u\n", g_phold_start_events);
	    printf("   stagger................%u\n", stagger);
	    printf("   Mean...................%lf\n", mean);
	    printf("   Mult...................%lf\n", mult);
	    printf("   Memory.................%u\n", optimistic_memory);
	    printf("   Remote.................%lf\n", percent_remote);
	    printf("========================================\n\n");
	  }

	tw_run();
#ifdef USE_DAMARIS
    } // end if(g_st_ross_rank)
#endif
	tw_end();

	return 0;
}
