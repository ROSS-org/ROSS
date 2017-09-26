#include <ross.h>
#include <sys/stat.h>

int g_st_instrumentation = 0;
int g_st_engine_stats = 0;
int g_st_rt_sampling = 0;
tw_clock g_st_rt_interval = 1000;
int g_st_gvt_sampling = 0;
int g_st_num_gvt = 10;
int g_st_model_stats = 0;
char g_st_stats_out[INST_MAX_LENGTH] = {0};


static const tw_optdef inst_options[] = {
    TWOPT_GROUP("ROSS Instrumentation"),
    TWOPT_UINT("engine-stats", g_st_engine_stats, "Collect sim engine level stats; 0 don't collect, 1 GVT-sampling, 2 RT sampling, 3 both"),
    TWOPT_UINT("model-stats", g_st_model_stats, "Collect model level stats (requires model-level implementation); 0 don't collect, 1 GVT-sampling, 2 RT sampling, 3 both"),
    TWOPT_UINT("num-gvt", g_st_num_gvt, "number of GVT computations between GVT-based sampling points"),
    TWOPT_ULONGLONG("rt-interval", g_st_rt_interval, "real time sampling interval in ms"),
    TWOPT_UINT("granularity", g_st_granularity, "for sim engine instrumentation; 0 = collect on PE basis only, 1 for KP/LP data as well"),
    TWOPT_UINT("event-trace", g_st_ev_trace, "collect detailed data on all events for specified LPs; 0, no trace, 1 full trace, 2 only events causing rollbacks, 3 only committed events"),
    TWOPT_CHAR("stats-prefix", g_st_stats_out, "prefix for filename(s) for stats output"),
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
    if (!(g_st_engine_stats || g_st_model_stats || g_st_ev_trace))
        return;

    g_st_instrumentation = 1;

    // setup appropriate flags for various instrumentation modes
    // set up files and buffers for necessary instrumentation modes
    if (g_st_engine_stats == GVT_STATS || g_st_engine_stats == BOTH_STATS)
    {
        g_st_gvt_sampling = 1;
        st_buffer_init(GVT_COL);
    }
    if (g_st_engine_stats == RT_STATS || g_st_engine_stats == BOTH_STATS)
    {
        g_st_rt_sampling = 1;
        st_buffer_init(RT_COL);
    }
    if (g_st_model_stats == GVT_STATS || g_st_model_stats == BOTH_STATS)
        g_st_gvt_sampling = 1;
    if (g_st_model_stats == RT_STATS || g_st_model_stats == BOTH_STATS)
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

    st_stats_init();
}

void st_stats_init()
{
    // Need to call after st_buffer_init()!
    int i;
    int npe = tw_nnodes();
    tw_lpid nlp_per_pe[npe];
    if (g_tw_synchronization_protocol != SEQUENTIAL)
        MPI_Gather(&g_tw_nlp, 1, MPI_UINT64_T, nlp_per_pe, 1, MPI_UINT64_T, g_tw_masternode, MPI_COMM_ROSS);
    else
        nlp_per_pe[0] = g_tw_nlp;

    if (!g_st_disable_out && g_tw_mynode == 0) {
        FILE *file;
        char filename[INST_MAX_LENGTH*2];
        sprintf(filename, "%s/%s-README.txt", stats_directory, g_st_stats_out);
        file = fopen(filename, "w");

        /* start of metadata info for binary reader */
        fprintf(file, "GRANULARITY=%d\n", g_st_granularity);
        fprintf(file, "NUM_PE=%d\n", tw_nnodes());
        fprintf(file, "NUM_KP=%lu\n", g_tw_nkp);

        fprintf(file, "LP_PER_PE=");
        for (i = 0; i < npe; i++)
        {
            if (i == npe - 1)
                fprintf(file, "%"PRIu64"\n", nlp_per_pe[i]);
            else
                fprintf(file, "%"PRIu64",", nlp_per_pe[i]);
        }

        print_sim_engine_metadata(file);

        fprintf(file, "END\n\n");

        /* end of metadata info for binary reader */
        fprintf(file, "Info for ROSS run.\n\n");
#if HAVE_CTIME
        time_t raw_time;
        time(&raw_time);
        fprintf(file, "Date Created:\t%s\n", ctime(&raw_time));
#endif
        fprintf(file, "## BUILD CONFIGURATION\n\n");
#ifdef ROSS_VERSION
        fprintf(file, "ROSS Version:\t%s\n", ROSS_VERSION);
#endif
        //fprintf(file, "MODEL Version:\t%s\n", model_version);
        fprintf(file, "\n## RUN TIME SETTINGS by GROUP:\n\n");
        tw_opt_settings(file);
        fclose(file);
    }
}

void st_inst_dump()
{
    if (!g_st_disable_out && (g_st_engine_stats == GVT_STATS || g_st_engine_stats == BOTH_STATS))
        st_buffer_write(0, GVT_COL);
    if (!g_st_disable_out && (g_st_engine_stats == RT_STATS || g_st_engine_stats == BOTH_STATS))
        st_buffer_write(0, RT_COL);
    if (!g_st_disable_out && (g_st_ev_trace))
        st_buffer_write(0, EV_TRACE);
    if (!g_st_disable_out && (g_st_model_stats))
        st_buffer_write(0, MODEL_COL);
}

void st_inst_finalize(tw_pe *me)
{
    if (g_st_engine_stats == GVT_STATS || g_st_engine_stats == BOTH_STATS)
        st_buffer_finalize(GVT_COL);
    if (g_st_engine_stats == RT_STATS || g_st_engine_stats == BOTH_STATS)
    {
        // collect data one final time to account for time between last sample and sim end time
        st_collect_data(me, (double)tw_clock_read() / g_tw_clock_rate);
        st_buffer_finalize(RT_COL);
    }
    if (g_st_ev_trace)
        st_buffer_finalize(EV_TRACE);
    if (g_st_model_stats)
        st_buffer_finalize(MODEL_COL);

}
