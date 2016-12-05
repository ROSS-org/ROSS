#include <ross.h>

// collect_flag allows for specific events to be turned on/off in tracing
typedef void (*rbev_trace_f) (void *msg, tw_lp *lp, char *buffer, int *collect_flag);
typedef void (*ev_trace_f) (void *msg, tw_lp *lp, char *buffer, int *collect_flag);
typedef struct st_trace_type st_trace_type;

// each LP type needs to have this
// can turn on/off event collection for each LP type, but not at event level
struct st_trace_type {
    rbev_trace_f rbev_trace; /**< @brief function pointer to collect data about events causing rollbacks */
    size_t rbev_sz;      /**< @brief size of data collected from model about events causing rollbacks */
    ev_trace_f ev_trace;     /**< @brief function pointer to collect data about all events for given LP */
    size_t ev_sz;       /**< @brief size of data collected from model for each event */
};

extern int g_st_ev_trace;
extern st_trace_type *g_st_trace_types;
extern int g_st_buf_size;

void st_evtrace_setup_types(tw_lp *lp);
void st_evtrace_settype(tw_lpid i, st_trace_type *event_types);
void st_collect_event_data(tw_event *cev, tw_stime recv_rt, tw_stime duration);
