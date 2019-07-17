#include <ross.h>
#include <sys/stat.h>

char g_st_stats_out[INST_MAX_LENGTH] = {0};
char g_st_stats_path[4096] = {0};
int g_st_pe_data = 1;
int g_st_kp_data = 0;
int g_st_lp_data = 0;
int g_st_disable_out = 0;

int g_st_model_stats = 0;
int g_st_engine_stats = 0;

int g_st_gvt_sampling = 0;
int g_st_num_gvt = 10;

int g_st_rt_sampling = 0;
tw_clock g_st_rt_interval = 1000;
tw_clock g_st_rt_samp_start_cycles = 0;

double g_st_vt_interval = 1000000;
double g_st_sampling_end = 0;



static const tw_optdef inst_options[] = {
    TWOPT_GROUP("ROSS Instrumentation"),
    TWOPT_UINT("engine-stats", g_st_engine_stats, "Collect sim engine level stats; 0 don't collect, 1 GVT-sampling, 2 RT sampling, 3 VT sampling, 4 All sampling modes"),
    TWOPT_UINT("model-stats", g_st_model_stats, "Collect model level stats (requires model-level implementation); 0 don't collect, 1 GVT-sampling, 2 RT sampling, 3 VT sampling, 4 all sampling modes"),
    TWOPT_UINT("num-gvt", g_st_num_gvt, "number of GVT computations between GVT-based sampling points"),
    TWOPT_ULONGLONG("rt-interval", g_st_rt_interval, "real time sampling interval in ms"),
    TWOPT_DOUBLE("vt-interval", g_st_vt_interval, "Virtual time sampling interval"),
    TWOPT_DOUBLE("vt-samp-end", g_st_sampling_end, "End time for virtual time sampling (if different from g_tw_ts_end)"),
    TWOPT_UINT("pe-data", g_st_pe_data, "Turn on/off collection of sim engine data at PE level"),
    TWOPT_UINT("kp-data", g_st_kp_data, "Turn on/off collection of sim engine data at KP level"),
    TWOPT_UINT("lp-data", g_st_lp_data, "Turn on/off collection of sim engine data at LP level"),
    TWOPT_UINT("event-trace", g_st_ev_trace, "collect detailed data on all events for specified LPs; 0, no trace, 1 full trace, 2 only events causing rollbacks, 3 only committed events"),
    TWOPT_CHAR("stats-prefix", g_st_stats_out, "prefix for filename(s) for stats output"),
    TWOPT_CHAR("stats-path", g_st_stats_path, "path to directory to save instrumentation output"),
    TWOPT_UINT("buffer-size", g_st_buffer_size, "size of buffer in bytes for stats collection"),
    TWOPT_UINT("buffer-free", g_st_buffer_free_percent, "percentage of free space left in buffer before writing out at GVT"),
    TWOPT_UINT("disable-output", g_st_disable_out, "used for perturbation analysis; buffer never dumped to file when 1"),
    TWOPT_END()
};

const tw_optdef *st_inst_opts(void)
{
	return inst_options;
}

void st_inst_init(void)
{
    specialized_lp_run();

    if (!(g_st_engine_stats || g_st_model_stats || g_st_ev_trace))
        return;

    // setup appropriate flags for various instrumentation modes
    // set up files and buffers for necessary instrumentation modes
    if (g_st_engine_stats == GVT_STATS || g_st_engine_stats == ALL_STATS)
    {
        g_st_gvt_sampling = 1;
        st_buffer_init(GVT_COL);
    }
    if (g_st_engine_stats == RT_STATS || g_st_engine_stats == ALL_STATS)
    {
        g_st_rt_sampling = 1;
        st_buffer_init(RT_COL);
    }

    if (g_st_model_stats == GVT_STATS || g_st_model_stats == ALL_STATS)
        g_st_gvt_sampling = 1;
    if (g_st_model_stats == RT_STATS || g_st_model_stats == ALL_STATS)
        g_st_rt_sampling = 1;

    if (g_st_rt_sampling)
    {
        g_st_rt_interval = g_st_rt_interval * g_tw_clock_rate / 1000;
        g_st_rt_samp_start_cycles = tw_clock_read();
    }

    if (g_st_ev_trace)
        st_buffer_init(EV_TRACE);
    if (g_st_model_stats)
        st_buffer_init(MODEL_COL);
}

void st_inst_dump()
{
    if (g_st_disable_out)
        return;

    if (g_st_engine_stats == GVT_STATS || g_st_engine_stats == ALL_STATS)
        st_buffer_write(0, GVT_COL);
    if (g_st_engine_stats == RT_STATS || g_st_engine_stats == ALL_STATS)
        st_buffer_write(0, RT_COL);
    if (g_st_ev_trace)
        st_buffer_write(0, EV_TRACE);
    if (g_st_model_stats)
        st_buffer_write(0, MODEL_COL);
    if (g_st_use_analysis_lps)
        st_buffer_write(0, ANALYSIS_LP);
}

void st_inst_finalize(tw_pe *me)
{
    if (g_st_engine_stats == GVT_STATS || g_st_engine_stats == ALL_STATS)
        st_buffer_finalize(GVT_COL);
    if (g_st_engine_stats == RT_STATS || g_st_engine_stats == ALL_STATS)
    {
        // collect data one final time to account for time between last sample and sim end time
        st_collect_engine_data(me, RT_COL);
        st_buffer_finalize(RT_COL);
    }
    if (g_st_ev_trace)
        st_buffer_finalize(EV_TRACE);
    if (g_st_model_stats)
        st_buffer_finalize(MODEL_COL);
    if (g_st_use_analysis_lps)
        st_buffer_finalize(ANALYSIS_LP);

}
