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
tw_gvt_force_update_realtime(tw_pe *me)
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
	if(me->gvt_status == TW_GVT_COMPUTE ||
		(++gvt_cnt < g_tw_gvt_interval && (tw_pq_minimum(me->pq) - me->GVT < g_tw_max_opt_lookahead)))
		return;

	me->gvt_status = TW_GVT_COMPUTE;
}

void
tw_gvt_step1_realtime(tw_pe *me)
{
  unsigned long long current_rt;

  if( (me->gvt_status == TW_GVT_COMPUTE) ||
      ( ((current_rt = tw_clock_read()) - g_tw_gvt_interval_start_cycles < g_tw_gvt_realtime_interval)
          && (tw_pq_minimum(me->pq) - me->GVT < g_tw_max_opt_lookahead)))
    {
      /* if( me->node == 0 ) */
      /* 	{ */
      /* 	  printf("GVT Step 1 RT Rank %ld: found start_cycles at %llu, rt interval at %llu, current time at %llu \n", */
      /* 		 me->node, g_tw_gvt_interval_start_cycles, g_tw_gvt_realtime_interval, current_rt); */

      /* 	} */

    return;
    }

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
	while(1)
	  {
	    tw_net_read(me);

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
	net_min = tw_net_minimum(me);

	lvt = me->trans_msg_ts;
	if(lvt > pq_min)
	  lvt = pq_min;
	if(lvt > net_min)
		lvt = net_min;

	all_reduce_cnt++;
	if(MPI_Allreduce(
			&lvt,
			&gvt,
			1,
			MPI_DOUBLE,
			MPI_MIN,
			MPI_COMM_ROSS) != MPI_SUCCESS)
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

	me->s_nwhite_sent = 0;
	me->s_nwhite_recv = 0;
	me->trans_msg_ts = DBL_MAX;
	me->GVT_prev = DBL_MAX; // me->GVT;
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
	    tw_pe_fossil_collect(me);
	    me->stats.s_fossil_collect += tw_clock_read() - start;
	  }

    if ((g_st_engine_stats == GVT_STATS || g_st_engine_stats == BOTH_STATS) && 
            g_tw_gvt_done % g_st_num_gvt == 0 && gvt <= g_tw_ts_end)
    {
        tw_clock start_cycle_time = tw_clock_read();
        tw_statistics s;
        bzero(&s, sizeof(s));
        tw_get_stats(me, &s);
		st_gvt_log(me, gvt, &s, all_reduce_cnt);
        g_st_stat_comp_ctr += tw_clock_read() - start_cycle_time;
    }
    if ((g_st_model_stats == GVT_STATS || g_st_model_stats == BOTH_STATS) && g_tw_gvt_done % g_st_num_gvt == 0)
        st_collect_model_data(me, (tw_stime)tw_clock_read() / g_tw_clock_rate, GVT_STATS);
    
    st_inst_dump();

	g_tw_gvt_done++;

	// reset for the next gvt round -- for use in realtime GVT mode only!!
	g_tw_gvt_interval_start_cycles = tw_clock_read();
 }
