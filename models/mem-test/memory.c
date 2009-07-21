#include "memory.h"

void
mem_stats_print()
{
	if(!tw_ismaster())
		return;

	printf("\nMemory Model Statistics:\n");
	printf("\t%-50s %11lld\n", "Total Rollbacks", g_stats.s_rb);
	printf("\t%-50s %11lld\n", "Total Events Sent", g_stats.s_sent);
	printf("\t%-50s %11lld\n", "Total Events Recv", g_stats.s_recv);
	printf("\n");
	printf("\t%-50s %11lld\n", "Total Allocated", g_stats.s_mem_alloc);
	printf("\t%-50s %11lld\n", "Total Allocated RC", g_stats.s_mem_alloc_rc);
	printf("\t%-50s %11lld\n", "NET Allocated", g_stats.s_mem_alloc - g_stats.s_mem_alloc_rc);
	printf("\n");
	printf("\t%-50s %11lld\n", "Total Freed", g_stats.s_mem_free);
	printf("\t%-50s %11lld\n", "Total Freed RC", g_stats.s_mem_free_rc);
	printf("\t%-50s %11lld *\n", "NET Freed", g_stats.s_mem_free - g_stats.s_mem_free_rc);
	printf("\n");
	printf("\t%-50s %11lld\n", "Total Gets", g_stats.s_mem_get);
	printf("\t%-50s %11lld\n", "Total Gets RC", g_stats.s_mem_get_rc);
	printf("\t%-50s %11lld *\n", "NET Gets", g_stats.s_mem_get - g_stats.s_mem_get_rc);
	printf("\n");
}

void
mem_fill(tw_memory * m)
{
	mem_packet	*p;

	p = tw_memory_data(m);
	strcpy(p->bytes, test_string);
}

void
mem_verify(tw_memory * m)
{
	mem_packet	*p;

	p = tw_memory_data(m);

	if(0 != (strcmp(p->bytes, test_string)))
	{
		tw_error(TW_LOC, "String does not match: %s \n", p->bytes);
	}
}

tw_memory	*
mem_alloc(tw_lp * lp)
{
	tw_memory	*m;

	m = tw_memory_alloc(lp, my_fd);

	if(m->next)
		tw_error(TW_LOC, "Next pointer is set!");

	if(m->prev)
		tw_error(TW_LOC, "Prev pointer is set!");

	return m;
};

tw_peid
phold_map(tw_lpid gid)
{
	return (tw_peid) gid / nlp_per_pe;
}

void
phold_init(phold_state * s, tw_lp * lp)
{
	tw_event	*e;
	tw_memory	*b;

	int              i;

	for (i = 0; i < g_phold_start_events; i++)
	{
		e = tw_event_new(lp->gid, 1.0 + tw_rand_exponential(lp->rng, mean), lp);

		for(i = 0; i < nbufs; i++)
		{
			b = mem_alloc(lp);
			mem_fill(b);
			tw_event_memory_set(e, b, my_fd);

			s->stats.s_mem_alloc++;
		}

		tw_event_send(e);
		s->stats.s_sent++;
	}
}

void
phold_event_handler(phold_state * s, tw_bf * bf, phold_message * m, tw_lp * lp)
{
	tw_lpid		 dest;
	tw_event	*e;
	tw_memory	*b;

	int		 i;

	s->stats.s_recv++;

	// read membufs off inbound event, check it and free it
	for(i = 0; i < nbufs; i++)
	{
		b = tw_event_memory_get(lp);

		if(!b)
			tw_error(TW_LOC, "Missing memory buffers: %d of %d",
					i+1, nbufs);

		mem_verify(b);
		tw_memory_free(lp, b, my_fd);

		s->stats.s_mem_free++;
		s->stats.s_mem_get++;
	}

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

	e = tw_event_new(dest, 1.0 + tw_rand_exponential(lp->rng, mean), lp);

	// allocate membufs and attach them to the event
	for(i = 0; i < nbufs; i++)
	{
		b = mem_alloc(lp);

		if(!b)
			tw_error(TW_LOC, "no membuf allocated!");

		mem_fill(b);
		tw_event_memory_set(e, b, my_fd);
		s->stats.s_mem_alloc++;
	}

	tw_event_send(e);
}

void
phold_event_handler_rc(phold_state * s, tw_bf * bf, phold_message * m, tw_lp * lp)
{
	tw_memory	*b;

	int		 i;

	s->stats.s_rb++;
	s->stats.s_recv--;

	tw_rand_reverse_unif(lp->rng);
	tw_rand_reverse_unif(lp->rng);

	if(bf->c1 == 1)
		tw_rand_reverse_unif(lp->rng);

	// undo the membuf frees and reattach them to the RB event
	for(i = 0; i < nbufs; i++)
	{
		b = tw_memory_free_rc(lp, my_fd);
		s->stats.s_mem_free_rc++;
		tw_event_memory_get_rc(lp, b, my_fd);
		s->stats.s_mem_get_rc++;

		/* 
		 * unnecessary to undo the allocs .. 
		 * they will be reclaimed when the events are reclaimed.
		 */
	}

	// sanity check
	if(i != nbufs)
		tw_error(TW_LOC, "Did not free_rc %d (%d) memory buffers!",
				 nbufs, i);
}

void
phold_finish(phold_state * s, tw_lp * lp)
{
	g_stats.s_rb += s->stats.s_rb;
	g_stats.s_sent += s->stats.s_sent;
	g_stats.s_recv += s->stats.s_recv;

	g_stats.s_mem_alloc += s->stats.s_mem_alloc;
	g_stats.s_mem_alloc_rc += s->stats.s_mem_alloc_rc;

	g_stats.s_mem_free += s->stats.s_mem_free;
	g_stats.s_mem_free_rc += s->stats.s_mem_free_rc;

	g_stats.s_mem_get += s->stats.s_mem_get;
	g_stats.s_mem_get_rc += s->stats.s_mem_get_rc;
}

tw_lptype mylps[] =
{
	{
		(init_f) phold_init,
		(event_f) phold_event_handler,
		(revent_f) phold_event_handler_rc,
		(final_f) phold_finish,
		(map_f) phold_map,
		sizeof(phold_state)
	},
	{0},
};

const tw_optdef app_opt[] =
{
	TWOPT_GROUP("Memory Model"),
	TWOPT_UINT("nbuffers", nbufs, "number of memory buffers per event"),
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
	g_tw_memory_nqueues = 1;
	g_tw_events_per_pe = (mult * nlp_per_pe * g_phold_start_events) + 
				optimistic_memory;

	tw_define_lps(nlp_per_pe, sizeof(phold_message), 0);

	for(i = 0; i < nlp_per_pe; i++)
		tw_lp_settype(i, &mylps[0]);

	// init the KP memory queues
	for(i = 0; i < g_tw_nkp; i++)
		my_fd = tw_kp_memory_init(tw_getkp(i), Q_SIZE, sizeof(mem_packet), 1);

	tw_run();

	mem_stats_print();

	return 0;
}