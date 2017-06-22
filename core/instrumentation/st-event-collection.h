#include <ross.h>

// collect_flag allows for specific events to be turned on/off in tracing
typedef void (*rbev_trace_f) (void *msg, tw_lp *lp, char *buffer, int *collect_flag);
typedef void (*ev_trace_f) (void *msg, tw_lp *lp, char *buffer, int *collect_flag);


extern int g_st_ev_trace;
extern int g_st_buf_size;

void st_collect_event_data(tw_event *cev, tw_stime recv_rt, tw_stime duration);
