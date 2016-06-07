#include <ross.h>
#include <sys/stat.h>

char g_tw_stats_out[128] = {0};
int g_tw_stats_enabled = 0;
long g_tw_time_interval = 0;
long g_tw_current_interval = 0;
tw_clock g_tw_real_time_samp = 0;
tw_clock g_tw_real_samp_start_cycles = 0;
long g_tw_ = 0;
int g_tw_pe_per_file = 1;
int g_tw_my_file_id = 0;
tw_stat_list *g_tw_stat_head = NULL;
tw_stat_list *g_tw_stat_tail = NULL;
MPI_File gvt_file;
MPI_File interval_file;

static const tw_optdef stats_options[] = {
    TWOPT_GROUP("ROSS Stats"),
    TWOPT_UINT("enable-stats", g_tw_stats_enabled, "0 no stats, 1 for stats"), 
    TWOPT_UINT("time-interval", g_tw_time_interval, "collect stats for specified sim time interval"), 
    TWOPT_UINT("real-time-samp", g_tw_real_time_samp, "real time sampling interval in ms"), 
    TWOPT_CHAR("stats-filename", g_tw_stats_out, "filename for stats output"),
    TWOPT_UINT("pe-per-file", g_tw_pe_per_file, "how many PEs to output per file"), 
    TWOPT_END()
};

const tw_optdef *tw_stats_setup(void)
{
	return stats_options;
}

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

/** write header line to stats output files */
void tw_gvt_stats_file_setup(tw_peid id)
{
    int max_files_directory = 100;
    char directory_path[128];
    char filename[256];
    if (g_tw_stats_out[0])
    {
        sprintf(directory_path, "%s-gvt-%d", g_tw_stats_out, g_tw_my_file_id/max_files_directory);
        mkdir(directory_path, S_IRUSR | S_IWUSR | S_IXUSR);
        sprintf(filename, "%s/%s-%d-gvt.txt", directory_path, g_tw_stats_out, g_tw_my_file_id);
    }
    else
    {
        sprintf(directory_path, "ross-gvt-%d", g_tw_my_file_id/max_files_directory);
        mkdir(directory_path, S_IRUSR | S_IWUSR | S_IXUSR);
        sprintf( filename, "%s/ross-gvt-stats-%d.txt", directory_path, g_tw_my_file_id);
    }

    MPI_File_open(stats_comm, filename, MPI_MODE_CREATE | MPI_MODE_WRONLY, MPI_INFO_NULL, &gvt_file);
    
    char buffer[1024];
	sprintf(buffer, "PE,GVT,Total All Reduce Calls," 
        "total events processed,events aborted,events rolled back,event ties detected in PE queues," 
        "efficiency,total remote network events processed," 
        "total rollbacks,primary rollbacks,secondary roll backs,fossil collect attempts," 
        "net events processed,remote sends,remote recvs\n");
    MPI_File_write(gvt_file, buffer, strlen(buffer), MPI_CHAR, MPI_STATUS_IGNORE);
}

/** write header line to stats output files */
void tw_interval_stats_file_setup(tw_peid id)
{
/*    tw_stat forward_events;
    tw_stat reverse_events;
    tw_stat num_gvts;
    tw_stat all_reduce_calls;
    tw_stat events_aborted;
    tw_stat pe_event_ties;

    tw_stat nevents_remote;
    tw_stat nsend_network;
    tw_stat nread_network;

    tw_stat events_rbs;
    tw_stat rb_primary;
    tw_stat rb_secondary;
    tw_stat fc_attempts;
    */
    int max_files_directory = 100;
    char directory_path[128];
    char filename[256];
    if (g_tw_stats_out[0])
    {
        sprintf(directory_path, "%s-interval-%d", g_tw_stats_out, g_tw_my_file_id/max_files_directory);
        mkdir(directory_path, S_IRUSR | S_IWUSR | S_IXUSR);
        sprintf(filename, "%s/%s-%d-interval.txt", directory_path, g_tw_stats_out, g_tw_my_file_id);
    }
    else
    {
        sprintf(directory_path, "ross-interval-%d", g_tw_my_file_id/max_files_directory);
        mkdir(directory_path, S_IRUSR | S_IWUSR | S_IXUSR);
        sprintf( filename, "%s/ross-interval-stats-%d.txt", directory_path, g_tw_my_file_id);
    }

    MPI_File_open(stats_comm, filename, MPI_MODE_CREATE | MPI_MODE_WRONLY, MPI_INFO_NULL, &interval_file);
    
    char buffer[1024];
	sprintf(buffer, "PE,interval,forward events,reverse events,number of GVT comps,all reduce calls,"
        "events aborted,event ties detected in PE queues,remote events,network sends,network recvs,"
        "events rolled back,primary rollbacks,secondary roll backs,fossil collect attempts\n");
    MPI_File_write(interval_file, buffer, strlen(buffer), MPI_CHAR, MPI_STATUS_IGNORE);
}

