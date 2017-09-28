#include "ross.h"
#include "analysis-lp.h"

/*
 * This file is for general set up functions related to setting up any
 * ROSS specialized LPs.
 *
 * TODO add some output on these LPs, add some counters that can be subtracted from other counters, so we're not including this in the model info
 */

int g_st_use_analysis_lps = 0;
tw_lpid g_st_analysis_nlp = 0;
tw_stime g_st_vt_interval = 1000000;
tw_stime g_st_sampling_end = 0;
int g_st_sample_count = 65536;

tw_lpid analysis_start_gid = 0;
tw_lpid g_st_total_model_lps = 0;

void specialized_lp_setup()
{
    if (!g_st_use_analysis_lps)
        return;

    // determine total LPs used by model and assign value to analysis_start_gid
    MPI_Allreduce(&g_tw_nlp, &g_st_total_model_lps, 1, MPI_UNSIGNED_LONG_LONG, MPI_SUM, MPI_COMM_ROSS);

    analysis_start_gid = g_st_total_model_lps;
    g_st_analysis_nlp = g_tw_nkp; // # of analysis LPs per PE

    if (g_st_sampling_end == 0)
        g_st_sampling_end = g_tw_ts_end;
    
    // set up buffers to be used by analysis LPs
    st_buffer_init(ANALYSIS_LP); 

}

void specialized_lp_init_mapping()
{
    tw_lpid lpid;
    for(lpid = 0; lpid < g_st_analysis_nlp; lpid++)
    {
        tw_lp_onpe(g_tw_nlp + lpid, g_tw_pe[0], analysis_start_gid + g_tw_mynode * g_st_analysis_nlp + lpid);
        tw_lp_onkp(g_tw_lp[g_tw_nlp + lpid], g_tw_kp[lpid]); // analysis lpid == kpid
        st_analysis_lp_settype(g_tw_nlp + lpid);
    }
}

const tw_optdef special_lp_opt[] =
{
    TWOPT_GROUP("Specialized ROSS LPs"),
    TWOPT_UINT("analysis-lps", g_st_use_analysis_lps, "Set to 1 to turn on analysis LPs (1 per KP) for virtual time sampling"),
    TWOPT_STIME("vt-interval", g_st_vt_interval, "Virtual time sampling interval"),
    TWOPT_STIME("vt-samp-end", g_st_sampling_end, "End time for virtual time sampling (if different from g_tw_ts_end)"),
    TWOPT_UINT("sample-count", g_st_sample_count, "Number of samples to allocate in memory"),
    TWOPT_END()
};

const tw_optdef *st_special_lp_opts(void)
{
	return special_lp_opt;
}
