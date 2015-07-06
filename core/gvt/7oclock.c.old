#include <ross.h>

//#define VERIFY_GVT 1

#define TW_GVT_NORMAL		 0
#define TW_GVT_COMPUTE_LGVT	 1
#define TW_GVT_WAIT		 2
#define TW_GVT_COMPUTE		 3
#define TW_GVT_WAIT_REMOTE	 4
#define TW_MHZ 1000000

	/*
	 * GVT computation variables:
	 *
	 * g_tw_gvt_interval 	    -- how often we compute GVT
	 * g_tw_gvt_max_no_change   -- Maxmimum number of times we can compute
	 *                             GVT and not have it change (go up only).
	 * g_tw_gvt_no_change       -- Number of times in a row that GVT hasn't
	 *                             changed.
	 */
static unsigned int g_tw_gvt_interval = 16;
static unsigned int g_tw_gvt_max_no_change = 1000;
static unsigned int g_tw_gvt_no_change = 0;

static tw_mutex g_tw_gvt_lck = TW_MUTEX_INIT;
static tw_volatile tw_stime sim_gvt;
static tw_stime node_lvt;
static unsigned int gvt_cnt;
static tw_stime secs = 0.1;
static tw_stime delta_t = 0.01;
static unsigned int CPU = 1998;

static int TW_PARALLEL = 0;
static int TW_DISTRIBUTED = 0;

static const tw_optdef gvt_opts [] =
{
	TWOPT_GROUP("ROSS 7 O'clock GVT"),
	TWOPT_STIME("report-interval", gvt_print_interval, 
			"percent of runtime to print GVT"),
	TWOPT_STIME("delta_t", delta_t, "Max event send time (secs)"),
	TWOPT_STIME("clock-interval", secs, "GVT clock interval (in secs)"),
	TWOPT_UINT("clock-speed", CPU, "CPU clock speed (in MHz)"),
	TWOPT_UINT("gvt-interval", g_tw_gvt_interval, "GVT interval (shm only)"),
	TWOPT_END()
};

void
tw_gvt_stats(FILE * f)
{
	fprintf(f, "TW GVT Settings: Seven O'clock\n");
	fprintf(f, "\t%-50s %11.2lf (secs)\n", "GVT Clock Interval", secs);
	fprintf(f, "\t%-50s %11d\n", "GVT Interval", g_tw_gvt_interval);
	fprintf(f, "\t%-50s %11d\n", "Batch Size", g_tw_mblock);
	fprintf(f, "\n");
	fprintf(f, "TW GVT Statistics: Seven O'clock\n");
	fprintf(f, "\t%-50s %11d\n", "Total GVT Computations", g_tw_gvt_done);
}

