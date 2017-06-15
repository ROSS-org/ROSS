#include <ross.h>

extern MPI_Comm MPI_COMM_ROSS;

st_stats_buffer **g_st_buffer;

char g_st_stats_out[INST_MAX_LENGTH] = {0};

static const tw_optdef inst_options[] = {
    TWOPT_GROUP("ROSS Instrumentation"),
    TWOPT_UINT("model-stats", g_st_model_stats, "Collect model level stats (requires model-level implementation); 0 don't collect, 1 GVT-sampling, 2 RT sampling, 3 both"),
    TWOPT_UINT("enable-gvt-stats", g_st_stats_enabled, "Collect data after each GVT; 0 no stats, 1 for stats"),
    TWOPT_UINT("num-gvt", g_st_num_gvt, "number of GVT computations between GVT-based sampling points"),
    TWOPT_ULONGLONG("real-time-samp", g_st_real_time_samp, "real time sampling interval in ms"),
    TWOPT_UINT("granularity", g_st_granularity, "collect on PE basis only, or also KP/LP basis, 0 for PE, 1 for KP/LP"),
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
    g_st_buffer = (st_stats_buffer**) tw_calloc(TW_LOC, "instrumentation (buffer)", sizeof(st_stats_buffer*), NUM_COL_TYPES); 
    // set up files and buffers for necessary instrumentation modes
    if (g_st_stats_enabled)
    {
        char suffix[4];
        sprintf(suffix, "gvt");
        g_st_buffer[GVT_COL] = st_buffer_init(suffix, &g_st_gvt_fh);
    }
    if (g_st_real_time_samp)
    {
        g_st_real_time_samp = g_st_real_time_samp * g_tw_clock_rate / 1000;
        g_st_real_samp_start_cycles = tw_clock_read();
        char suffix[3];
        sprintf(suffix, "rt");
        g_st_buffer[RT_COL] = st_buffer_init(suffix, &g_st_rt_fh);
    }
    if (g_st_ev_trace)
    {
        char suffix[8];
        sprintf(suffix, "evtrace");
        g_st_buffer[EV_TRACE] = st_buffer_init(suffix, &g_st_evrb_fh);
    }
    if (g_st_model_stats)
    {
        char suffix[6];
        sprintf(suffix, "model");
        g_st_buffer[MODEL_COL] = st_buffer_init(suffix, &g_st_model_fh);
    }

    if (g_st_stats_enabled || g_st_real_time_samp || g_st_ev_trace || g_st_model_stats)
        st_stats_init();
}

void st_stats_init()
{
    // Need to call after st_buffer_init()!
    int i;
    int npe = tw_nnodes();
    tw_lpid nlp_per_pe[npe];
    MPI_Gather(&g_tw_nlp, 1, MPI_UINT64_T, nlp_per_pe, 1, MPI_UINT64_T, 0, MPI_COMM_ROSS);

    if (!g_st_disable_out && g_tw_mynode == 0) {
        FILE *file;
        char filename[INST_MAX_LENGTH];
        sprintf(filename, "%s/%s-README.txt", g_st_directory, g_st_stats_out);
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
    if (!g_st_disable_out && g_st_stats_enabled)
        st_buffer_write(g_st_buffer[GVT_COL], 0, GVT_COL);
    if (!g_st_disable_out && g_st_real_time_samp)
        st_buffer_write(g_st_buffer[RT_COL], 0, RT_COL);
    if (!g_st_disable_out && (g_st_ev_trace))
        st_buffer_write(g_st_buffer[EV_TRACE], 0, EV_TRACE);
    if (!g_st_disable_out && (g_st_model_stats))
        st_buffer_write(g_st_buffer[MODEL_COL], 0, MODEL_COL);
}

void st_inst_finalize(tw_pe *me)
{
    if (g_st_stats_enabled)
        st_buffer_finalize(g_st_buffer[GVT_COL], GVT_COL);
    if (g_st_real_time_samp)
    {
        // collect data one final time to account for time between last sample and sim end time
        st_collect_data(me, (double)tw_clock_read() / g_tw_clock_rate);
        st_buffer_finalize(g_st_buffer[RT_COL], RT_COL);
    }
    if (g_st_ev_trace)
        st_buffer_finalize(g_st_buffer[EV_TRACE], EV_TRACE);
    if (g_st_model_stats)
        st_buffer_finalize(g_st_buffer[MODEL_COL], MODEL_COL);

}
