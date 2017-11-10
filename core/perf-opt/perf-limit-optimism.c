#include "ross.h"

static int limit_opt = LIMIT_OFF;
static pe_data last_check = {0};
int g_perf_disable_opt = 0;

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
}

void perf_efficiency_check(tw_pe *pe)
{
    //tw_statistics s;
    //bzero(&s, sizeof(s));
    //tw_get_stats(pe, &s);

    int i;
    tw_kp *kp;
    tw_stat nevent_delta;
    tw_stat e_rbs_delta;
    double efficiency = 0;
    double lowest_eff = DBL_MAX;

    for (i = 0; i < g_tw_nkp; i++)
    {
        kp = tw_getkp(i);
        nevent_delta = kp->s_nevent_processed - last_check.nevent_processed[i];
        e_rbs_delta = kp->s_e_rbs - last_check.e_rbs[i];
        if (nevent_delta - e_rbs_delta != 0)
            efficiency = 100.0 * (1 - ((double)e_rbs_delta/(double)(nevent_delta - e_rbs_delta)));
        else
            efficiency = 0;

        if (efficiency < lowest_eff)
            lowest_eff = efficiency;

        last_check.nevent_processed[i] = kp->s_nevent_processed;
        last_check.e_rbs[i] = kp->s_e_rbs;

    }

    efficiency = lowest_eff;

    if (limit_opt == LIMIT_OFF || limit_opt == LIMIT_RECOVER)
    {
        if (efficiency < 0.0 && efficiency >= -100.0) // set flag to limit optimism
            limit_opt = LIMIT_ON;
        else if (efficiency < -100)
            limit_opt = LIMIT_AGGRESSIVE;
    }
    else if (limit_opt == LIMIT_ON)
    {
        if (efficiency > 0)
            limit_opt = LIMIT_RECOVER;
        else if (efficiency < -100)
            limit_opt = LIMIT_AGGRESSIVE;
    }
    else if (limit_opt == LIMIT_AGGRESSIVE)
    {
        if (efficiency > 0)
            limit_opt = LIMIT_RECOVER;
        else if (efficiency < 0 && efficiency > -100)
            limit_opt = LIMIT_ON;
    }

}

void perf_adjust_optimism(tw_pe *pe)
{
    if (g_perf_disable_opt)
        return;

    perf_efficiency_check(pe);

    if (limit_opt == LIMIT_ON)
    { // use multiplicative decrease to lower g_st_max_opt_lookahead
        if (g_tw_max_opt_lookahead >= 1000)
            g_tw_max_opt_lookahead *= 0.75;
        //printf("PE %ld decreasing opt lookahead to %llu\n", g_tw_mynode, g_tw_max_opt_lookahead);
    }
    else if (limit_opt == LIMIT_AGGRESSIVE)
    { // use multiplicative decrease to lower g_st_max_opt_lookahead
        if (g_tw_max_opt_lookahead >= 1000)
            g_tw_max_opt_lookahead *= 0.5;
        //printf("PE %ld decreasing opt lookahead (aggressive) to %llu\n", g_tw_mynode, g_tw_max_opt_lookahead);
    }
    else if (limit_opt == LIMIT_RECOVER)
    { // use additive increase to slowly allow for more optimism
        if (ULLONG_MAX - g_tw_max_opt_lookahead < 1000) // avoid overflow
            g_tw_max_opt_lookahead = ULLONG_MAX;
        else 
            g_tw_max_opt_lookahead += 1000;
        //printf("PE %ld increasing opt lookahead to %llu\n", g_tw_mynode, g_tw_max_opt_lookahead);
    }
}

