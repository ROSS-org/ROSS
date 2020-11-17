#include <ross.h>

#define TW_GVT_NORMAL 0
#define TW_GVT_COMPUTE 1

static unsigned int g_tw_gvt_max_no_change = 10000;
static unsigned int g_tw_gvt_no_change = 0;
static tw_stat all_reduce_cnt = 0;
static unsigned int gvt_cnt = 0;
static unsigned int gvt_force = 0;

static const tw_optdef gvt_opts [] =
{
	TWOPT_GROUP("ROSS MPI GVT"),
	TWOPT_UINT("gvt-interval", g_tw_gvt_interval, "GVT Interval: Iterations through scheduling loop (synch=1,2,3,4), or ms between GVTs (synch=5)"),
	TWOPT_DOUBLE("report-interval", gvt_print_interval,
			"percent of runtime to print GVT"),
	TWOPT_END()
};

tw_stat st_get_allreduce_count()
{
    return all_reduce_cnt;
}

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
tw_gvt_force_update(void)
{
	gvt_force++;
	gvt_cnt = g_tw_gvt_interval;
}

void
tw_gvt_force_update_realtime(void)
{
	gvt_force++;
        g_tw_gvt_interval_start_cycles = 0; // reset to start of time
}

void
tw_gvt_stats(FILE * f)
{
	fprintf(f, "\nTW GVT Statistics: MPI AllReduce\n");
	fprintf(f, "\t%-50s %11d\n", "GVT Interval", g_tw_gvt_interval);
	fprintf(f, "\t%-50s %llu\n", "GVT Real Time Interval (cycles)", g_tw_gvt_realtime_interval);
	fprintf(f, "\t%-50s %11.8lf\n", "GVT Real Time Interval (sec)", (double)g_tw_gvt_realtime_interval/(double)g_tw_clock_rate);
	fprintf(f, "\t%-50s %11d\n", "Batch Size", g_tw_mblock);
	fprintf(f, "\n");
	fprintf(f, "\t%-50s %11d\n", "Forced GVT", gvt_force);
	fprintf(f, "\t%-50s %11d\n", "Total GVT Computations", g_tw_gvt_done);
	fprintf(f, "\t%-50s %11lld\n", "Total All Reduce Calls", all_reduce_cnt);
	fprintf(f, "\t%-50s %11.2lf\n", "Average Reduction / GVT",
			(double) ((double) all_reduce_cnt / (double) g_tw_gvt_done));
}

void
tw_gvt_step1(tw_pe *me)
{
	// printf("%d\n",g_tw_max_opt_lookahead);
	if(me->gvt_status == TW_GVT_COMPUTE ||
#ifdef USE_RAND_TIEBREAKER
           (++gvt_cnt < g_tw_gvt_interval && (TW_STIME_DBL(tw_pq_minimum_sig(me->pq).recv_ts) - TW_STIME_DBL(me->GVT_sig.recv_ts) < g_tw_max_opt_lookahead)))
#else
           (++gvt_cnt < g_tw_gvt_interval && (TW_STIME_DBL(tw_pq_minimum(me->pq)) - TW_STIME_DBL(me->GVT) < g_tw_max_opt_lookahead)))
#endif
		return;

	me->gvt_status = TW_GVT_COMPUTE;
}

void
tw_gvt_step1_realtime(tw_pe *me)
{
  unsigned long long current_rt;

  if( (me->gvt_status == TW_GVT_COMPUTE) ||
      ( ((current_rt = tw_clock_read()) - g_tw_gvt_interval_start_cycles < g_tw_gvt_realtime_interval)
#ifdef USE_RAND_TIEBREAKER
          && (TW_STIME_DBL(tw_pq_minimum_sig(me->pq).recv_ts) - TW_STIME_DBL(me->GVT_sig.recv_ts) < g_tw_max_opt_lookahead)))
#else
          && (TW_STIME_DBL(tw_pq_minimum(me->pq)) - TW_STIME_DBL(me->GVT) < g_tw_max_opt_lookahead)))
#endif
    {
      /* if( me->id == 0 ) */
      /* 	{ */
      /* 	  printf("GVT Step 1 RT Rank %ld: found start_cycles at %llu, rt interval at %llu, current time at %llu \n", */
      /* 		 me->id, g_tw_gvt_interval_start_cycles, g_tw_gvt_realtime_interval, current_rt); */

      /* 	} */

    return;
    }

  me->gvt_status = TW_GVT_COMPUTE;
}

