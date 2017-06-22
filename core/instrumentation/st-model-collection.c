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
        fprintf(stderr, "The struct st_model_types has not been defined! No model level data will be collected\n");
        model_type_warned = 1;
    }
}

/*
 * This function allows for ROSS to collect model level data.
 * Call this function when collecting simulation level data (GVT-based and/or real time-based).
 * Loop through all LPs on this PE and collect stats
 */
void st_collect_model_data(tw_pe *pe, tw_stime current_rt, int stats_type)
{
    int index, lpid = 0;
    int initial_sz = sizeof(tw_lpid) + sizeof(tw_stime) * 2 + sizeof(int);
    int model_sz, total_sz = 0;
    char *model_buffer = NULL;
    tw_lp *clp;
    tw_clock start_cycle_time = tw_clock_read();

    for (lpid = 0; lpid < g_tw_nlp; lpid++) 
    {
        index = 0;
        clp = g_tw_lp[lpid];
        if (!clp->model_types)
        {
            // may not want to collect model stats on every LP type, so if not defined, just continue
            continue;
        }

        if (!clp->model_types->model_stat_fn)
        {
            // may not want to collect model stats on every LP type, so if not defined, just continue
            continue;
        }

        model_sz = clp->model_types->mstat_sz;
        total_sz = initial_sz + model_sz;
        char buffer[total_sz];

        if (model_sz > 0)
        {
            // TODO want to have KP LVT instead of GVT?
            memcpy(&buffer[index], &(clp->gid), sizeof(tw_lpid));
            index += sizeof(tw_lpid);
            memcpy(&buffer[index], &current_rt, sizeof(tw_stime));
            index += sizeof(tw_stime);
            memcpy(&buffer[index], &pe->GVT, sizeof(tw_stime));
            index += sizeof(tw_stime);
            memcpy(&buffer[index], &stats_type, sizeof(int));
            index += sizeof(int);
            if (index != initial_sz)
                printf("WARNING: size of data being pushed to buffer is incorrect!\n");
            (*clp->model_types->model_stat_fn)(clp->cur_state, clp, &buffer[index]);

            st_buffer_push(g_st_buffer_model, &buffer[0], total_sz);
        }
    }
    g_st_stat_comp_ctr += tw_clock_read() - start_cycle_time;
}