static void
tw_gvt_compute(tw_pe * pe)
{
	tw_stime gvt_temp;
	tw_stime trans_temp;
	tw_pe *p;

	pe->LVT = min(tw_pq_minimum(pe->pq), pe->trans_msg_ts);

#if 0
	printf("%d pq %lf, trans %lf \n", pe->id, tw_pq_minimum(pe->pq), pe->trans_msg_ts);
#endif

	if (pe->LVT < sim_gvt)
		tw_error(
			TW_LOC,
			"%d: LVT < GVT!!! gvt: %f lvt: %f t_msg: %f",
			pe->id, sim_gvt, pe->LVT, pe->trans_msg_ts);

	tw_mutex_lock(&g_tw_gvt_lck);
	if (g_tw_7oclock_node_flag != 1) {
		g_tw_7oclock_node_flag--;
		tw_mutex_unlock(&g_tw_gvt_lck);
		pe->gvt_status = TW_GVT_WAIT_REMOTE;
		return;
	}

	/* Last to compute GVT for this node (LGVT) */
	gvt_temp = pe->LVT;
	trans_temp = pe->trans_msg_ts;

	p = NULL;
	while ((p = tw_pe_next(p)))
	{
		if (gvt_temp > p->LVT)
			gvt_temp = p->LVT;
		if (trans_temp > p->trans_msg_ts)
			trans_temp = p->trans_msg_ts;
	}

	if (trans_temp < gvt_temp)
		gvt_temp = trans_temp;

	if(TW_DISTRIBUTED) {
		node_lvt = gvt_temp;
		g_tw_7oclock_node_flag--;
	} else {
		g_tw_gvt_done++;
		sim_gvt = gvt_temp;
		g_tw_7oclock_node_flag = -1;
	}

	tw_mutex_unlock(&g_tw_gvt_lck);

#if VERIFY_GVT || 0
                if(!TW_DISTRIBUTED)
                {
                        printf("%d %d new GVT %f in gvt_compute\n",
                           pe->id, (int) *tw_net_onnode(pe->id), pe->GVT);
                } else
                        printf("%d %d new LVT %f in gvt_compute\n",
                           pe->id, (int) *tw_net_onnode(pe->id), pe->LVT);

		printf("%d %d sets gvt flag = %d in gvt_compute \n",
			pe->id, (int) *tw_net_onnode(pe->id), g_tw_7oclock_node_flag);
#endif

	/* Send LVT to master node on network */
	if (g_tw_mynode != g_tw_masternode) {
		tw_net_send_lvt(pe, node_lvt);
	}

	pe->gvt_status = TW_GVT_WAIT_REMOTE;
	pe->trans_msg_ts = DBL_MAX;

	if(TW_PARALLEL)
	{
		if (sim_gvt != pe->GVT) {
			g_tw_gvt_no_change = 0;
		} else {
			g_tw_gvt_no_change++;
			if (g_tw_gvt_no_change >= g_tw_gvt_max_no_change) {
				tw_error(
					TW_LOC,
					"GVT computed %d times in a row"
					" without changing: GVT = %g"
					" -- GLOBAL SYNCH -- out of memory!",
					g_tw_gvt_no_change, sim_gvt);
			}
		}

		pe->GVT = sim_gvt;

		if (sim_gvt > g_tw_ts_end)
			return;

		tw_pe_fossil_collect(pe);
		pe->gvt_status = TW_GVT_NORMAL;
		pe->trans_msg_ts = DBL_MAX;
	}
}

void
tw_gvt_wait(tw_pe * pe)
{
	/*
	 * If new GVT is available grab it and store in
	 * our PE.  Also decrement the global flag so that
	 * it is known that we have finished doing GVT work.
	 * Then cleanup our lists by doing a fossil collection.
	 */
	pe->GVT = sim_gvt;
	pe->trans_msg_ts = DBL_MAX;

	tw_mutex_lock(&g_tw_gvt_lck);
	g_tw_7oclock_node_flag--;
	tw_mutex_unlock(&g_tw_gvt_lck);

	tw_pe_fossil_collect(pe);
	pe->gvt_status = TW_GVT_NORMAL;

#if VERIFY_GVT
	printf("%d %d new GVT %lf (gvt_wait)\n",
		pe->id, (int) *tw_net_onnode(pe->id), sim_gvt);
	printf("%d %d set gvt_flag = %d in gvt_wait\n",
		pe->id, (int) *tw_net_onnode(pe->id), g_tw_7oclock_node_flag);
#endif
}

int
tw_gvt_set(tw_pe * pe, tw_stime GVT)
{
	tw_mutex_lock(&g_tw_gvt_lck);

	sim_gvt = GVT;
	g_tw_7oclock_node_flag--;

	tw_mutex_unlock(&g_tw_gvt_lck);

	if(0 == g_tw_mynode && 0 == pe->id && sim_gvt == pe->GVT)
	{
		g_tw_gvt_no_change++;

		if (g_tw_gvt_no_change >= g_tw_gvt_max_no_change)
		{
			tw_error(TW_LOC, "GVT computed %d times in a row"
					 " without changing: GVT = %g -- "
					 "GLOBAL SYNCH -- out of memory!\n",
					 g_tw_gvt_no_change, sim_gvt);
		}
	} else
	{
		g_tw_gvt_no_change = 0;
	}

	g_tw_gvt_done++;
	if(sim_gvt / g_tw_ts_end > percent_complete)
		gvt_print(sim_gvt);

	if (sim_gvt < pe->GVT)
		tw_error(TW_LOC, "GVT DECREASE from %f to %f\n", pe->GVT, sim_gvt);

	pe->GVT = sim_gvt;
	pe->gvt_status = TW_GVT_NORMAL;
	pe->trans_msg_ts = DBL_MAX;
	pe->s_ngvts = 0;

	tw_pe_fossil_collect(pe);

	return 0;
}

