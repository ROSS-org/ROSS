#include <ross.h>

int g_st_ev_trace = 0;

static short evtype_warned = 0;

// collect src LP, dest LP, virtual time stamp, real time start
// model can implement callback function to collect model level data, e.g. event type
void st_collect_event_data(tw_event *cev, double recv_rt)
{
    tw_clock start_cycle_time = tw_clock_read();
    int collect_flag = 1;
    st_event_data ev_data;
    ev_data.src_lp = (unsigned int) cev->send_lp;
    ev_data.dest_lp = (unsigned int) cev->dest_lp->gid;
    ev_data.send_vts = (float) TW_STIME_DBL(cev->send_ts);
    ev_data.recv_vts = (float) TW_STIME_DBL(cev->recv_ts);
    ev_data.real_ts = (float) recv_rt;
    int total_sz = sizeof(ev_data);

    if (!cev->dest_lp->model_types && !evtype_warned && g_tw_mynode == g_tw_masternode)
    {
        fprintf(stderr, "WARNING: node: %ld: %s:%i: ", g_tw_mynode, __FILE__, __LINE__);
        fprintf(stderr, "The struct st_model_types has not been defined! No model level data will be collected\n");
        evtype_warned = 1;
    }

    if (cev->dest_lp->model_types && cev->dest_lp->model_types->ev_trace)
        ev_data.model_data_sz = cev->dest_lp->model_types->ev_sz;
    else
        ev_data.model_data_sz = 0;


    total_sz += ev_data.model_data_sz;
    char buffer[total_sz];

    if (ev_data.model_data_sz > 0)
        (*cev->dest_lp->model_types->ev_trace)(tw_event_data(cev), cev->dest_lp, &buffer[sizeof(ev_data)], &collect_flag);

    if (collect_flag)
    {
        memcpy(&buffer[0], &ev_data, sizeof(ev_data));
        if (g_tw_synchronization_protocol != SEQUENTIAL)
            st_buffer_push(EV_TRACE, &buffer[0], total_sz);
        else if (g_tw_synchronization_protocol == SEQUENTIAL && !g_st_disable_out)
            fwrite(buffer, total_sz, 1, seq_ev_trace);

    }
    g_tw_pe->stats.s_stat_comp += tw_clock_read() - start_cycle_time;
}
