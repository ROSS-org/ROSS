#include <ross.h>

int g_st_ev_trace = 0;
st_trace_type *g_st_trace_types = NULL;

static int buf_size = sizeof(tw_lpid) * 2 + sizeof(tw_stime) * 2;
static short evtype_warned = 0;

// if model uses tw_lp_setup_types() to set lp->type, it will also call
// this function to set up the functions types for event collection
// because this can make use of the already defined type mapping
// TODO I don't think g_st_trace_types is ever initialized
void st_evtrace_setup_types(tw_lp *lp)
{
    if (g_st_trace_types)
        lp->trace_types = &g_st_trace_types[g_tw_lp_typemap(lp->gid)];
    else
        tw_error(TW_LOC, "g_st_trace_types needs to be defined!\n");
}

// if model uses tw_lp_settypes(), model will also need to call
// this function to set up function types for event collection
void st_evtrace_settype(tw_lpid i, st_trace_type *trace_types)
{
    if (trace_types)
    {
        tw_lp *lp = g_tw_lp[i];
        lp->trace_types = trace_types;
    }
    else if (!evtype_warned && g_tw_mynode == g_tw_masternode)
    {
        fprintf(stderr, "WARNING: node: %ld: error: %s:%i: ", g_tw_mynode, __FILE__, __LINE__);
        fprintf(stderr, "The struct st_trace_type has not been defined! No model level data will be collected\n");
        evtype_warned = 1;
    }
}

// collect src LP, dest LP, virtual time stamp, real time start
// model can implement callback function to collect model level data, e.g. event type
void st_collect_event_data(tw_event *cev, tw_stime recv_rt)
{
    int index = 0;
    int usr_sz = 0;
    int collect_flag = 1;

    if (!cev->dest_lp->trace_types && !evtype_warned && g_tw_mynode == g_tw_masternode)
    {
        fprintf(stderr, "WARNING: node: %ld: error: %s:%i: ", g_tw_mynode, __FILE__, __LINE__);
        fprintf(stderr, "The struct st_trace_type has not been defined! No model level data will be collected\n");
        evtype_warned = 1;
    }
    if (g_st_ev_trace == RB_TRACE && cev->dest_lp->trace_types)
        usr_sz = cev->dest_lp->trace_types->rbev_sz;
    else if (g_st_ev_trace == FULL_TRACE && cev->dest_lp->trace_types)
        usr_sz = cev->dest_lp->trace_types->ev_sz;

    int total_sz = buf_size + usr_sz;
    char buffer[total_sz];

    if (usr_sz > 0)
    {
        if (g_st_ev_trace == RB_TRACE)
            (*cev->dest_lp->trace_types->rbev_trace)(tw_event_data(cev), cev->dest_lp, &buffer[buf_size], &collect_flag);
        else if (g_st_ev_trace == FULL_TRACE)
            (*cev->dest_lp->trace_types->ev_trace)(tw_event_data(cev), cev->dest_lp, &buffer[buf_size], &collect_flag);
    }
    
    if (collect_flag)
    {
        memcpy(&buffer[index], &(cev->send_lp), sizeof(tw_lpid));
        index += sizeof(tw_lpid);
        memcpy(&buffer[index], &cev->dest_lp->gid, sizeof(tw_lpid));
        index += sizeof(tw_lpid);
        memcpy(&buffer[index], &cev->recv_ts, sizeof(tw_stime));
        index += sizeof(tw_stime);
        memcpy(&buffer[index], &recv_rt, sizeof(tw_stime));
        index += sizeof(tw_stime);

        if (index != buf_size)
            printf("WARNING: size of data being pushed to buffer is incorrect!");

        st_buffer_push(g_st_buffer_evrb, &buffer[0], total_sz);
    }
}

// for now we're just collecting data about what events are causing rollbacks
// so probably don't want ever roll this back
//void st_collect_event_data_rc()
//{
//}