#ifdef USE_RAND_TIEBREAKER
//This function had so many interweavings of USE_RAND_TIEBREAKER that it was simpler to duplicate the function
void
tw_gvt_step2(tw_pe *me)
{
	long long local_white = 0;
	long long total_white = 0;

	tw_event_sig pq_min_sig = (tw_event_sig){TW_STIME_MAX, TW_STIME_MAX};
	tw_event_sig net_min_sig = (tw_event_sig){TW_STIME_MAX, TW_STIME_MAX};

	tw_event_sig lvt_sig;
	tw_event_sig gvt_sig;

    tw_clock net_start;
	tw_clock start = tw_clock_read();

	if(me->gvt_status != TW_GVT_COMPUTE)
		return;
	while(1)
	  {
        net_start = tw_clock_read();
	    tw_net_read(me);
        me->stats.s_net_read += tw_clock_read() - net_start;

	    // send message counts to create consistent cut
	    local_white = me->s_nwhite_sent - me->s_nwhite_recv;
	    all_reduce_cnt++;
	    if(MPI_Allreduce(
			     &local_white,
			     &total_white,
			     1,
			     MPI_LONG_LONG,
			     MPI_SUM,
			     MPI_COMM_ROSS) != MPI_SUCCESS)
	      tw_error(TW_LOC, "MPI_Allreduce for GVT failed");

	    if(total_white == 0)
	      break;
	  }
	pq_min_sig = tw_pq_minimum_sig(me->pq);
	net_min_sig = tw_net_minimum_sig();

	lvt_sig = me->trans_msg_sig;
	if(tw_event_sig_compare(lvt_sig, pq_min_sig) > 0)
	{
	  lvt_sig = pq_min_sig;
	}
	if(tw_event_sig_compare(lvt_sig, net_min_sig) > 0)
	{
		lvt_sig = net_min_sig;
	}

	all_reduce_cnt++;
	if(MPI_Allreduce(
		&lvt_sig,
		&gvt_sig,
		1,
		MPI_TYPE_TW_STIME,
		MPI_MIN,
		MPI_COMM_ROSS) != MPI_SUCCESS)
		tw_error(TW_LOC, "MPI_Allreduce for GVT event signatures failed");

	gvt_sig.event_tiebreaker = 0;

	if(tw_event_sig_compare(gvt_sig, me->GVT_prev_sig) < 0)
	{
		g_tw_gvt_no_change = 0;
	} else
	{
				gvt_sig = me->GVT_prev_sig;
		g_tw_gvt_no_change++;
		if (g_tw_gvt_no_change >= g_tw_gvt_max_no_change) {
			tw_error(
				TW_LOC,
				"GVT computed %d times in a row"
				" without changing: GVT = %14.14lf, PREV %14.14lf"
				" -- GLOBAL SYNCH -- out of memory!",
				g_tw_gvt_no_change, gvt_sig.recv_ts, me->GVT_prev_sig.recv_ts);
		}
	}

	if (tw_event_sig_compare(me->GVT_sig, gvt_sig) > 0)
	{
		tw_error(TW_LOC, "PE %u GVT decreased %g  %g  -> %g  %g",
				me->id, me->GVT_sig.recv_ts, me->GVT_sig.event_tiebreaker, gvt_sig.recv_ts, gvt_sig.event_tiebreaker);
	}

	if (TW_STIME_DBL(gvt_sig.recv_ts) / g_tw_ts_end > percent_complete && (g_tw_mynode == g_tw_masternode))
	{
		gvt_print(gvt_sig.recv_ts);
	}

	me->s_nwhite_sent = 0;
	me->s_nwhite_recv = 0;
	me->trans_msg_sig = (tw_event_sig){TW_STIME_MAX, TW_STIME_MAX};
	me->GVT_prev_sig = (tw_event_sig){TW_STIME_MAX, TW_STIME_MAX};
	me->GVT_sig = gvt_sig;
	me->gvt_status = TW_GVT_NORMAL;

	gvt_cnt = 0;

	// update GVT timing stats
	me->stats.s_gvt += tw_clock_read() - start;

	// only FC if OPTIMISTIC or REALTIME, do not do for DEBUG MODE
	if( g_tw_synchronization_protocol == OPTIMISTIC ||
	    g_tw_synchronization_protocol == OPTIMISTIC_REALTIME )
	  {
	    start = tw_clock_read();
	    tw_pe_fossil_collect();
	    me->stats.s_fossil_collect += tw_clock_read() - start;
	  }

    // do any necessary instrumentation calls
    if ((g_st_engine_stats == GVT_STATS || g_st_engine_stats == ALL_STATS) &&
        g_tw_gvt_done % g_st_num_gvt == 0 && TW_STIME_DBL(gvt_sig.recv_ts) <= g_tw_ts_end)
    {
#ifdef USE_DAMARIS
        if (g_st_damaris_enabled)
        {
            st_damaris_expose_data(me, gvt, GVT_COL);
            st_damaris_end_iteration();
        }
        else
            st_collect_engine_data(me, GVT_COL);
#else
		st_collect_engine_data(me, GVT_COL);
#endif
    }
#ifdef USE_DAMARIS
    // need to make sure damaris_end_iteration is called if GVT instrumentation not turned on
    //if (!g_st_stats_enabled && g_st_real_time_samp) //need to make sure if one PE enters this, all do; otherwise deadlock
    if (g_st_damaris_enabled && (g_st_engine_stats == RT_STATS || g_st_engine_stats == VT_STATS))
    {
        st_damaris_end_iteration();
    }
#endif

    if ((g_st_model_stats == GVT_STATS || g_st_model_stats == ALL_STATS) && g_tw_gvt_done % g_st_num_gvt == 0)
        st_collect_model_data(me, ((double)tw_clock_read()) / g_tw_clock_rate, GVT_STATS);

    st_inst_dump();
    // done with instrumentation related stuff

	g_tw_gvt_done++;

	// reset for the next gvt round -- for use in realtime GVT mode only!!
	g_tw_gvt_interval_start_cycles = tw_clock_read();
 }
