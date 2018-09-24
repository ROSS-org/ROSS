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

tw_stime g_st_vt_interval = 1000000;
tw_stime g_st_sampling_end = 0;

int engine_modes[NUM_INST_MODES];
int model_modes[NUM_INST_MODES];

static char config_file[1024];


static const tw_optdef inst_options[] = {
    TWOPT_GROUP("ROSS Instrumentation"),
    TWOPT_CHAR("config-file", config_file, "Use config file for instrumentation settings (will ignore command line params set)"),
    TWOPT_UINT("engine-stats", g_st_engine_stats, "Collect sim engine level stats; 0 don't collect, 1 GVT-sampling, 2 RT sampling, 3 VT sampling, 4 All sampling modes"),
    TWOPT_UINT("model-stats", g_st_model_stats, "Collect model level stats (requires model-level implementation); 0 don't collect, 1 GVT-sampling, 2 RT sampling, 3 VT sampling, 4 all sampling modes"),
    TWOPT_UINT("num-gvt", g_st_num_gvt, "number of GVT computations between GVT-based sampling points"),
    TWOPT_ULONGLONG("rt-interval", g_st_rt_interval, "real time sampling interval in ms"),
    TWOPT_STIME("vt-interval", g_st_vt_interval, "Virtual time sampling interval"),
    TWOPT_STIME("vt-samp-end", g_st_sampling_end, "End time for virtual time sampling (if different from g_tw_ts_end)"),
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

void print_settings()
{
    printf("g_st_engine_stats:\t%d\n", g_st_engine_stats);
    printf("g_st_model_stats:\t%d\n", g_st_model_stats);
    printf("g_st_num_gvt:\t\t%d\n", g_st_num_gvt);
    printf("g_st_rt_interval:\t%lu\n", g_st_rt_interval);
    printf("g_st_vt_interval:\t%f\n", g_st_vt_interval);
    printf("g_st_sampling_end:\t%f\n", g_st_sampling_end);
    printf("g_st_pe_data:\t\t%d\n", g_st_pe_data);
    printf("g_st_kp_data:\t\t%d\n", g_st_kp_data);
    printf("g_st_lp_data:\t\t%d\n", g_st_lp_data);
    printf("g_st_ev_trace:\t\t%d\n", g_st_ev_trace);
}

// TODO this has become a mess and could definitely be waaaay better
// should be called from tw_init, so this ensures we have all instrumentation global variables
// set by the end of tw_init(), which may be used by models
void st_inst_init(void)
{
#ifdef USE_DAMARIS
    st_damaris_ross_init();
    if (!g_st_ross_rank) // Damaris ranks only
        return;

    st_damaris_parse_config(&config_file[0]);
#endif
    specialized_lp_run();

    if (!(g_st_engine_stats || g_st_model_stats || g_st_ev_trace))
        return;

    if (g_st_engine_stats == GVT_STATS || g_st_engine_stats == ALL_STATS)
    {
        g_st_gvt_sampling = 1;
        engine_modes[GVT_INST] = 1;
    }
    if (g_st_engine_stats == RT_STATS || g_st_engine_stats == ALL_STATS)
    {
        g_st_rt_sampling = 1;
        engine_modes[RT_INST] = 1;
    }
    if (g_st_model_stats == GVT_STATS || g_st_model_stats == ALL_STATS)
    {
        g_st_gvt_sampling = 1;
        model_modes[GVT_INST] = 1;
    }
    if (g_st_model_stats == RT_STATS || g_st_model_stats == ALL_STATS)
    {
        g_st_rt_sampling = 1;
        model_modes[RT_INST] = 1;
    }

    if (g_st_rt_sampling)
    {
        g_st_rt_interval = g_st_rt_interval * g_tw_clock_rate / 1000;
        g_st_rt_samp_start_cycles = tw_clock_read();
    }

#ifdef USE_DAMARIS
    if (g_st_damaris_enabled)
    {
        st_damaris_inst_init(config_file);
        g_st_disable_out = 1;
        return; // no need to set up buffers in this case
    }
#endif

    st_buffer_allocate();

    // setup appropriate flags for various instrumentation modes
    // set up files and buffers for necessary instrumentation modes
    if (g_st_engine_stats == GVT_STATS || g_st_engine_stats == ALL_STATS)
        st_buffer_init(GVT_COL);
    if (g_st_engine_stats == RT_STATS || g_st_engine_stats == ALL_STATS)
        st_buffer_init(RT_COL);

    if (g_st_ev_trace)
        st_buffer_init(EV_TRACE);
    if (g_st_model_stats)
        st_buffer_init(MODEL_COL);
}

// warning: when calling from GVT, all PEs must call else deadlock
// this function doesn't do any checking on the correct time to do sampling
// assumes the caller is only making the call when it is time to sample
void st_inst_sample(tw_pe *me, int inst_type)
{
    //printf("pe %ld about to sample for mode %d\n", g_tw_mynode, inst_type);
    tw_clock current_rt = tw_clock_read();

#ifdef USE_DAMARIS
    // need to make sure damaris_end_iteration is called if GVT instrumentation not turned on
    // new method should mean that this gets called regardless at GVT
    if (g_st_damaris_enabled)
	{
        st_damaris_expose_data(me, inst_type);
        st_damaris_end_iteration();
        return;
	}
#endif

    // TODO need to test to make sure inst still works correctly w/out damaris enabled
    if (engine_modes[inst_type] && g_tw_synchronization_protocol != SEQUENTIAL)
        st_collect_engine_data(me, inst_type);
    if (model_modes[inst_type])
        st_collect_model_data(me, (tw_stime)current_rt / g_tw_clock_rate, inst_type);

    // if damaris is enabled, g_st_disable_out == 1
    if (inst_type == GVT_INST && !g_st_disable_out)
        st_inst_dump();
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
    if (g_st_disable_out)
        return;

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
