#include <ross.h>

#ifndef ROSS_DO_NOT_PRINT
static void
show_lld(const char *name, tw_stat v)
{
	printf("\t%-50s %11lld\n", name, v);
	fprintf(g_tw_csv, "%lld,", v);
}

static void
show_2f(const char *name, double v)
{
	printf("\t%-50s %11.2f %%\n", name, v);
	fprintf(g_tw_csv, "%.2f,", v);
}

static void
show_4f(const char *name, double v)
{
	printf("\t%-50s %11.4f %%\n", name, v);
	fprintf(g_tw_csv, "%.4f,", v);
}
#endif

void
tw_stats(tw_pe * me)
{
	tw_statistics s;
	tw_pe	*pe;
	tw_kp	*kp;
	tw_lp	*lp;
	int	 i;

	size_t m_alloc, m_waste;

	if (me != g_tw_pe[0])
		return;

	if (0 == g_tw_sim_started)
		return;

	tw_calloc_stats(&m_alloc, &m_waste);
	bzero(&s, sizeof(s));

	for(pe = NULL; (pe = tw_pe_next(pe));)
	{
		tw_wtime rt;

		tw_wall_sub(&rt, &pe->end_time, &pe->start_time);

		s.max_run_time = max(s.max_run_time, tw_wall_to_double(&rt));
		s.tw_s_nevent_abort += pe->s_nevent_abort;
		s.tw_s_pq_qsize += tw_pq_get_size(me->pq);

		s.tw_s_nsend_net_remote += pe->s_nsend_net_remote;
		s.tw_s_nsend_loc_remote += pe->s_nsend_loc_remote;

		s.tw_s_nsend_network += pe->s_nsend_network;
		s.tw_s_nrecv_network += pe->s_nrecv_network;
		s.tw_s_nsend_remote_rb += pe->s_nsend_remote_rb;

		for(i = 0; i < g_tw_nkp; i++)
		{
			kp = tw_getkp(i);
			s.tw_s_nevent_processed += kp->s_nevent_processed;
			s.tw_s_e_rbs += kp->s_e_rbs;
			s.tw_s_rb_total += kp->s_rb_total;
			s.tw_s_rb_secondary += kp->s_rb_secondary;
		}

		for(i = 0; i < g_tw_nlp; i++)
		{
			lp = tw_getlp(i);
			if (lp->type.final)
				(*lp->type.final) (lp->cur_state, lp);
		}
	}

	s.tw_s_fc_attempts = g_tw_fossil_attempts;
	s.tw_s_net_events = s.tw_s_nevent_processed - s.tw_s_e_rbs;
	s.tw_s_rb_primary = s.tw_s_rb_total - s.tw_s_rb_secondary;

	s = *(tw_net_statistics(me, &s));

	if (!tw_ismaster())
		return;

#ifndef ROSS_DO_NOT_PRINT
	printf("\n\t: Running Time = %.3f seconds\n", s.max_run_time);
	fprintf(g_tw_csv, "%.4f,", s.max_run_time);

	printf("\nTW Library Statistics:\n");
	show_lld("Total Events Processed", s.tw_s_nevent_processed);
	show_lld("Events Aborted (part of RBs)", s.tw_s_nevent_abort);
	show_lld("Events Rolled Back", s.tw_s_e_rbs);
	show_2f("Efficiency", 100.0 * (1.0 - ((double) s.tw_s_e_rbs / (double) s.tw_s_net_events)));
	show_lld("Total Remote (shared mem) Events Processed", s.tw_s_nsend_loc_remote);

	show_2f(
		"Percent Remote Events",
		( (double)s.tw_s_nsend_loc_remote
		/ (double)s.tw_s_net_events)
		* 100.0
	);

	show_lld("Total Remote (network) Events Processed", s.tw_s_nsend_net_remote);
	show_2f(
		"Percent Remote Events",
		( (double)s.tw_s_nsend_net_remote
		/ (double)s.tw_s_net_events)
		* 100.0
	);

	printf("\n");
	show_lld("Total Roll Backs ", s.tw_s_rb_total);
	show_lld("Primary Roll Backs ", s.tw_s_rb_primary);
	show_lld("Secondary Roll Backs ", s.tw_s_rb_secondary);
	show_lld("Fossil Collect Attempts", s.tw_s_fc_attempts);
	show_lld("Total GVT Computations", g_tw_gvt_done);

	printf("\n");
	show_lld("Net Events Processed", s.tw_s_net_events);
	show_4f(
		"Event Rate (events/sec)",
		((double)s.tw_s_net_events / s.max_run_time)
	);

	printf("\n");
	printf("TW Memory Statistics:\n");
	show_lld("Events Allocated", g_tw_events_per_pe * g_tw_npe);
	show_lld("Memory Allocated", m_alloc / 1024);
	show_lld("Memory Wasted", m_waste / 1024);

	if (tw_nnodes() > 1) {
		printf("\n");
		printf("TW Network Statistics:\n");
		show_lld("Remote sends", s.tw_s_nsend_network);
		show_lld("Remote recvs", s.tw_s_nrecv_network);
	}

	printf("\n");
	printf("TW Data Structure sizes in bytes (sizeof):\n");
	show_lld("PE struct", sizeof(tw_pe));
	show_lld("KP struct", sizeof(tw_kp));
	show_lld("LP struct", sizeof(tw_lp));
	show_lld("LP Model struct", lp->type.state_sz);
	show_lld("LP RNGs", sizeof(*lp->rng));
	show_lld("Total LP", sizeof(tw_lp) + lp->type.state_sz + sizeof(*lp->rng));
	show_lld("Event struct", sizeof(tw_event));
	show_lld("Event struct with Model", sizeof(tw_event) + g_tw_msg_sz);

	printf("\n");
	tw_gvt_stats(stdout);

	printf("\n");
#endif
}
