#include <ross.h>

int g_st_ev_rb_collect = 0;
st_event_collect *g_st_event_types = NULL;

static int buf_size = sizeof(tw_lpid) * 2 + sizeof(tw_stime) * 2;

// if model uses tw_lp_setup_types() to set lp->type, it will also call
// this function to set up the functions types for event collection
// because this can make use of the already defined type mapping
void st_evcol_setup_types(tw_lp *lp)
{
    lp->ev_types = &g_st_event_types[g_tw_lp_typemap(lp->gid)];
}

// if model uses tw_lp_settypes(), model will also need to call
// this function to set up function types for event collection
void st_evcol_settype(tw_lpid i, st_event_collect *event_types)
{
    tw_lp *lp = g_tw_lp[i];
    lp->ev_types = event_types;
}

// collect src LP, dest LP, virtual time stamp, real time start
void st_collect_event_data(tw_event *cev, tw_stime recv_rt)
{
    // add in callback function to be implemented by user so we can get event type (and potentially other data the user may want)
    int index = 0;
    int usr_sz = 0;
    if (cev->dest_lp->ev_types)
        usr_sz = cev->dest_lp->ev_types->rbev_sz;
    int total_sz = buf_size + usr_sz;
    char buffer[total_sz];

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
    if (usr_sz > 0)
        (*cev->dest_lp->ev_types->rbev_col)(tw_event_data(cev), &buffer[index]);

    st_buffer_push(g_st_buffer_evrb, &buffer[0], total_sz);
}

// for now we're just collecting data about what events are causing rollbacks
// so probably don't want ever roll this back
//void st_collect_event_data_rc()
//{
//}
