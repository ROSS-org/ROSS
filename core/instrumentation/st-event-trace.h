#ifndef ST_EVENT_TRACE_H
#define ST_EVENT_TRACE_H

typedef struct {
    unsigned int src_lp;
    unsigned int dest_lp;
    float send_vts;
    float recv_vts;
    float real_ts;
    unsigned int model_data_sz;
} st_event_data;

#endif // ST_EVENT_TRACE_H