void
tw_get_stats(tw_pe * me, tw_statistics *s)
{
	tw_pe	*pe;
	tw_kp	*kp;

	int	 i;

	if (me != g_tw_pe[0])
		return;

	if (0 == g_tw_sim_started)
		return;

	bzero(s, sizeof(*s));

	for(pe = NULL; (pe = tw_pe_next(pe));)
	{
		tw_wtime rt;

		tw_wall_sub(&rt, &pe->end_time, &pe->start_time);

		s->s_max_run_time = ROSS_MAX(s->s_max_run_time, tw_wall_to_double(&rt));
		s->s_nevent_abort += pe->stats.s_nevent_abort;
		s->s_pq_qsize += tw_pq_get_size(me->pq);

		s->s_nsend_net_remote += pe->stats.s_nsend_net_remote;
		s->s_nsend_loc_remote += pe->stats.s_nsend_loc_remote;

		s->s_nsend_network += pe->stats.s_nsend_network;
		s->s_nread_network += pe->stats.s_nread_network;
		s->s_nsend_remote_rb += pe->stats.s_nsend_remote_rb;

		s->s_total += pe->stats.s_total;
		s->s_net_read += pe->stats.s_net_read;
		s->s_gvt += pe->stats.s_gvt;
		s->s_fossil_collect += pe->stats.s_fossil_collect;
		s->s_event_abort += pe->stats.s_event_abort;
		s->s_event_process += pe->stats.s_event_process;
		s->s_pq += pe->stats.s_pq;
		s->s_rollback += pe->stats.s_rollback;
		s->s_cancel_q += pe->stats.s_cancel_q;
        s->s_pe_event_ties += pe->stats.s_pe_event_ties;
        s->s_min_detected_offset = g_tw_min_detected_offset;
        s->s_avl += pe->stats.s_avl;
        s->s_buddy += pe->stats.s_buddy;
        s->s_lz4 += pe->stats.s_lz4;
        s->s_events_past_end += pe->stats.s_events_past_end;

		for(i = 0; i < g_tw_nkp; i++)
		{
			kp = tw_getkp(i);
			s->s_nevent_processed += kp->s_nevent_processed;
			s->s_e_rbs += kp->s_e_rbs;
			s->s_rb_total += kp->s_rb_total;
			s->s_rb_secondary += kp->s_rb_secondary;
		}

	}

	s->s_fc_attempts = g_tw_fossil_attempts;
	s->s_net_events = s->s_nevent_processed - s->s_e_rbs;
	s->s_rb_primary = s->s_rb_total - s->s_rb_secondary;
}

void
tw_stats(tw_pe *me)
{
    tw_statistics s;
	size_t m_alloc, m_waste;
	tw_calloc_stats(&m_alloc, &m_waste);
    tw_lp *lp = NULL;
    int i;
    for(i = 0; i < g_tw_nlp; i++)
    {
        lp = tw_getlp(i);
        if (lp->type->final)
            (*lp->type->final) (lp->cur_state, lp);
    }
    tw_get_stats(me, &s);
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
	show_lld("Events Allocated",(1+g_tw_events_per_pe+g_tw_events_per_pe_extra) * g_tw_npe);
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
	show_4f("Statistics Computation", (double) s.s_stat_comp / g_tw_clock_rate);
	show_4f("Statistics Write", (double) s.s_stat_write / g_tw_clock_rate);
	show_4f("Total Time (Note: Using Running Time above for Speedup)", (double) s.s_total / g_tw_clock_rate);
#endif

	tw_gvt_stats(stdout);
#endif
}

void get_time_ahead_GVT(tw_pe *me)
{
    tw_kp *kp;
    int i;
    printf("%lf,", tw_clock_read() / g_tw_clock_rate);    
    for(i = 0; i < g_tw_nkp; i++)
    {
        kp = tw_getkp(i);
        printf("%lf", kp->last_time - me->GVT);
        if(i < g_tw_nkp - 1)
           printf(",");
        else
           printf("\n"); 
    }
}