const tw_optdef *
tw_gvt_setup(void)
{
	return gvt_opts;
}

void
tw_gvt_start(void)
{
	if(tw_nnodes() > 1)
		TW_DISTRIBUTED = 1;
	else if(g_tw_npe > 1)
		TW_PARALLEL = 1;

	g_tw_7oclock_node_flag = -g_tw_npe;

	g_tw_clock_gvt_interval = 
		g_tw_clock_gvt_window_size = (tw_clock) CPU * TW_MHZ * secs;
	g_tw_clock_max_send_delta_t = (tw_clock) CPU * TW_MHZ * delta_t;
}

void
tw_gvt_force_update(tw_pe *me)
{
	if (tw_nnodes() > 1)
	{
	} else {
		gvt_cnt = g_tw_gvt_interval - 1;
	}
}

void
tw_gvt_step1(tw_pe * me)
{
	if(TW_PARALLEL)
	{
		if (me->master)
		{
			gvt_cnt++;
			if (gvt_cnt >= g_tw_gvt_interval &&
				me->gvt_status == TW_GVT_NORMAL)
			{
				gvt_cnt = 0;
				if (g_tw_7oclock_node_flag == -g_tw_npe)
				{
					tw_mutex_lock(&g_tw_gvt_lck);
					g_tw_7oclock_node_flag = g_tw_npe;
					tw_mutex_unlock(&g_tw_gvt_lck);
					me->gvt_status = TW_GVT_COMPUTE;
				}
			}

		} else if (g_tw_7oclock_node_flag > 0
			&& me->gvt_status == TW_GVT_NORMAL)
		{
			me->gvt_status = TW_GVT_COMPUTE;
		}

		return;
	}
	
	if(TW_DISTRIBUTED) 
	{
		if (me->gvt_status != TW_GVT_NORMAL)
			return;

		if (me->local_master)
		{
			/*
			 * start GVT in motion, all nodes see this simultaneously
			 * and make a consistent cut by setting the gvt flag 
			 * atomically over the network
			 */
			if (tw_clock_now(me) >= g_tw_clock_gvt_interval)
			{
				if (g_tw_7oclock_node_flag == -g_tw_npe)
				{
					gvt_cnt++;
					g_tw_clock_gvt_interval += g_tw_clock_gvt_window_size;

					g_tw_7oclock_node_flag = g_tw_npe;
					me->gvt_status = TW_GVT_COMPUTE_LGVT;

#if VERIFY_GVT || 0
					printf("\n\n\nGVT COUNT = %d (%d %d) \n",
						gvt_cnt, me->id, (int) *tw_net_onnode(me->id));
#endif
				}
			}
		} else
		{
			if (g_tw_7oclock_node_flag > 0 &&
				tw_clock_now(me) >= g_tw_clock_gvt_interval)
			{
				me->gvt_status = TW_GVT_COMPUTE_LGVT;
			}
		}
	}
}

void
tw_gvt_step2(tw_pe * me)
{
	if(TW_PARALLEL)
	{
		if (me->gvt_status == TW_GVT_COMPUTE)
			tw_gvt_compute(me);
		else if (me->gvt_status == TW_GVT_WAIT_REMOTE
				&& g_tw_7oclock_node_flag < 0)
			tw_gvt_wait(me);
		return;
	}

	/* Do we need to calculate our LVT for the GVT computation
	 * currently being performed?  If so, either calculate it 
	 * or check to see if the new GVT is ready yet.
	 */
	if (me->gvt_status == TW_GVT_COMPUTE_LGVT) {
		tw_gvt_compute(me);
		//return;
	}

	if (me->gvt_status == TW_GVT_WAIT_REMOTE) {
		/* When g_tw_7oclock_node_flag == 0,
		 * we have our node's new LVT
		 */
		if (g_tw_7oclock_node_flag == 0)
			tw_net_gvt_compute(me, &node_lvt);
		/* Still waiting for gvt */
		else if (g_tw_7oclock_node_flag < 0)
			tw_gvt_wait(me);
	}
}
