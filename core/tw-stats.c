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
show_1f(const char *name, double v)
{
	printf("\t%-50s %11.1f\n", name, v);
	fprintf(g_tw_csv, "%.2f,", v);
}

static void
show_4f(const char *name, double v)
{
	printf("\t%-50s %11.4lf\n", name, v);
	fprintf(g_tw_csv, "%.4lf,", v);
}

#endif

void
tw_stats(tw_pe * me)
{
	tw_statistics s;

	tw_pe	*pe;
	tw_kp	*kp;
	tw_lp	*lp = NULL;

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

		s.s_max_run_time = ROSS_MAX(s.s_max_run_time, tw_wall_to_double(&rt));
		s.s_nevent_abort += pe->stats.s_nevent_abort;
		s.s_pq_qsize += tw_pq_get_size(me->pq);

		s.s_nsend_net_remote += pe->stats.s_nsend_net_remote;
		s.s_nsend_loc_remote += pe->stats.s_nsend_loc_remote;

		s.s_nsend_network += pe->stats.s_nsend_network;
		s.s_nread_network += pe->stats.s_nread_network;
		s.s_nsend_remote_rb += pe->stats.s_nsend_remote_rb;

		s.s_total += pe->stats.s_total;
		s.s_net_read += pe->stats.s_net_read;
		s.s_gvt += pe->stats.s_gvt;
		s.s_fossil_collect += pe->stats.s_fossil_collect;
		s.s_event_abort += pe->stats.s_event_abort;
		s.s_event_process += pe->stats.s_event_process;
		s.s_pq += pe->stats.s_pq;
		s.s_rollback += pe->stats.s_rollback;
		s.s_cancel_q += pe->stats.s_cancel_q;
        s.s_pe_event_ties += pe->stats.s_pe_event_ties;
        s.s_min_detected_offset = g_tw_min_detected_offset;
        s.s_avl += pe->stats.s_avl;
        s.s_buddy += pe->stats.s_buddy;
        s.s_lz4 += pe->stats.s_lz4;
        s.s_events_past_end += pe->stats.s_events_past_end;

		for(i = 0; i < g_tw_nkp; i++)
		{
			kp = tw_getkp(i);
			s.s_nevent_processed += kp->s_nevent_processed;
			s.s_e_rbs += kp->s_e_rbs;
			s.s_rb_total += kp->s_rb_total;
			s.s_rb_secondary += kp->s_rb_secondary;
		}

		for(i = 0; i < g_tw_nlp; i++)
		{
			lp = tw_getlp(i);
			if (lp->type->final)
				(*lp->type->final) (lp->cur_state, lp);
		}
	}

	s.s_fc_attempts = g_tw_fossil_attempts;
	s.s_net_events = s.s_nevent_processed - s.s_e_rbs;
	s.s_rb_primary = s.s_rb_total - s.s_rb_secondary;

	s = *(tw_net_statistics(me, &s));

	if (!tw_ismaster())
		return;

#ifndef ROSS_DO_NOT_PRINT
	printf("\n\t: Running Time = %.4f seconds\n", s.s_max_run_time);
	fprintf(g_tw_csv, "%.4f,", s.s_max_run_time);

	printf("\nTW Library Statistics:\n");
	show_lld("Total Events Processed", s.s_nevent_processed);
	show_lld("Events Aborted (part of RBs)", s.s_nevent_abort);
	show_lld("Events Rolled Back", s.s_e_rbs);
	show_lld("Event Ties Detected in PE Queues", s.s_pe_event_ties);
        if(g_tw_synchronization_protocol == CONSERVATIVE)
            printf("\t%-50s %11.9lf\n",
               "Minimum TS Offset Detected in Conservative Mode",  
               (double) s.s_min_detected_offset);
	show_2f("Efficiency", 100.0 * (1.0 - ((double) s.s_e_rbs / (double) s.s_net_events)));
	show_lld("Total Remote (shared mem) Events Processed", s.s_nsend_loc_remote);

	show_2f(
		"Percent Remote Events",
		( (double)s.s_nsend_loc_remote
		/ (double)s.s_net_events)
		* 100.0
	);

	show_lld("Total Remote (network) Events Processed", s.s_nsend_net_remote);
	show_2f(
		"Percent Remote Events",
		( (double)s.s_nsend_net_remote
		/ (double)s.s_net_events)
		* 100.0
	);

	printf("\n");
	show_lld("Total Roll Backs ", s.s_rb_total);
	show_lld("Primary Roll Backs ", s.s_rb_primary);
	show_lld("Secondary Roll Backs ", s.s_rb_secondary);
	show_lld("Fossil Collect Attempts", s.s_fc_attempts);
	show_lld("Total GVT Computations", g_tw_gvt_done);

	printf("\n");
	show_lld("Net Events Processed", s.s_net_events);
	show_1f(
		"Event Rate (events/sec)",
		((double)s.s_net_events / s.s_max_run_time)
	);

        show_lld("Total Events Scheduled Past End Time", s.s_events_past_end);
        
	printf("\nTW Memory Statistics:\n");
	show_lld("Events Allocated", g_tw_events_per_pe * g_tw_npe);
	show_lld("Memory Allocated", m_alloc / 1024);
	show_lld("Memory Wasted", m_waste / 1024);

	if (tw_nnodes() > 1) {
		printf("\n");
		printf("TW Network Statistics:\n");
		show_lld("Remote sends", s.s_nsend_network);
		show_lld("Remote recvs", s.s_nread_network);
	}

	printf("\nTW Data Structure sizes in bytes (sizeof):\n");
	show_lld("PE struct", sizeof(tw_pe));
	show_lld("KP struct", sizeof(tw_kp));
	show_lld("LP struct", sizeof(tw_lp));
	show_lld("LP Model struct", lp->type->state_sz);
	show_lld("LP RNGs", sizeof(*lp->rng));
	show_lld("Total LP", sizeof(tw_lp) + lp->type->state_sz + sizeof(*lp->rng));
	show_lld("Event struct", sizeof(tw_event));
	show_lld("Event struct with Model", sizeof(tw_event) + g_tw_msg_sz);

#ifdef ROSS_timing
	printf("\nTW Clock Cycle Statistics (MAX values in secs at %1.4lf GHz):\n", g_tw_clock_rate / 1000000000.0);
	show_4f("Priority Queue (enq/deq)", (double) s.s_pq / g_tw_clock_rate);
    show_4f("AVL Tree (insert/delete)", (double) s.s_avl / g_tw_clock_rate);
    show_4f("LZ4 (de)compression", (double) s.s_lz4 / g_tw_clock_rate);
    show_4f("Buddy system", (double) s.s_buddy / g_tw_clock_rate);
	show_4f("Event Processing", (double) s.s_event_process / g_tw_clock_rate);
	show_4f("Event Cancel", (double) s.s_cancel_q / g_tw_clock_rate);
	show_4f("Event Abort", (double) s.s_event_abort / g_tw_clock_rate);
	printf("\n");
	show_4f("GVT", (double) s.s_gvt / g_tw_clock_rate);
	show_4f("Fossil Collect", (double) s.s_fossil_collect / g_tw_clock_rate);
	show_4f("Primary Rollbacks", (double) s.s_rollback / g_tw_clock_rate);
	show_4f("Network Read", (double) s.s_net_read / g_tw_clock_rate);
	show_4f("Total Time (Note: Using Running Time above for Speedup)", (double) s.s_total / g_tw_clock_rate);
#endif

	tw_gvt_stats(stdout);
#endif
}