#else
void
tw_gvt_step2(tw_pe *me)
{
	long long local_white = 0;
	long long total_white = 0;

	tw_stime pq_min = TW_STIME_MAX;
	tw_stime net_min = TW_STIME_MAX;

	tw_stime lvt;
	tw_stime gvt;

    tw_clock net_start;
	tw_clock start = tw_clock_read();

	if(me->gvt_status != TW_GVT_COMPUTE)
		return;
	while(1)
	  {
        net_start = tw_clock_read();
	    tw_net_read(me);
        me->stats.s_net_read += tw_clock_read() - net_start;

	    // send message counts to create consistent cut
	    local_white = me->s_nwhite_sent - me->s_nwhite_recv;
	    all_reduce_cnt++;
	    if(MPI_Allreduce(
			     &local_white,
			     &total_white,
			     1,
			     MPI_LONG_LONG,
			     MPI_SUM,
			     MPI_COMM_ROSS) != MPI_SUCCESS)
	      tw_error(TW_LOC, "MPI_Allreduce for GVT failed");

	    if(total_white == 0)
	      break;
	  }

	pq_min = tw_pq_minimum(me->pq);
	net_min = tw_net_minimum();

	lvt = me->trans_msg_ts;
	if(TW_STIME_CMP(lvt, pq_min) > 0)
	  lvt = pq_min;
	if(TW_STIME_CMP(lvt, net_min) > 0)
		lvt = net_min;

	all_reduce_cnt++;

	if(MPI_Allreduce(
			&lvt,
			&gvt,
			1,
			MPI_TYPE_TW_STIME,
			MPI_MIN,
			MPI_COMM_ROSS) != MPI_SUCCESS)
			tw_error(TW_LOC, "MPI_Allreduce for GVT failed");

	if(TW_STIME_CMP(gvt, me->GVT_prev) < 0)
	{
		g_tw_gvt_no_change = 0;
	} else
	{
                gvt = me->GVT_prev;
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

	if (TW_STIME_CMP(me->GVT, gvt) > 0)
	{
		tw_error(TW_LOC, "PE %u GVT decreased %g -> %g",
				me->id, me->GVT, gvt);
	}

	if (TW_STIME_DBL(gvt) / g_tw_ts_end > percent_complete && (g_tw_mynode == g_tw_masternode))
	{
		gvt_print(gvt);
	}

	me->s_nwhite_sent = 0;
	me->s_nwhite_recv = 0;
	me->trans_msg_ts = TW_STIME_MAX;
	me->GVT_prev = TW_STIME_MAX; // me->GVT;
	me->GVT = gvt;
	me->gvt_status = TW_GVT_NORMAL;

	gvt_cnt = 0;

	// update GVT timing stats
	me->stats.s_gvt += tw_clock_read() - start;

	// only FC if OPTIMISTIC or REALTIME, do not do for DEBUG MODE
	if( g_tw_synchronization_protocol == OPTIMISTIC ||
	    g_tw_synchronization_protocol == OPTIMISTIC_REALTIME )
	  {
	    start = tw_clock_read();
	    tw_pe_fossil_collect();
	    me->stats.s_fossil_collect += tw_clock_read() - start;
	  }

    // do any necessary instrumentation calls
    if ((g_st_engine_stats == GVT_STATS || g_st_engine_stats == ALL_STATS) &&
        g_tw_gvt_done % g_st_num_gvt == 0 && TW_STIME_DBL(gvt) <= g_tw_ts_end)
    {
#ifdef USE_DAMARIS
        if (g_st_damaris_enabled)
        {
            st_damaris_expose_data(me, gvt, GVT_COL);
            st_damaris_end_iteration();
        }
        else
            st_collect_engine_data(me, GVT_COL);
#else
		st_collect_engine_data(me, GVT_COL);
#endif
    }
#ifdef USE_DAMARIS
    // need to make sure damaris_end_iteration is called if GVT instrumentation not turned on
    //if (!g_st_stats_enabled && g_st_real_time_samp) //need to make sure if one PE enters this, all do; otherwise deadlock
    if (g_st_damaris_enabled && (g_st_engine_stats == RT_STATS || g_st_engine_stats == VT_STATS))
    {
        st_damaris_end_iteration();
    }
#endif

    if ((g_st_model_stats == GVT_STATS || g_st_model_stats == ALL_STATS) && g_tw_gvt_done % g_st_num_gvt == 0)
        st_collect_model_data(me, ((double)tw_clock_read()) / g_tw_clock_rate, GVT_STATS);

    st_inst_dump();
    // done with instrumentation related stuff

	g_tw_gvt_done++;

	// reset for the next gvt round -- for use in realtime GVT mode only!!
	g_tw_gvt_interval_start_cycles = tw_clock_read();
 }
#endif