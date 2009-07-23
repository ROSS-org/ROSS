#include "phold.h"

tw_peid
phold_map(tw_lpid gid)
{
	return (tw_peid) gid / g_tw_nlp;
}

void
phold_init(phold_state * s, tw_lp * lp)
{
	int              i;

#if 0
	if(g_tw_rng_default == TW_FALSE)
		tw_rand_init_streams(lp, 1);
#endif

	for (i = 0; i < g_phold_start_events; i++)
	{
		tw_event_send(
			tw_event_new(lp->gid, 
				     tw_rand_exponential(lp->rng, mean), 
				     lp));
	}
}

void
phold_event_handler(phold_state * s, tw_bf * bf, phold_message * m, tw_lp * lp)
{
	tw_lpid	 dest;

	if(tw_rand_unif(lp->rng) <= percent_remote)
	{
		bf->c1 = 1;
		dest = tw_rand_integer(lp->rng, 0, ttl_lps - 1);

		dest += offset_lpid;

		if(dest >= ttl_lps)
			dest -= ttl_lps;
	} else
	{
		bf->c1 = 0;
		dest = lp->gid;
	}

	if(dest < 0 || dest >= (g_tw_nlp * tw_nnodes()))
		tw_error(TW_LOC, "bad dest");

	tw_event_send(tw_event_new(dest, tw_rand_exponential(lp->rng, mean), lp));
}

void
phold_event_handler_rc(phold_state * s, tw_bf * bf, phold_message * m, tw_lp * lp)
{
	tw_rand_reverse_unif(lp->rng);
	tw_rand_reverse_unif(lp->rng);

	if(bf->c1 == 1)
		tw_rand_reverse_unif(lp->rng);
}

void
phold_finish(phold_state * s, tw_lp * lp)
{
}

tw_lptype       mylps[] = {
	{(init_f) phold_init,
	 (event_f) phold_event_handler,
	 (revent_f) phold_event_handler_rc,
	 (final_f) phold_finish,
	 (map_f) phold_map,
	sizeof(phold_state)},
	{0},
};

const tw_optdef app_opt[] =
{
	TWOPT_GROUP("PHOLD Model"),
	TWOPT_STIME("remote", percent_remote, "desired remote event rate"),
	TWOPT_UINT("nlp", nlp_per_pe, "number of LPs per processor"),
	TWOPT_STIME("mean", mean, "exponential distribution mean for timestamps"),
	TWOPT_STIME("mult", mult, "multiplier for event memory allocation"),
	TWOPT_UINT("start-events", g_phold_start_events, "number of initial messages per LP"),
	TWOPT_UINT("memory", optimistic_memory, "additional memory buffers"),
	TWOPT_CHAR("run", run_id, "user supplied run name"),
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
	g_tw_events_per_pe = (mult * nlp_per_pe * g_phold_start_events) + 
				optimistic_memory;
	//g_tw_rng_default = TW_FALSE;

	tw_define_lps(nlp_per_pe, sizeof(phold_message), 0);

	for(i = 0; i < g_tw_nlp; i++)
		tw_lp_settype(i, &mylps[0]);

	tw_run();

	return 0;
}
