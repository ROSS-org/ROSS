#include <ross.h>

int g_st_ev_rb_collect = 0;
static int buf_size = sizeof(tw_lpid) * 2 + sizeof(tw_stime) * 2; 

// collect src LP, dest LP, virtual time stamp, real time start
void st_collect_event_data(tw_event *cev, tw_stime recv_rt)
{
    // add in callback function to be implemented by user so we can get event type (and potentially other data the user may want)
    // TODO need to change from sending peid to sending lpid
    // probably just have to add something to tw_event to track it
    int index = 0;
    char buffer[buf_size];
    
    memcpy(&buffer[index], &(cev->send_lp), sizeof(tw_peid));
    index += sizeof(tw_lpid);
    memcpy(&buffer[index], &cev->dest_lp->gid, sizeof(tw_lpid));
    index += sizeof(tw_lpid);
    memcpy(&buffer[index], &cev->recv_ts, sizeof(tw_stime));
    index += sizeof(tw_stime);
    memcpy(&buffer[index], &recv_rt, sizeof(tw_stime));
    index += sizeof(tw_stime);

    if (index != buf_size)
        printf("WARNING: size of data being pushed to buffer is incorrect!");

    st_buffer_push(g_st_buffer_evrb, &buffer[0], buf_size);
}

// for now we're just collecting data about what events are causing rollbacks
// so probably don't want ever roll this back
//void st_collect_event_data_rc()
//{
//}
