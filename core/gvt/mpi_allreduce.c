#include <ross.h>

#define TW_GVT_NORMAL 0
#define TW_GVT_COMPUTE 1

static unsigned int g_tw_gvt_max_no_change = 10000;
static unsigned int g_tw_gvt_no_change = 0;
static tw_stat all_reduce_cnt = 0;
static unsigned int gvt_cnt = 0;
static unsigned int gvt_force = 0;
static tw_statistics last_stats = {0};
static tw_stat last_all_reduce_cnt = 0;

static const tw_optdef gvt_opts [] =
{
	TWOPT_GROUP("ROSS MPI GVT"),
	TWOPT_UINT("gvt-interval", g_tw_gvt_interval, "GVT Interval"),
	TWOPT_STIME("report-interval", gvt_print_interval, 
			"percent of runtime to print GVT"),
	TWOPT_END()
};

const tw_optdef *
tw_gvt_setup(void)
{
	gvt_cnt = 0;

	return gvt_opts;
}

void
tw_gvt_start(void)
{
}

void
tw_gvt_force_update(tw_pe *me)
{
	gvt_force++;
	gvt_cnt = g_tw_gvt_interval;
}

void
tw_gvt_stats(FILE * f)
{
	fprintf(f, "\nTW GVT Statistics: MPI AllReduce\n");
	fprintf(f, "\t%-50s %11d\n", "GVT Interval", g_tw_gvt_interval);
	fprintf(f, "\t%-50s %11d\n", "Batch Size", g_tw_mblock);
	fprintf(f, "\n");
	fprintf(f, "\t%-50s %11d\n", "Forced GVT", gvt_force);
	fprintf(f, "\t%-50s %11d\n", "Total GVT Computations", g_tw_gvt_done);
	fprintf(f, "\t%-50s %11lld\n", "Total All Reduce Calls", all_reduce_cnt);
	fprintf(f, "\t%-50s %11.2lf\n", "Average Reduction / GVT", 
			(double) ((double) all_reduce_cnt / (double) g_tw_gvt_done));
}

void
tw_gvt_log(FILE * f, tw_pe *me, tw_stime gvt, tw_statistics *s)
{
	// GVT,Forced GVT, Total GVT Computations, Total All Reduce Calls, Average Reduction / GVT
    // total events processed, events aborted, events rolled back, event ties detected in PE queues
    // efficiency, total remote network events processed, percent remote events
    // total rollbacks, primary rollbacks, secondary roll backs, fossil collect attempts
    // net events processed, remote sends, remote recvs
    tw_clock start_cycle_time = tw_clock_read();
    char buffer[2048];
	sprintf(buffer, "%ld,%f,%lld,%lld,%lld,%lld,%lld,%f,%lld,%lld,%lld,%lld,%lld,%lld,%lld,%lld\n", 
            g_tw_mynode, gvt, all_reduce_cnt-last_all_reduce_cnt, 
            s->s_nevent_processed-last_stats.s_nevent_processed, s->s_nevent_abort-last_stats.s_nevent_abort, 
            s->s_e_rbs-last_stats.s_e_rbs, s->s_pe_event_ties-last_stats.s_pe_event_ties,
            100.0 * (1.0 - ((double) (s->s_e_rbs-last_stats.s_e_rbs)/(double) (s->s_net_events-last_stats.s_net_events))),
            s->s_nsend_net_remote-last_stats.s_nsend_net_remote,
            s->s_rb_total-last_stats.s_rb_total, s->s_rb_primary-last_stats.s_rb_primary,
            s->s_rb_secondary-last_stats.s_rb_secondary, s->s_fc_attempts-last_stats.s_fc_attempts,
            s->s_net_events-last_stats.s_net_events, s->s_nsend_network-last_stats.s_nsend_network, 
            s->s_nread_network-last_stats.s_nread_network);

    stat_comp_cycle_counter += tw_clock_read() - start_cycle_time;
    start_cycle_time = tw_clock_read();
    MPI_File_write(gvt_file, buffer, strlen(buffer), MPI_CHAR, MPI_STATUS_IGNORE);
    stat_write_cycle_counter += tw_clock_read() - start_cycle_time;
    start_cycle_time = tw_clock_read();
    memcpy(&last_stats, s, sizeof(tw_statistics));
    last_all_reduce_cnt = all_reduce_cnt;
    stat_comp_cycle_counter += tw_clock_read() - start_cycle_time;
}

void
tw_gvt_step1(tw_pe *me)
{
	if(me->gvt_status == TW_GVT_COMPUTE ||
		++gvt_cnt < g_tw_gvt_interval)
		return;

	me->gvt_status = TW_GVT_COMPUTE;
}

