#include <ross.h>
#include <instrumentation/st-stats-buffer.h>
#include <instrumentation/st-instrumentation-internal.h>
#include <instrumentation/st-model-data.h>

st_model_types *g_st_model_types = NULL;
static int model_type_warned = 0;

// used to help track the current sample details
// since model collection will call a model callback function
// fine to use the same one for different sampling modes, because
// its temporary and is serial
static model_buffer cur_buf;
static vts_model_sample cur_vts_data;

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
        fprintf(stderr, "The struct st_model_types has not been defined for at least 1 LP type!"
                "No model level data will be collected for LP types without a valid st_model_types struct defined.\n");
        model_type_warned = 1;
    }
}

size_t calc_model_sample_size(int inst_mode, int *num_lps)
{
    if (inst_mode == VT_INST)
        tw_error(TW_LOC, "VTS calculates model sample size in the Analysis LPs!");

    *num_lps = 0;

    if (!model_modes[inst_mode])
        return 0;

    size_t total_size = 0;
    size_t type_size = 0;
    unsigned int lpid = 0, i = 0;
    tw_lp* clp;

    for (lpid = 0; lpid < g_tw_nlp; lpid++)
    {
        clp = g_tw_lp[lpid];
        if (inst_mode != VT_INST &&
                (!clp->model_types || !clp->model_types->rt_event_fn || clp->model_types->num_vars <= 0))
            continue;

        (*num_lps)++;
        total_size += sizeof(st_model_data);
        // loop through this LP's variables
        for (i = 0; i < clp->model_types->num_vars; i++)
        {
            total_size += sizeof(model_var_md);
            type_size = model_var_type_size(clp->model_types->model_vars[i].type);
            total_size += type_size * clp->model_types->model_vars[i].num_elems;
        }
    }

    return total_size;
}

size_t model_var_type_size(st_model_typename type)
{
    if (type == MODEL_INT)
        return sizeof(int);
    else if (type == MODEL_LONG)
        return sizeof(long);
    else if (type == MODEL_FLOAT)
        return sizeof(float);
    else if (type == MODEL_DOUBLE)
        return sizeof(double);
    return 0;
}

/*
 * This function allows for ROSS to collect model level data, when not using Analysis LPs.
 * Loop through all LPs on this PE and collect stats
 */
void st_collect_model_data(tw_pe *pe, int inst_mode, char *buffer, size_t data_size)
{
    if (inst_mode == VT_INST)
        tw_error(TW_LOC, "Should not be called for VTS!");

    tw_clock start_cycle_time = tw_clock_read();

    cur_buf.buffer = buffer;
    cur_buf.cur_pos = buffer;
    cur_buf.total_size = data_size;
    cur_buf.cur_size = 0;
    cur_buf.inst_mode = inst_mode;

    unsigned int lpid = 0;
    tw_lp *clp;

    for (lpid = 0; lpid < g_tw_nlp; lpid++)
    {
        clp = g_tw_lp[lpid];
        if (!clp->model_types || !clp->model_types->rt_event_fn || clp->model_types->num_vars <= 0)
            // may not want to collect model stats on every LP type, so if not defined, just continue
            continue;

        st_model_data* model_data = (st_model_data*)cur_buf.cur_pos;
        cur_buf.cur_pos += sizeof(*model_data);
        cur_buf.cur_size += sizeof(*model_data);

        memcpy(model_data->lp_name, clp->model_types->lp_name, lp_name_len);
        model_data->lpid = (unsigned int) clp->gid;
        model_data->kpid = (unsigned int) clp->kp->id;
        model_data->num_vars = clp->model_types->num_vars;

        (*clp->model_types->rt_event_fn)(clp->cur_state, clp);

    }

    if (cur_buf.cur_size != cur_buf.total_size)
        tw_error(TW_LOC, "Model data caused buffer overflow! cur_size %lu, data_size %lu\n",
                cur_buf.cur_size, cur_buf.total_size);

    pe->stats.s_stat_comp += tw_clock_read() - start_cycle_time;
}

void st_collect_model_data_vts(tw_pe *pe, tw_lp* lp, int inst_mode, char* buffer,
        sample_metadata *sample_md, size_t data_size)
{
    if (inst_mode != VT_INST)
        return;

    model_sample_data *sample = cur_vts_data.cur_sample;
    sample_md->vts = sample->timestamp;
    int i, j;
    char *cur_buf_ptr = buffer;
    size_t cur_size = 0;
    for (i = 0; i < sample->num_lps; i++)
    {
        model_lp_sample *cur_lp = &sample->lp_data[i];
        st_model_data *model_data = (st_model_data*)cur_buf_ptr;
        cur_buf_ptr += sizeof(*model_data);
        cur_size += sizeof(*model_data);

        memcpy(model_data->lp_name, cur_lp->lp_name, lp_name_len);
        model_data->lpid = (unsigned int) cur_lp->lpid;
        model_data->kpid = (unsigned int) lp->kp->id;
        model_data->num_vars = cur_lp->num_vars;

        for (j = 0; j < cur_lp->num_vars; j++)
        {
            model_var_data *var_data = &cur_lp->vars[j];
            model_var_md* cur_var = (model_var_md*)cur_buf_ptr;
            cur_buf_ptr += sizeof(*cur_var);
            cur_size += sizeof(*cur_var);

            cur_var->var_id = j;
            cur_var->num_elems = var_data->var.num_elems;
            cur_var->type = var_data->var.type;
            size_t type_size = model_var_type_size(cur_var->type);
            size_t size = cur_var->num_elems * type_size;

            // TODO can we remove the memcpy somehow?
            memcpy(cur_buf_ptr, var_data->data, size);
            cur_buf_ptr += size;
            cur_size += size;
        }
    }
    if (cur_size != data_size)
        tw_error(TW_LOC, "Model data caused buffer overflow! cur_size %lu, data_size %lu\n", cur_size, data_size);
}

