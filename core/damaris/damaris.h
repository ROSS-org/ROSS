#include <ross.h>

typedef struct {
    float s_net_read;
    float s_gvt;
    float s_fossil_collect;
    float s_event_abort;
    float s_event_process;
    float s_pq;
    float s_rollback;
    float s_cancel_q;
    float s_avl;
    float s_buddy;
    float s_lz4;
} damaris_cycle_counters;

typedef struct {
    int s_pq_qsize;
    int pe_event_ties;
    int fc_attempts;
    float efficiency;

} damaris_pe_counters;

typedef struct {
    int *s_rb_total;
    int *s_rb_primary;
    int *s_rb_secondary;
} damaris_kp_counters;

typedef struct {
    int *s_nevent_processed;
    int *s_e_rbs;
    int *s_net_events;
    int *s_nsend_network;
    int *s_nread_network;
    int *s_nsend_net_remote;
} damaris_lp_counters;

extern void st_set_damaris_parameters(int num_lp);
extern void st_ross_damaris_init();
extern void st_ross_damaris_finalize();
extern void st_expose_gvt_data_damaris(tw_pe *me, tw_stime gvt, tw_statistics *s);
