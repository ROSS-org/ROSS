#include "phold.h"

static unsigned int offset_lpid = 0;
static tw_stime mult = 1.4;
static tw_stime percent_remote = 0.25;
static unsigned int ttl_lps = 0;
static unsigned int nlp = 8;
static int g_phold_start_events = 1;
static int optimistic_memory = 100;
static char run_id[1024] = "undefined";

// rate for timestamp exponential distribution
static tw_stime mean = 1.0;

void
phold_init(phold_state * s, tw_lp * lp)
{
	int              i;

	for (i = 0; i < g_phold_start_events; i++)
	{
		rn_event_send(
			rn_event_new(lp->gid, 
				     tw_rand_exponential(lp->rng, mean), 
				     lp, DOWNSTREAM, 0));
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

	rn_event_send(
		rn_event_new(dest, tw_rand_exponential(lp->rng, mean), 
				lp, DOWNSTREAM, 0));
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

const tw_optdef app_opt[] =
{
	TWOPT_GROUP("PHOLD Model"),
	TWOPT_STIME("remote", percent_remote, "desired remote event rate"),
	TWOPT_UINT("nlp", nlp, "number of LPs per processor"),
	TWOPT_STIME("mean", mean, "exponential distribution mean for timestamps"),
	TWOPT_STIME("mult", mult, "multiplier for event memory allocation"),
	TWOPT_UINT("start-events", g_phold_start_events, "number of initial messages per LP"),
	TWOPT_UINT("memory", optimistic_memory, "additional memory buffers"),
	TWOPT_CHAR("run", run_id, "user supplied run name"),
	TWOPT_END()
};

int
phold_main(int argc, char **argv, char **env)
{
	int		 i;

	tw_opt_add(app_opt);

	offset_lpid = g_tw_mynode * g_tw_nlp;
	ttl_lps = tw_nnodes() * g_tw_npe * g_tw_nlp;

	return 0;
}