void
tw_gvt_step2(tw_pe *me)
{
	long long local_white = 0;
	long long total_white = 0;

	tw_stime pq_min = DBL_MAX;
	tw_stime net_min = DBL_MAX;

	tw_stime lvt;
	tw_stime gvt;

	tw_clock start = tw_clock_read();

	if(me->gvt_status != TW_GVT_COMPUTE)
		return;
    int tmp_all_red_cnt = 0;
	while(1)
	  {
	    tw_net_read(me);
	
	    // send message counts to create consistent cut
	    local_white = me->s_nwhite_sent - me->s_nwhite_recv;
	    all_reduce_cnt++;
        tmp_all_red_cnt++;
	    if(MPI_Allreduce(
			     &local_white,
			     &total_white,
			     1,
			     MPI_LONG_LONG,
			     MPI_SUM,
			     MPI_COMM_WORLD) != MPI_SUCCESS)
	      tw_error(TW_LOC, "MPI_Allreduce for GVT failed");
	    
	    if(total_white == 0)
	      break;
	  }

	pq_min = tw_pq_minimum(me->pq);
	net_min = tw_net_minimum(me);

	lvt = me->trans_msg_ts;
	if(lvt > pq_min)
	  lvt = pq_min;
	if(lvt > net_min)
		lvt = net_min;

	all_reduce_cnt++;
    tmp_all_red_cnt++;
	if(MPI_Allreduce(
			&lvt,
			&gvt,
			1,
			MPI_DOUBLE,
			MPI_MIN,
			MPI_COMM_WORLD) != MPI_SUCCESS)
			tw_error(TW_LOC, "MPI_Allreduce for GVT failed");

	gvt = ROSS_MIN(gvt, me->GVT_prev);

	if(gvt != me->GVT_prev)
	{
		g_tw_gvt_no_change = 0;
	} else
	{
		g_tw_gvt_no_change++;
		if (g_tw_gvt_no_change >= g_tw_gvt_max_no_change) {
			tw_error(
				TW_LOC,
				"GVT computed %d times in a row"
				" without changing: GVT = %14.14lf, PREV %14.14lf"
				" -- GLOBAL SYNCH -- out of memory!",
				g_tw_gvt_no_change, gvt, me->GVT_prev);
		}
	}

	if (me->GVT > gvt)
	{
		tw_error(TW_LOC, "PE %u GVT decreased %g -> %g",
				me->id, me->GVT, gvt);
	}

	if (gvt / g_tw_ts_end > percent_complete && (g_tw_mynode == g_tw_masternode))
	{
		gvt_print(gvt);
	}

    if (g_tw_stats_enabled && gvt <= g_tw_ts_end)
    {
        tw_clock start_cycle_time = tw_clock_read();
        tw_statistics s;
        tw_get_stats(me, &s);
        stat_comp_cycle_counter += tw_clock_read() - start_cycle_time;
		tw_gvt_log(NULL, me, gvt, &s);
    }
    if (g_tw_time_interval)
    {
        // increment appropriate bin for gvt comp
        me->stats_tree_root = stat_increment(me->stats_tree_root, gvt, NUM_GVT, me->stats_tree_root, 1);
        me->stats_tree_root = stat_increment(me->stats_tree_root, gvt, NUM_ALLREDUCE, me->stats_tree_root, tmp_all_red_cnt);
        
        me->stats_tree_root = gvt_write_bins(NULL, me->stats_tree_root, gvt);
    }

	me->s_nwhite_sent = 0;
	me->s_nwhite_recv = 0;
	me->trans_msg_ts = DBL_MAX;
	me->GVT_prev = DBL_MAX; // me->GVT;
	me->GVT = gvt;
	me->gvt_status = TW_GVT_NORMAL;

	gvt_cnt = 0;

	// update GVT timing stats
	me->stats.s_gvt += tw_clock_read() - start;

	// only FC if OPTIMISTIC
	if( g_tw_synchronization_protocol == OPTIMISTIC )
	  {
	    start = tw_clock_read();
	    tw_pe_fossil_collect(me);
	    me->stats.s_fossil_collect += tw_clock_read() - start;
        if (g_tw_time_interval)
            me->stats_tree_root = stat_increment(me->stats_tree_root, gvt, FC_ATTEMPTS, me->stats_tree_root, 1);
	  }

    if (g_tw_real_time_samp)
        st_buffer_write(g_st_buffer, 0); 

	g_tw_gvt_done++;
}
