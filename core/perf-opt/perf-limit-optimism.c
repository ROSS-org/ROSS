#include "ross.h"
#include <float.h>

static int limit_opt = LIMIT_OFF;
static pe_data last_check = {0};
int g_perf_disable_opt = 0;
static tw_stime max_diff_sim = 0.0;
static MPI_Op custom_opt_op;
static int decrease = 1;
static int rest_cycles = 0;
static int max_rest = 10;
static int beginning_wait = 200;
static tw_stime *vt_diff_ema;
static int first_iteration = 1;
static double alpha = 0.25;

static void test_method2(tw_pe *pe, double *global_data, int len);

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

    vt_diff_ema = tw_calloc(TW_LOC, "performance optimizations", sizeof(tw_stime), g_tw_nkp);
    for (i = 0; i < g_tw_nkp; i++)
		vt_diff_ema[i] = 0;

	MPI_Op_create((MPI_User_function *)perf_custom_reduction, 1, &custom_opt_op);
}

// collect min, max, and sum for all KPs on this PE
// for both metrics, efficiency and difference between local clock and GVT
static void reduce_kp_data_local(tw_pe *pe, double *results, int len)
{
	int i;
	tw_kp *kp;
	tw_stime vt_diff;
	double mean[len];
	
	results[TIME_MAX] = -DBL_MAX;

    for (i = 0; i < g_tw_nkp; i++)
    {
        kp = tw_getkp(i);

        vt_diff = kp->last_time - pe->GVT;
		if (first_iteration)
			vt_diff_ema[i] = vt_diff;
		else
			vt_diff_ema[i] = alpha * vt_diff + (1.0 - alpha) * vt_diff_ema[i];

		if (vt_diff_ema[i] > results[TIME_MAX])
			results[TIME_MAX] = vt_diff_ema[i];
    }

	first_iteration = 0;
}

/* use Allreduce to exchange all of the relevant kp data to use for tuning
 * NOTE: This should only be called during GVT!
 */
void perf_adjust_optimism(tw_pe *pe)
{
    if (g_perf_disable_opt)
        return;

	int len = NUM_RED_ELEM, i;
	double local_data[len], global_data[len], std_dev_local[2], std_dev_global[2];
	tw_kp *kp;

	reduce_kp_data_local(pe, &local_data[0], len);
	local_data[TIME_MIN_MAX] = local_data[TIME_MAX];
	int num_pe = tw_nnodes();
	double all_max[num_pe];

	//MPI_Allreduce(&local_data, &global_data, len, MPI_DOUBLE, custom_opt_op, MPI_COMM_ROSS);
	// actually should switch to all gather
	MPI_Allgather(&local_data, 1, MPI_DOUBLE, &all_max[0], 1, MPI_DOUBLE, MPI_COMM_ROSS);


	//test_method1();
	test_method2(pe, &all_max[0], num_pe);

}

static void test_method2(tw_pe *pe, double *global_data, int len)
{
	int i, found = 0;
	tw_stime min = DBL_MAX;
	tw_stime max = -DBL_MAX;
	tw_clock window_buffer;

	if (!(beginning_wait-- > 0))
	{
		// find the min
		for (i = 0; i < len; i++)
		{
			if (global_data[i] < min  && global_data[i] > 0)
				min = global_data[i];
			if (global_data[i] > max  && global_data[i] > 0)
				max = global_data[i];
		}

		if (g_tw_mynode == g_tw_masternode)
		{
			if (max == -DBL_MAX || min == DBL_MAX)
			{
				printf("No positive max or min value found: ");
				for (i = 0; i < len; i++)
					printf("%f, ", global_data[i]);
			}
			//printf("PE %ld, min = %f, max = %f\n", g_tw_mynode, min, max);
		}
		if (!decrease && rest_cycles > max_rest)
		{ 
			// test to see if we should increase or decrease
			window_buffer = g_tw_max_opt_lookahead * 0.75;
			//printf("PE %ld: decrease = %d, window_buffer = %lu, g_tw_max_opt_lookahead = %llu \n", 
					//g_tw_mynode, decrease, window_buffer, g_tw_max_opt_lookahead);

			for (i = 0; i < len; i++)
			{
				if (global_data[i] < pe->GVT + window_buffer && global_data[i] > 0)
				{
					if (g_tw_mynode == g_tw_masternode)
						printf("PE %ld: found max less than window_buffer %lu lookahead: %llu\n", g_tw_mynode, window_buffer, g_tw_max_opt_lookahead);
					found = 1;
					break;
				}
			}

			if (found) // now we need to decrease
			{
				decrease = 1;
			}
			else // we can increase
			{
				printf("PE %ld is increasing g_tw_max_opt_lookahead %llu\n", g_tw_mynode, g_tw_max_opt_lookahead);
				g_tw_max_opt_lookahead += window_buffer;
				printf("PE %ld increased g_tw_max_opt_lookahead %llu\n", g_tw_mynode, g_tw_max_opt_lookahead);
			}
			rest_cycles = 0;
		}

		// first test if we need to decrease
		if (decrease)
		{
			printf("PE %ld is about to decrease g_tw_max_opt_lookahead %llu\n", g_tw_mynode, g_tw_max_opt_lookahead);
			g_tw_max_opt_lookahead = ceil(min) * 1.25;
			decrease = 0;
			printf("PE %ld decreased g_tw_max_opt_lookahead %llu\n", g_tw_mynode, g_tw_max_opt_lookahead);
		}
		rest_cycles++;
	}

	if (g_tw_max_opt_lookahead < 10)
		g_tw_max_opt_lookahead = 10;
}

