#include <ross.h>
#include <sys/stat.h>
#include <instrumentation/st-stats-buffer.h>
#include <instrumentation/st-instrumentation-internal.h>
#include <instrumentation/st-sim-engine.h>
#include <instrumentation/st-model-data.h>

char g_st_stats_out[INST_MAX_LENGTH] = {0};
char g_st_stats_path[4096] = {0};
int g_st_pe_data = 1;
int g_st_kp_data = 0;
int g_st_lp_data = 0;
int g_st_disable_out = 0;

int g_st_model_stats = 0;
int g_st_engine_stats = 0;

int g_st_num_gvt = 10;

int g_st_rt_sampling = 0;
tw_clock g_st_rt_interval = 1000;
tw_clock g_st_rt_samp_start_cycles = 0;

tw_stime g_st_vt_interval = 1000000;
tw_stime g_st_sampling_end = 0;

short model_modes[NUM_INST_MODES] = {0};
short engine_modes[NUM_INST_MODES] = {0};
static size_t engine_data_sizes[NUM_INST_MODES] = {0};
static size_t model_data_sizes[NUM_INST_MODES] = {0};
static int model_num_lps[NUM_INST_MODES] = {0};

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

// should be called from tw_init, so this ensures we have all instrumentation global variables
// set by the end of tw_init(), which may be used by models
// TODO do we still need to make this true? currently called from tw_run()
void st_inst_init(void)
{
#ifdef USE_DAMARIS
    st_damaris_ross_init();
    if (!g_st_ross_rank) // Damaris ranks only
        return;

    st_damaris_parse_config(&config_file[0]);
#endif
    specialized_lp_run();

    // setup appropriate flags for various instrumentation modes
    if (!(g_st_engine_stats || g_st_model_stats || g_st_ev_trace))
        return;

    if (g_st_engine_stats == GVT_STATS || g_st_engine_stats == ALL_STATS)
    {
        engine_modes[GVT_INST] = 1;
    }
    if (g_st_engine_stats == RT_STATS || g_st_engine_stats == ALL_STATS)
    {
        g_st_rt_sampling = 1;
        engine_modes[RT_INST] = 1;
    }
    if (g_st_engine_stats == VT_STATS || g_st_engine_stats == ALL_STATS)
    {
        engine_modes[VT_INST] = 1;
    }
    if (g_st_model_stats == GVT_STATS || g_st_model_stats == ALL_STATS)
    {
        model_modes[GVT_INST] = 1;
    }
    if (g_st_model_stats == RT_STATS || g_st_model_stats == ALL_STATS)
    {
        g_st_rt_sampling = 1;
        model_modes[RT_INST] = 1;
    }
    if (g_st_model_stats == VT_STATS || g_st_model_stats == ALL_STATS)
    {
        model_modes[VT_INST] = 1;
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

}

void st_inst_finish_setup()
{
    if (!(g_st_engine_stats || g_st_model_stats || g_st_ev_trace))
        return;

    engine_data_sizes[GVT_INST] = calc_sim_engine_sample_size();
    engine_data_sizes[RT_INST] = calc_sim_engine_sample_size();
    // engine_data_sizes[VT_INST] has to be recalculated each time, for now
    model_data_sizes[GVT_INST] = calc_model_sample_size(GVT_INST, &model_num_lps[GVT_INST]);
    model_data_sizes[RT_INST] = calc_model_sample_size(RT_INST, &model_num_lps[RT_INST]);
    //model_data_sizes[VT_INST] = calc_model_sample_size(VT_INST, &model_num_lps[VT_INST]);
    // potentially different size per KP

    st_buffer_allocate();

}

// warning: when calling from GVT, all PEs must call else deadlock
// this function doesn't do any checking on the correct time to do sampling
// assumes the caller is only making the call when it is time to sample
void st_inst_sample(tw_pe *me, int inst_type)
{
    inst_sample(me, inst_type, NULL, 0);
}

void inst_sample(tw_pe *me, int inst_type, tw_lp* lp, int vts_commit)
{
    //printf("pe %ld about to sample for mode %d\n", g_tw_mynode, inst_type);
    tw_clock current_rt = tw_clock_read();

#ifdef USE_DAMARIS
    // need to make sure damaris_end_iteration is called if GVT instrumentation not turned on
    // new method should mean that this gets called regardless at GVT
    if (g_st_damaris_enabled)
	{
        st_damaris_expose_data(me, inst_type);
        if (inst_type == GVT_INST)
            st_damaris_end_iteration(me->GVT);
        return;
	}
#endif

    size_t buf_size = 0;
    char* buf_ptr;
    sample_metadata* sample_md;
    if (engine_modes[inst_type] || model_modes[inst_type])
    {
        buf_size = sizeof(sample_metadata);
        if (inst_type == VT_INST && engine_modes[VT_INST])
            engine_data_sizes[VT_INST] = calc_sim_engine_sample_size_vts(lp);
        if (!vts_commit && engine_modes[inst_type])
            buf_size += engine_data_sizes[inst_type];

        if (model_modes[inst_type] && inst_type != VT_INST)
            buf_size += model_data_sizes[inst_type];
        else if (model_modes[inst_type] && inst_type == VT_INST && vts_commit)
        {
            model_data_sizes[inst_type] = get_model_data_size(lp->cur_state, &model_num_lps[inst_type]);
            buf_size += model_data_sizes[inst_type];
            //printf("%lu: size = %lu\n", g_tw_mynode, model_data_sizes[inst_type]);
        }
        //printf("st_inst_sample: buf_size %lu\n", buf_size);

        if (buf_size > sizeof(sample_metadata))
        {
            buf_ptr = st_buffer_pointer(inst_type, buf_size);

            sample_md = (sample_metadata*)buf_ptr;
            buf_ptr += sizeof(*sample_md);
            buf_size -= sizeof(*sample_md);
            //printf("sizeof sample_md %lu\n", sizeof(*sample_md));

            bzero(sample_md, sizeof(*sample_md));
            sample_md->last_gvt = me->GVT;
            sample_md->rts = (double)tw_clock_read() / g_tw_clock_rate;
            //if(inst_type == VT_INST)
            //{
            //    // TODO this happens only at commit time, so this is incorrect
            //    //sample_md->vts = tw_now(lp);
            //    //printf("sample_md->vts %f\n", sample_md->vts);
            //}

            sample_md->peid = (unsigned int)g_tw_mynode;
            sample_md->num_model_lps = model_num_lps[inst_type];
        }
        else
        {
            buf_size = 0;
            //printf("%lu: Not writing data! vts_commit = %d\n", g_tw_mynode, vts_commit);
        }
    }

    if (!vts_commit && buf_size && engine_modes[inst_type] && g_tw_synchronization_protocol != SEQUENTIAL)
    {
        if (inst_type == VT_INST)
            sample_md->vts = tw_now(lp);
        st_collect_engine_data(me, inst_type, buf_ptr, engine_data_sizes[inst_type], sample_md, lp);
        buf_ptr += engine_data_sizes[inst_type];
        buf_size -= engine_data_sizes[inst_type];
    }
    if (buf_size && model_modes[inst_type])
    {
        if (inst_type != VT_INST)
        {
            sample_md->has_model = 1;
            st_collect_model_data(me, inst_type, buf_ptr, model_data_sizes[inst_type]);
            buf_size -= model_data_sizes[inst_type];
        }
        else if (inst_type == VT_INST && vts_commit)
        {
            sample_md->has_model = 1;
            st_collect_model_data_vts(me, lp, inst_type, buf_ptr, sample_md, model_data_sizes[inst_type]);
            buf_size -= model_data_sizes[inst_type];
        }
    }

    if (buf_size != 0)
        tw_error(TW_LOC, "buf_size = %lu", buf_size);

    // if damaris is enabled, g_st_disable_out == 1
    if (inst_type == GVT_INST && !g_st_disable_out)
        st_inst_dump();
    // GVT mode doesn't work in sequential
    if (g_tw_synchronization_protocol == SEQUENTIAL && !g_st_disable_out)
        st_inst_dump();
}

void st_inst_dump()
{
    if (g_st_disable_out)
        return;

    if (engine_modes[GVT_INST] || model_modes[GVT_INST])
        st_buffer_write(0, GVT_INST);
    if (engine_modes[RT_INST] || model_modes[RT_INST])
        st_buffer_write(0, RT_INST);
    if (g_st_ev_trace)
        st_buffer_write(0, ET_INST);
    if (g_st_use_analysis_lps)
        st_buffer_write(0, VT_INST);
}

void st_inst_finalize(tw_pe *me)
{
    if (g_st_disable_out)
        return;

    if (engine_modes[GVT_INST] || model_modes[GVT_INST])
        st_buffer_finalize(GVT_INST);
    if (engine_modes[RT_INST] || model_modes[RT_INST])
    {
        // collect data one final time to account for time between last sample and sim end time
        st_inst_sample(me, RT_INST);
        st_buffer_finalize(RT_INST);
    }
    if (g_st_ev_trace)
        st_buffer_finalize(ET_INST);
    if (g_st_use_analysis_lps)
        st_buffer_finalize(VT_INST);

}
