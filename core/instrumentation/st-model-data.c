#include <ross.h>

st_model_types *g_st_model_types = NULL;
static int model_type_warned = 0;


// if model uses tw_lp_setup_types() to set lp->type, it will also call
// this function to set up the functions types for model-level data collection
// because this can make use of the already defined type mapping
void st_model_setup_types(tw_lp *lp)
{
    if (g_st_model_types)
        lp->model_types = &g_st_model_types[g_tw_lp_typemap(lp->gid)];
    else if (!model_type_warned && g_tw_mynode == g_tw_masternode)
    {
        fprintf(stderr, "WARNING: node: %ld: %s:%i: ", g_tw_mynode, __FILE__, __LINE__);
        fprintf(stderr, "The g_st_model_types has not been defined! No model level data will be collected\n");
        model_type_warned = 1;
    }

}

// if model uses tw_lp_settypes(), model will also need to call
// this function to set up function types for model-level data collection
void st_model_settype(tw_lpid i, st_model_types *model_types)
{
    if (model_types)
    {
        tw_lp *lp = g_tw_lp[i];
        lp->model_types = model_types;
    }
    else if (!model_type_warned && g_tw_mynode == g_tw_masternode)
    {
        fprintf(stderr, "WARNING: node: %ld: %s:%i: ", g_tw_mynode, __FILE__, __LINE__);
        fprintf(stderr, "The struct st_model_types has not been defined for at least 1 LP type! No model level data will be collected for LP types without a valid st_model_types struct defined.\n");
        model_type_warned = 1;
    }
}

/*
 * This function allows for ROSS to collect model level data, when not using Analysis LPs.
 * Call this function when collecting simulation level data (GVT-based and/or real time-based).
 * Loop through all LPs on this PE and collect stats
 */
void st_collect_model_data(tw_pe *pe, double current_rt, int stats_type)
{
    tw_clock start_cycle_time = tw_clock_read();
    int index;
    tw_lpid lpid = 0;
    int total_sz = 0;
    tw_lp *clp;
    sample_metadata sample_md;
    model_metadata model_md;
    sample_md.flag = MODEL_TYPE;
    sample_md.sample_sz = sizeof(model_md);
    sample_md.real_time = current_rt;
    model_md.peid = (unsigned int) g_tw_mynode;
#ifdef USE_RAND_TIEBREAKER
    model_md.gvt = (float) TW_STIME_DBL(pe->GVT_sig.recv_ts);
#else
    model_md.gvt = (float) TW_STIME_DBL(pe->GVT);
#endif
    model_md.stats_type = stats_type;

    for (lpid = 0; lpid < g_tw_nlp; lpid++)
    {
        index = 0;
        clp = g_tw_lp[lpid];
        if (!clp->model_types || !clp->model_types->model_stat_fn)
        {
            // may not want to collect model stats on every LP type, so if not defined, just continue
            continue;
        }

        sample_md.ts = tw_now(clp);
        model_md.kpid = (unsigned int) clp->kp->id;
        model_md.lpid = (unsigned int) clp->gid;
        model_md.model_sz = (unsigned int) clp->model_types->mstat_sz;
        total_sz = sizeof(sample_md) + sizeof(model_md) + model_md.model_sz;
        char buffer[total_sz];
        memcpy(&buffer[0], &sample_md, sizeof(sample_md));
        index += sizeof(sample_md);
        memcpy(&buffer[index], &model_md, sizeof(model_md));
        index += sizeof(model_md);

        if (model_md.model_sz > 0)
        {
            (*clp->model_types->model_stat_fn)(clp->cur_state, clp, &buffer[index]);

            if (g_tw_synchronization_protocol != SEQUENTIAL)
                st_buffer_push(MODEL_COL, &buffer[0], total_sz);
            else if (g_tw_synchronization_protocol == SEQUENTIAL && !g_st_disable_out)
                fwrite(buffer, total_sz, 1, seq_model);
        }
    }
    pe->stats.s_stat_comp += tw_clock_read() - start_cycle_time;
}
