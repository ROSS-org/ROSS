#ifndef INC_gvt_7oclock_h
#define INC_gvt_7oclock_h

	/* Clock Computation Variables:
	 *
	 * The clock is used to implement the 7 O'Clock Algorithm, but
	 * is useful in other areas, such as determining how long it takes to
	 * actually complete tasks, such as enq's and deq's.
	 */
static tw_volatile int g_tw_7oclock_node_flag;
static tw_volatile tw_clock g_tw_clock_max_send_delta_t;
static tw_volatile tw_clock g_tw_clock_gvt_interval;
static tw_volatile tw_clock g_tw_clock_gvt_window_size;

static tw_stime gvt_print_interval = 0.1;
static tw_stime percent_complete = 0.0;

static inline int 
tw_gvt_inprogress(tw_pe * pe)
{
#if 0
	return (g_tw_7oclock_node_flag == -g_tw_npe && 
		tw_clock_now(pe) < g_tw_clock_gvt_interval ? 0 : 1);
#endif
	return (g_tw_7oclock_node_flag >= 0 || 
		tw_clock_now(pe) + g_tw_clock_max_send_delta_t >= g_tw_clock_gvt_interval);
}

static inline void 
gvt_print(tw_stime gvt)
{
	if(gvt_print_interval > 1.0)
		return;

	if(percent_complete == 0.0)
	{
		percent_complete = gvt_print_interval;
		return;
	}

	printf("GVT #%d: simulation %d%% complete (",
		g_tw_gvt_done,
		(int) min(100, floor(100 * (gvt/g_tw_ts_end))));

	if (gvt == DBL_MAX)
		printf("GVT = %s", "MAX");
	else
		printf("GVT = %.4f", gvt);

	printf(").\n");
	percent_complete += gvt_print_interval;
}

#endif
