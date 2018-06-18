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
static void reduce_kp_data_local(tw_pe *pe, double *results, int len, int type)
{
	int i;
	tw_kp *kp;
	tw_stime vt_diff, efficiency;
    tw_stat nevent_delta;
    tw_stat e_rbs_delta;
	double mean[len];
	
	if (type == 0)
	{
		results[EFF_MIN] = results[TIME_MIN] = DBL_MAX;
		results[EFF_MAX] = results[TIME_MAX] = -DBL_MAX;
		results[EFF_MEAN] = results[TIME_MEAN] = 0.0;
	}
	else if (type == 1)
	{
		// results passed in are the mean, save them so we don't overwrite
		for (i = 0; i < len; i++)
		{
			mean[i] = results[i];
			results[i] = 0;
		}
	}

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

        vt_diff = kp->last_time - pe->GVT;

		if (type == 0)
		{
			if (efficiency < results[EFF_MIN])
				results[EFF_MIN] = efficiency;
			if (efficiency > results[EFF_MAX])
				results[EFF_MAX] = efficiency;
			results[EFF_MEAN] += efficiency;

			if (vt_diff < results[TIME_MIN])
				results[TIME_MIN] = vt_diff;
			if (vt_diff > results[TIME_MAX])
				results[TIME_MAX] = vt_diff;
			results[TIME_MEAN] += vt_diff;
		}
		else if (type == 1)
		{
			results[0] += (efficiency - mean[0]) * (efficiency - mean[0]);
			results[1] += (vt_diff - mean[1]) * (vt_diff - mean[1]);

			last_check.nevent_processed[i] = kp->s_nevent_processed;
			last_check.e_rbs[i] = kp->s_e_rbs;
		}
    }

}

/* use Allreduce to exchange all of the relevant kp data to use for tuning
 * NOTE: This should only be called during GVT!
 */
void perf_adjust_optimism(tw_pe *pe)
{
    if (g_perf_disable_opt)
        return;

	int len = NUM_RED_ELEM, i;
	unsigned long long inc_amt = 1000;
	double dec_amt = 0.0;
	double data[len], results[len], std_dev_local[2], std_dev_global[2];
	tw_kp *kp;

	reduce_kp_data_local(pe, &data[0], len, 0);

	//printf("PE %lu: data: %f, %f, %f, %f, %f, %f\n", pe->id, data[0], data[1], data[2], data[3], data[4], data[5]);
	MPI_Allreduce(&data, &results, len, MPI_DOUBLE, custom_opt_op, MPI_COMM_ROSS);
	results[EFF_MEAN] /= (g_tw_nkp * tw_nnodes());
	results[TIME_MEAN] /= (g_tw_nkp * tw_nnodes());
	// update means in local data as well
	data[EFF_MEAN] /= g_tw_nkp;
	data[TIME_MEAN] /= g_tw_nkp;

	//printf("PE %lu: results: %f, %f, %f, %f, %f, %f\n", pe->id, results[0], results[1], results[2], results[3], results[4], results[5]);
	std_dev_local[0] = results[EFF_MEAN];
	std_dev_local[1] = results[TIME_MEAN];
	reduce_kp_data_local(pe, &std_dev_local[0], 2, 1);

	//printf("PE %lu: std_dev_local: %f, %f\n", pe->id, std_dev_local[0], std_dev_local[1]);
	MPI_Allreduce(&std_dev_local, &std_dev_global, 2, MPI_DOUBLE, MPI_SUM, MPI_COMM_ROSS);
	std_dev_global[0] = sqrt(std_dev_global[0] / (g_tw_nkp * tw_nnodes()));
	std_dev_global[1] = sqrt(std_dev_global[1] / (g_tw_nkp * tw_nnodes()));
	//printf("PE %lu: std_dev_global: %f, %f\n", pe->id, std_dev_global[0], std_dev_global[1]);

	// if global max is < 0, then no KP has processed anything since GVT...
	// this shouldn't happen, right?
	
	// for PEs with local max local clock difference < 0, decrease the lookahead
	// because this PE has a light load and we want to preemptively keep it from getting too far ahead of GVT
	if (data[TIME_MAX] < 0)
	{
		g_tw_max_opt_lookahead = 100;
		printf("PE %ld decreasing opt lookahead to %llu\n", g_tw_mynode, g_tw_max_opt_lookahead);
	}
	else
	{
		// are we ahead or behind the mean?
		if (data[TIME_MAX] <= results[TIME_MEAN])
		{
			// if we're less than the mean, let's increase our lookahead
			if (std_dev_global[1] > 1)
				inc_amt *= std_dev_global[1];

			if (ULLONG_MAX - g_tw_max_opt_lookahead  < inc_amt) // avoid overflow
				g_tw_max_opt_lookahead = ULLONG_MAX;
			else
				g_tw_max_opt_lookahead += inc_amt;
			printf("PE %ld increasing opt lookahead to %llu\n", g_tw_mynode, g_tw_max_opt_lookahead);
		}
		else
		{
			// increase our lookahead if we're within one standard dev of global mean
			if (data[TIME_MAX] < results[TIME_MEAN] + std_dev_global[1])
			{
				if (ULLONG_MAX - g_tw_max_opt_lookahead  < inc_amt) // avoid overflow
					g_tw_max_opt_lookahead = ULLONG_MAX;
				else
					g_tw_max_opt_lookahead += inc_amt;
				printf("PE %ld increasing opt lookahead to %llu\n", g_tw_mynode, g_tw_max_opt_lookahead);
			}
			else // we're somewhere between mean + stddev and global max
			{
				// so we want to decrease our lookahead
				dec_amt *= (data[TIME_MAX] - (results[TIME_MEAN] + std_dev_global[1])) / 
					(double)(results[TIME_MAX] - (results[TIME_MEAN] + std_dev_global[1]));
				if (dec_amt < 0.02)
					dec_amt = 0.02;
				else if (dec_amt > 0.98)
					dec_amt = 0.98;
				g_tw_max_opt_lookahead *= dec_amt;
				printf("PE %ld decreasing opt lookahead to %llu\n", g_tw_mynode, g_tw_max_opt_lookahead);
			}

		}

	}

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