int variable_id_lookup(tw_lp* lp, const char* var_name)
{
    if (!lp->model_types || !lp->model_types->model_vars || lp->model_types->num_vars <= 0)
        tw_error(TW_LOC, "Couldn't find var_name %s in st_model_types for LP %lu!\n", var_name, lp->gid);

    int i;
    for (i = 0; i < lp->model_types->num_vars; i++)
    {
        if (strcmp(lp->model_types->model_vars[i].var_name, var_name) == 0)
            return i;
    }

    return -1;
}

void vts_start_sample(model_sample_data* sample)
{
    cur_vts_data.in_progress = 1;
    cur_vts_data.cur_sample = sample;
    cur_vts_data.cur_lp = NULL;
}

void vts_next_lp(tw_lpid id)
{
    if (!cur_vts_data.cur_lp)
        cur_vts_data.cur_lp = &cur_vts_data.cur_sample->lp_data[0];
    else
        (cur_vts_data.cur_lp)++;

    cur_vts_data.cur_var = &cur_vts_data.cur_lp->vars[0];

    if (id != cur_vts_data.cur_lp->lpid)
        tw_error(TW_LOC, "saving incorrect LP data! requesting %lu, but have %lu",
                id, cur_vts_data.cur_lp->lpid);
}

void vts_next_lp_rev(tw_lpid id)
{
    if (!cur_vts_data.cur_lp)
        cur_vts_data.cur_lp = &cur_vts_data.cur_sample->lp_data[0];
    else
        (cur_vts_data.cur_lp)++;

    if (id != cur_vts_data.cur_lp->lpid)
        tw_error(TW_LOC, "returning incorrect LP data! requesting %lu, but have %lu",
                id, cur_vts_data.cur_lp->lpid);
}

void vts_end_sample()
{
    cur_vts_data.in_progress = 0;
    cur_vts_data.cur_sample == NULL;
    cur_vts_data.cur_lp = NULL;
    cur_vts_data.cur_var = NULL;
}

void vts_save_model_variable(tw_lp* lp, const char* var_name, void* data)
{
    model_var_data *cur_var = cur_vts_data.cur_var;
    int var_id = variable_id_lookup(lp, var_name);
    cur_var->var = lp->model_types->model_vars[var_id];
    size_t type_size = model_var_type_size(cur_var->var.type);
    cur_var->data = calloc(type_size, cur_var->var.num_elems);
    memcpy(cur_var->data, data, type_size * cur_var->var.num_elems);
    (cur_vts_data.cur_var)++;
}

/**
 * @brief Save model variable sample
 *
 * To be called by the model sampling event forward handler
 */
void st_save_model_variable(tw_lp* lp, const char* var_name, void* data)
{
    if (cur_vts_data.in_progress)
    {
        // we're in a VTS sampling point
        vts_save_model_variable(lp, var_name, data);
        return;
    }

    // GVT and RT modes should do this instead
    // setup metadata for this variable in the buffer
    model_var_md* cur_var = (model_var_md*)cur_buf.cur_pos;
    cur_buf.cur_pos += sizeof(*cur_var);
    cur_buf.cur_size += sizeof(*cur_var);

    // look up var_name to find id
    cur_var->var_id  = variable_id_lookup(lp, var_name);
    cur_var->num_elems = lp->model_types->model_vars[cur_var->var_id].num_elems;
    cur_var->type = lp->model_types->model_vars[cur_var->var_id].type;
    size_t type_size = model_var_type_size(cur_var->type);
    size_t data_size = cur_var->num_elems * type_size;

    // TODO can we remove the memcpy somehow?
    memcpy(cur_buf.cur_pos, data, data_size);
    cur_buf.cur_pos += data_size;
    cur_buf.cur_size += data_size;

    if (cur_buf.cur_size > cur_buf.total_size)
        tw_error(TW_LOC, "Model data caused buffer overflow! cur_size %lu, total_size %lu\n",
                cur_buf.cur_size, cur_buf.total_size);
}

/**
 * @brief Retrieve model variable sample for reverse computation
 *
 * To be called by the model sampling event reverse handler
 */
void* st_get_model_variable(tw_lp* lp, const char* var_name, size_t* data_size)
{
    if (!cur_vts_data.in_progress)
        tw_error(TW_LOC, "Should not be returning model VTS data!");

    model_lp_sample* cur_lp = cur_vts_data.cur_lp;

    if (lp->gid != cur_lp->lpid)
        tw_error(TW_LOC, "wrong LP!");

    int var_id = variable_id_lookup(lp, var_name);
    model_var_data* var = &cur_lp->vars[var_id];
    *data_size = model_var_type_size(var->var.type) * var->var.num_elems;
    return var->data;
}
