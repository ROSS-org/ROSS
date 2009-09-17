#include "tmr.h"

tw_peid
tmr_map(tw_lpid gid)
{
	return (tw_peid) gid / g_tw_nlp;
}

void
tmr_init(tmr_state * s, tw_lp * lp)
{
	int              i;

	if(!lp->rng)
	{
		tw_error(TW_LOC, "No RNG!");
		lp->rng = tw_calloc(TW_LOC, "LP RNG", sizeof(tw_rng_stream), 1);
		tw_rand_initial_seed(lp->pe->rng, lp->rng, lp->gid);
	}

	for (i = 0; i < g_tmr_start_events; i++)
	{
		tw_event_send(
			tw_event_new(lp->gid, 
				     tw_rand_exponential(lp->rng, mean), 
				     lp));
	}

	if((s->timer = tw_timer_init(lp, 100.0)) == NULL)
		tw_error(TW_LOC, "scheduled timer past end time \n");
}

void
tmr_event_handler(tmr_state * s, tw_bf * bf, tmr_message * m, tw_lp * lp)
{
	tw_lpid	 dest;

	* (int *) bf = 0;

	if(s->timer)
	{
		// if current event being processed is the timer
		if(tw_event_data(s->timer) == m)
		{
			bf->c1 = 1;

			m->old_timer = s->timer;
			s->timer = NULL;

			fprintf(f, "%lld: tmr fired at %lf\n", 
				lp->gid, tw_now(lp));

			return;
		} else
		{
			bf->c2 = 1;

			m->old_time = s->timer->recv_ts;
			tw_timer_reset(lp, &s->timer, tw_now(lp) + 100.0);

			if(s->timer == NULL)
				fprintf(f, "%lld: reset tmr failed %lf\n", 
					lp->gid, tw_now(lp) + 100.0);
			else
				fprintf(f, "%lld: reset tmr to %lf\n", 
					lp->gid, tw_now(lp) + 100.0);
		}
	}

	if(tw_rand_unif(lp->rng) <= percent_remote)
	{
		bf->c3 = 1;
		dest = tw_rand_integer(lp->rng, 0, ttl_lps - 1);

		dest += offset_lpid;

		if(dest >= ttl_lps)
			dest -= ttl_lps;
	} else
	{
		bf->c3 = 0;
		dest = lp->gid;
	}

	if(!lp->gid)
		dest = 0;

	tw_event_send(tw_event_new(dest, tw_rand_exponential(lp->rng, mean), lp));
}

void
tmr_event_handler_rc(tmr_state * s, tw_bf * bf, tmr_message * m, tw_lp * lp)
{
	if(bf->c1)
	{
		// old timer may be NULL
		fprintf(f, "%lld: rc tmr fired at %lf\n",
			lp->gid, tw_now(lp));

		s->timer = m->old_timer;
		m->old_timer = NULL;

		return;
	}

	if(bf->c2)
	{
		if(s->timer == NULL)
		{
			s->timer = tw_timer_init(lp, m->old_time);
			m->old_time = 0.0;

			fprintf(f, "%lld: restore tmr to %lf\n", 
				lp->gid, s->timer->recv_ts);
		} else
		{
			tw_timer_reset(lp, &s->timer, m->old_time);
			m->old_time = 0.0;

			fprintf(f, "%lld: reset tmr to %lf \n",
				lp->gid, s->timer->recv_ts);
		}
	}

	tw_rand_reverse_unif(lp->rng);
	tw_rand_reverse_unif(lp->rng);

	if(bf->c3 == 1)
		tw_rand_reverse_unif(lp->rng);
}

void
tmr_finish(tmr_state * s, tw_lp * lp)
{
}

tw_lptype       mylps[] = {
	{
	 (init_f) tmr_init,
	 (event_f) tmr_event_handler,
	 (revent_f) tmr_event_handler_rc,
	 (final_f) tmr_finish,
	 (map_f) tmr_map,
	 sizeof(tmr_state)
	},
	{0},
};

const tw_optdef app_opt[] =
{
	TWOPT_GROUP("TIMER Model"),
	TWOPT_STIME("remote", percent_remote, "desired remote event rate"),
	TWOPT_UINT("nlp", nlp_per_pe, "number of LPs per processor"),
	TWOPT_STIME("mean", mean, "exponential distribution mean for timestamps"),
	TWOPT_STIME("mult", mult, "multiplier for event memory allocation"),
	TWOPT_UINT("start-events", g_tmr_start_events, "number of initial messages per LP"),
	TWOPT_UINT("memory", optimistic_memory, "additional memory buffers"),
	TWOPT_CHAR("run", run_id, "user supplied run name"),
	TWOPT_CHAR("verbose", verbose, "verbose statements"),
	TWOPT_END()
};

int
main(int argc, char **argv, char **env)
{
	int		 i;

	tw_opt_add(app_opt);
	tw_init(&argc, &argv);

	offset_lpid = g_tw_mynode * nlp_per_pe;
	ttl_lps = tw_nnodes() * g_tw_npe * nlp_per_pe;
	g_tw_memory_nqueues = 1;
	g_tw_events_per_pe = (mult * nlp_per_pe * g_tmr_start_events) + 
				optimistic_memory;

	tw_define_lps(nlp_per_pe, sizeof(tmr_message), 0);

	// use g_tw_nlp now
	for(i = 0; i < g_tw_nlp; i++)
		tw_lp_settype(i, &mylps[0]);

	tw_kp_memory_init(g_tw_kp, 1000, 100, 1);

	if(verbose)
		f = stdout;
	else
		f = fopen("output", "w");
		//f = fopen("/dev/null", "w");

	tw_run();
	tw_end();

	return 0;
}
