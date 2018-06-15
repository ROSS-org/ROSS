#include "ross.h"
#include <float.h>

static int limit_opt = LIMIT_OFF;
static pe_data last_check = {0};
int g_perf_disable_opt = 0;
static tw_stime max_diff_sim = 0.0;
static MPI_Op custom_opt_op;

// TODO remove later
extern MPI_Comm MPI_COMM_ROSS;

static const tw_optdef perf_options[] = {
    TWOPT_GROUP("ROSS Performance Optimizations"),
    TWOPT_UINT("disable-opt", g_perf_disable_opt, "disable performance optimizations when set to 1"),
    TWOPT_END()
};

const tw_optdef *perf_opts(void)
{
	return perf_options;
}

void perf_init()
{
    int i;

    last_check.nevent_processed = tw_calloc(TW_LOC, "performance optimizations", sizeof(tw_stat), g_tw_nkp);
    last_check.e_rbs = tw_calloc(TW_LOC, "performance optimizations", sizeof(tw_stat), g_tw_nkp);
    for (i = 0; i < g_tw_nkp; i++)
    {
        last_check.nevent_processed[i] = 0;
        last_check.e_rbs[i] = 0;
    }

	MPI_Op_create((MPI_User_function *)perf_custom_reduction, 1, &custom_opt_op);
}

// collect min, max, and sum for all KPs on this PE
// for both metrics, efficiency and difference between local clock and GVT
static void reduce_kp_data_local(tw_pe *pe, double *results, int len)
{
	int i;
	tw_kp *kp;
	tw_stime vt_diff, efficiency;
    tw_stat nevent_delta;
    tw_stat e_rbs_delta;
	
	results[0] = results[3] = DBL_MAX;
	results[1] = results[4] = -DBL_MAX;
	results[2] = results[5] = 0.0;

    for (i = 0; i < g_tw_nkp; i++)
    {
        kp = tw_getkp(i);

		// determine this KP's efficiency and diff between clock and GVT
        nevent_delta = kp->s_nevent_processed - last_check.nevent_processed[i];
        e_rbs_delta = kp->s_e_rbs - last_check.e_rbs[i];
        if (nevent_delta - e_rbs_delta != 0)
			efficiency = 100.0 * (1 - ((double)e_rbs_delta/(double)(nevent_delta - e_rbs_delta)));
        else
            efficiency = 0;

		if (efficiency < results[0])
			results[0] = efficiency;
		if (efficiency > results[1])
			results[1] = efficiency;
		results[2] += efficiency;

        vt_diff = kp->last_time - pe->GVT;

		if (vt_diff < results[3])
			results[3] = vt_diff;
		if (vt_diff > results[4])
			results[4] = vt_diff;
		results[5] += vt_diff;

        last_check.nevent_processed[i] = kp->s_nevent_processed;
        last_check.e_rbs[i] = kp->s_e_rbs;
    }

}

/* use Allreduce to exchange all of the relevant kp data to use for tuning
 * NOTE: This should only be called during GVT!
 */
void perf_adjust_optimism(tw_pe *pe)
{
    if (g_perf_disable_opt)
        return;

	int len = 6;
	double data[len], results[len];
	reduce_kp_data_local(pe, &data[0], len);

	//printf("PE %lu: data: %f, %f, %f, %f, %f, %f\n", pe->id, data[0], data[1], data[2], data[3], data[4], data[5]);
	MPI_Allreduce(&data, &results, len, MPI_DOUBLE, custom_opt_op, MPI_COMM_ROSS);
	//printf("PE %lu: results: %f, %f, %f, %f, %f, %f\n", pe->id, results[0], results[1], results[2], results[3], results[4], results[5]);

	/*** old stuff ***/
    ////double efficiency = perf_efficiency_check(pe);
    //tw_stime diff_ratio = perf_efficiency_check(pe);
    //if (diff_ratio > 0.95)
    //    diff_ratio = 0.95;
    ////int inc_amount = 1000 * efficiency;
    //int inc_amount = 1000 * (1-diff_ratio);
    ////double dec_amt = 1.0 - (abs(efficiency) / (abs(efficiency) + 1000.0));
    //double dec_amt = (1 - diff_ratio);
    //if (dec_amt < 0.05)
    //    dec_amt = 0.05;
    //else if (dec_amt > 0.95)
    //    dec_amt = 0.95;

    //if (limit_opt == LIMIT_ON)
    //{ // use multiplicative decrease to lower g_st_max_opt_lookahead
    //    g_tw_max_opt_lookahead *= dec_amt;
    //    if (g_tw_max_opt_lookahead < 100)
    //        g_tw_max_opt_lookahead = 100;
    //    //printf("ratio = %f, dec_amt = %f\n", diff_ratio, dec_amt);
    //    //printf("PE %ld decreasing opt lookahead to %llu\n", g_tw_mynode, g_tw_max_opt_lookahead);
    //}
    ////else if (limit_opt == LIMIT_RECOVER)
    ////{ // use additive increase to slowly allow for more optimism
    ////    if (ULLONG_MAX - g_tw_max_opt_lookahead < inc_amount) // avoid overflow
    ////        g_tw_max_opt_lookahead = ULLONG_MAX;
    ////    else 
    ////        g_tw_max_opt_lookahead += inc_amount;
    ////    //printf("ratio = %f, inc_amt = %d\n", diff_ratio, inc_amount);
    ////    //printf("PE %ld increasing opt lookahead to %llu\n", g_tw_mynode, g_tw_max_opt_lookahead);
    ////}
}

/* Expected format of invec/inoutvec
 * elements 0-2, efficiency values (min, max, sum)
 * elements 3-5, kp clock - GVT (min, max, sum)
 */
void perf_custom_reduction(double *invec, double *inoutvec, int *len, MPI_Datatype *dt)
{
	int i;
	for (i = 0; i < *len; i++)
	{
		if (i % 3 == 0) // find the min
			inoutvec[i] = ROSS_MIN(invec[i], inoutvec[i]);
		else if (i % 3 == 1) // find the max
			inoutvec[i] = ROSS_MAX(invec[i], inoutvec[i]);
		else if (i % 3 == 2) // find sum
			inoutvec[i] += invec[i];

	}
}

void perf_finalize()
{
	MPI_Op_free(&custom_opt_op);
}
