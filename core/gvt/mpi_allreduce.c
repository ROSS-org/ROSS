#include <ross.h>
#include <assert.h>

#define TW_GVT_NORMAL 0
#define TW_GVT_COMPUTE 1

static unsigned int g_tw_gvt_max_no_change = 10000;
static unsigned int g_tw_gvt_no_change = 0;
static tw_stat all_reduce_cnt = 0;
static unsigned int gvt_cnt = 0;
static unsigned int gvt_force = 0;
void (*g_tw_gvt_hook) (tw_pe * pe) = NULL;
// Holds one timestamp at which to trigger the arbitrary function
struct trigger_gvt_hook g_tw_trigger_gvt_hook = {.active = GVT_HOOK_disabled};

// MPI configuration parameters for tw_event_sig
#ifdef USE_RAND_TIEBREAKER
MPI_Datatype event_sig_type;
int event_sig_blocklengths[4] = {1, 1, MAX_TIE_CHAIN, 1};
MPI_Aint event_sig_displacements[4];
MPI_Datatype event_sig_types[4] = {MPI_DOUBLE, MPI_DOUBLE, MPI_DOUBLE, MPI_UNSIGNED};
MPI_Aint event_sig_base_address;
MPI_Op event_sig_min_op;
tw_event_sig dummy_event_sig;

void find_min_sig(void *in, void *inout, int *len, MPI_Datatype *datatype) {
    tw_event_sig *in_sig = (tw_event_sig *)in;
    tw_event_sig *inout_sig = (tw_event_sig *)inout;

    for (int i=0; i < *len; i++) {
        assert(in_sig->tie_lineage_length < MAX_TIE_CHAIN);
        assert(inout_sig->tie_lineage_length < MAX_TIE_CHAIN);
        if (tw_event_sig_compare(*in_sig, *inout_sig) < 0) {
            inout_sig->recv_ts = in_sig->recv_ts;
            inout_sig->priority = in_sig->priority;
            for (unsigned int j = 0; j < in_sig->tie_lineage_length; j++) {
                inout_sig->event_tiebreaker[j] = in_sig->event_tiebreaker[j];
            }
            inout_sig->tie_lineage_length = in_sig->tie_lineage_length;
        }
        in_sig++; inout_sig++;
    }
}
#endif

static const tw_optdef gvt_opts [] =
{
	TWOPT_GROUP("ROSS MPI GVT"),
	TWOPT_UINT("gvt-interval", g_tw_gvt_interval, "GVT Interval: Iterations through scheduling loop (synch=1,2,3,4), or ms between GVTs (synch=5)"),
	TWOPT_DOUBLE("report-interval", gvt_print_interval, "percent of runtime to print GVT"),
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
#ifdef USE_RAND_TIEBREAKER
    MPI_Get_address(&dummy_event_sig, &event_sig_base_address);
    MPI_Get_address(&dummy_event_sig.recv_ts, &event_sig_displacements[0]);
    MPI_Get_address(&dummy_event_sig.priority, &event_sig_displacements[1]);
    MPI_Get_address(&dummy_event_sig.event_tiebreaker, &event_sig_displacements[2]);
    MPI_Get_address(&dummy_event_sig.tie_lineage_length, &event_sig_displacements[3]);

    for (int i = 0; i < 4; i++) {
        event_sig_displacements[i] = MPI_Aint_diff(event_sig_displacements[i], event_sig_base_address);
    }

    MPI_Type_create_struct(4, event_sig_blocklengths, event_sig_displacements, event_sig_types, &event_sig_type);
    MPI_Type_commit(&event_sig_type);
    MPI_Op_create(find_min_sig, 1, &event_sig_min_op); // 1 means operation is commutative
#endif
}

void
tw_gvt_finish(void)
{
#ifdef USE_RAND_TIEBREAKER
    MPI_Op_free(&event_sig_min_op);
    MPI_Type_free(&event_sig_type);
#endif
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

// To use in `tw_gvt_step1` and `tw_gvt_step1_realtime`
#ifdef USE_RAND_TIEBREAKER
#define NOT_PAST_LOOKAHEAD(pe) (TW_STIME_DBL(tw_pq_minimum_sig(pe->pq).recv_ts) - TW_STIME_DBL(pe->GVT_sig.recv_ts) < g_tw_max_opt_lookahead)
#define NOT_PAST_GVT_HOOK_ACTIVATION(pe) (g_tw_trigger_gvt_hook.active == GVT_HOOK_disabled \
        || tw_event_sig_compare(tw_pq_minimum_sig(pe->pq), g_tw_trigger_gvt_hook.sig_at) < 0)