//static void test_method1()
//{
//	//printf("PE %lu: std_dev_local: %f, %f\n", pe->id, std_dev_local[0], std_dev_local[1]);
//	MPI_Allreduce(&std_dev_local, &std_dev_global, 2, MPI_DOUBLE, MPI_SUM, MPI_COMM_ROSS);
//	std_dev_global[0] = sqrt(std_dev_global[0] / (g_tw_nkp * tw_nnodes()));
//	std_dev_global[1] = sqrt(std_dev_global[1] / (g_tw_nkp * tw_nnodes()));
//	//printf("PE %lu: std_dev_global: %f, %f\n", pe->id, std_dev_global[0], std_dev_global[1]);
//
//	// if global max is < 0, then no KP has processed anything since GVT...
//	// this shouldn't happen, right?
//	
//	// for PEs with local max local clock difference < 0, decrease the lookahead
//	// because this PE has a light load and we want to preemptively keep it from getting too far ahead of GVT
//	if (data[TIME_MAX] < 0)
//	{
//		g_tw_max_opt_lookahead = 100;
//		//printf("PE %ld decreasing opt lookahead to %llu\n", g_tw_mynode, g_tw_max_opt_lookahead);
//	}
//	else
//	{
//		// are we ahead or behind the mean?
//		if (data[TIME_MAX] <= global_data[TIME_MEAN])
//		{
//			// if we're less than the mean, let's increase our lookahead
//			if (std_dev_global[1] > 1)
//				inc_amt *= std_dev_global[1];
//
//			if (ULLONG_MAX - g_tw_max_opt_lookahead  < inc_amt) // avoid overflow
//				g_tw_max_opt_lookahead = ULLONG_MAX;
//			else
//				g_tw_max_opt_lookahead += inc_amt;
//			//printf("PE %ld increasing opt lookahead to %llu\n", g_tw_mynode, g_tw_max_opt_lookahead);
//		}
//		else
//		{
//			// increase our lookahead if we're within one standard dev of global mean
//			if (data[TIME_MAX] < global_data[TIME_MEAN] + std_dev_global[1])
//			{
//				if (ULLONG_MAX - g_tw_max_opt_lookahead  < inc_amt) // avoid overflow
//					g_tw_max_opt_lookahead = ULLONG_MAX;
//				else
//					g_tw_max_opt_lookahead += inc_amt;
//				//printf("PE %ld increasing opt lookahead to %llu\n", g_tw_mynode, g_tw_max_opt_lookahead);
//			}
//			else // we're somewhere between mean + stddev and global max
//			{
//				// so we want to decrease our lookahead
//				dec_amt *= (data[TIME_MAX] - (global_data[TIME_MEAN] + std_dev_global[1])) / 
//					(double)(global_data[TIME_MAX] - (global_data[TIME_MEAN] + std_dev_global[1]));
//				if (dec_amt < 0.02)
//					dec_amt = 0.02;
//				else if (dec_amt > 0.98)
//					dec_amt = 0.98;
//				g_tw_max_opt_lookahead *= dec_amt;
//				//printf("PE %ld decreasing opt lookahead to %llu\n", g_tw_mynode, g_tw_max_opt_lookahead);
//			}
//
//		}
//
//	}
//	//printf("PE %ld lookahead: %llu\n", g_tw_mynode, g_tw_max_opt_lookahead);
//	if (g_tw_max_opt_lookahead < 50)
//		g_tw_max_opt_lookahead = 50;
//
//}

/* Expected format of invec/inoutvec
 * elements 0-2, efficiency values (min, max, sum)
 * elements 3-5, kp clock - GVT (min, max, sum)
 */
void perf_custom_reduction(double *invec, double *inoutvec, int *len, MPI_Datatype *dt)
{
	int i;
	for (i = 0; i < *len; i++)
	{
		if (i % 4 == 0) // find the min
			inoutvec[i] = ROSS_MIN(invec[i], inoutvec[i]);
		else if (i % 4 == 1) // find the max
			inoutvec[i] = ROSS_MAX(invec[i], inoutvec[i]);
		else if (i % 4 == 2) // find the min (of the max)
			inoutvec[i] = ROSS_MIN(invec[i], inoutvec[i]);
		else if (i % 4 == 3) // find sum
			inoutvec[i] += invec[i];

	}
}

void perf_finalize()
{
	MPI_Op_free(&custom_opt_op);
}