#else
#define NOT_PAST_LOOKAHEAD(pe) (TW_STIME_DBL(tw_pq_minimum(pe->pq)) - TW_STIME_DBL(pe->GVT) < g_tw_max_opt_lookahead)
#define NOT_PAST_GVT_HOOK_ACTIVATION(pe) (g_tw_trigger_gvt_hook.active == GVT_HOOK_disabled \
        || tw_pq_minimum(me->pq) < g_tw_trigger_gvt_hook.at)
#endif

void
tw_gvt_step1(tw_pe *me)
{
	if (me->gvt_status == TW_GVT_COMPUTE) {
        return;
    }

    int const still_within_interval = ++gvt_cnt < g_tw_gvt_interval;
    if (still_within_interval && NOT_PAST_LOOKAHEAD(me) && NOT_PAST_GVT_HOOK_ACTIVATION(me)) {
        return;
    }

	me->gvt_status = TW_GVT_COMPUTE;
}

void
tw_gvt_step1_realtime(tw_pe *me)
{
	if (me->gvt_status == TW_GVT_COMPUTE) {
        return;
    }
    int const still_within_interval = tw_clock_read() - g_tw_gvt_interval_start_cycles < g_tw_gvt_realtime_interval;
    if (still_within_interval && NOT_PAST_LOOKAHEAD(me) && NOT_PAST_GVT_HOOK_ACTIVATION(me)) {
        return;
    }

    me->gvt_status = TW_GVT_COMPUTE;
}

#ifdef USE_RAND_TIEBREAKER
//This function had so many interweavings of USE_RAND_TIEBREAKER that it was simpler to duplicate the function
void
tw_gvt_step2(tw_pe *me)
{
	if(me->gvt_status != TW_GVT_COMPUTE)
		return;

	long long local_white = 0;
	long long total_white = 0;

	tw_event_sig pq_min_sig = (tw_event_sig){TW_STIME_MAX, TW_STIME_MAX};
	tw_event_sig net_min_sig = (tw_event_sig){TW_STIME_MAX, TW_STIME_MAX};

	tw_event_sig lvt_sig;
	tw_event_sig gvt_sig;

    tw_clock net_start;
	tw_clock start = tw_clock_read();

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
		&lvt_sig.recv_ts,
		&gvt_sig.recv_ts,
		1,
		event_sig_type,
		event_sig_min_op,
		MPI_COMM_ROSS
	) != MPI_SUCCESS) {
		tw_error(TW_LOC, "MPI_Allreduce for GVT event signatures failed");
	}

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
		tw_error(TW_LOC, "PE %u GVT decreased %g -> %g",
				me->id, me->GVT_sig.recv_ts, gvt_sig.recv_ts);

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


#ifdef USE_RAND_TIEBREAKER
void tw_trigger_gvt_hook_at(tw_stime time) {
    tw_event_sig now = g_tw_pe->GVT_sig;
    tw_event_sig time_sig = {
        .recv_ts = time,
        .priority = 1.0,
        .event_tiebreaker = {0.0},
        .tie_lineage_length = 1};

    if (now.recv_ts >= time) {
        tw_warning(TW_LOC, "Trying to schedule arbitrary function trigger at a time in the past %e, current GVT %e\n", time, now);
    }

    g_tw_trigger_gvt_hook.sig_at = time_sig;
    g_tw_trigger_gvt_hook.active = GVT_HOOK_enabled;
}
#else
void tw_trigger_gvt_hook_at(tw_stime time) {
    tw_stime now = g_tw_pe->GVT;

    if (now >= time) {
        tw_warning(TW_LOC, "Trying to schedule arbitrary function trigger at a time in the past %e, current GVT %e\n", time, now);
    }

    g_tw_trigger_gvt_hook.at = time;
    g_tw_trigger_gvt_hook.active = GVT_HOOK_enabled;
}
#endif

#ifdef USE_RAND_TIEBREAKER
void tw_trigger_gvt_hook_at_event_sig(tw_event_sig time) {
    tw_event_sig now = g_tw_pe->GVT_sig;

    if (tw_event_sig_compare(now, time) >= 0) {
        tw_warning(TW_LOC, "Trying to schedule arbitrary function trigger at a time in the past %e, current GVT %e\n", time.recv_ts, now.recv_ts);
    }

    g_tw_trigger_gvt_hook.sig_at = time;
    //g_tw_trigger_gvt_hook.at = time;
    g_tw_trigger_gvt_hook.active = GVT_HOOK_enabled;
}
#endif
