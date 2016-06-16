#include <ross.h>

typedef struct {
    tw_clock s_net_read;
    tw_clock s_gvt;
    tw_clock s_fossil_collect;
    tw_clock s_event_abort;
    tw_clock s_event_process;
    tw_clock s_pq;
    tw_clock s_rollback;
    tw_clock s_cancel_q;
    tw_clock s_avl;
    tw_clock s_buddy;
    tw_clock s_lz4;
} st_cycle_counters;

typedef struct {
    tw_stat s_net_events;
    tw_stat s_nevent_processed;
    tw_stat s_nevent_abort;
    tw_stat s_e_rbs;

    tw_stat s_rb_total;
    tw_stat s_rb_primary;
    tw_stat s_rb_secondary;
    tw_stat s_fc_attempts;

    tw_stat s_pq_qsize;
    tw_stat s_nsend_network;
    tw_stat s_nread_network;
    tw_stat s_nsend_remote_rb;

    tw_stat s_nsend_loc_remote;
    tw_stat s_nsend_net_remote;
    tw_stat s_pe_event_ties;

    tw_stat s_ngvts;  
} st_event_counters;

extern const tw_optdef *tw_stats_setup();
extern void tw_gvt_stats_file_setup(tw_peid id);
extern void tw_interval_stats_file_setup(tw_peid id);
extern char g_st_stats_out[128]; 
extern int g_st_stats_enabled;
extern long g_st_time_interval;
extern int g_st_pe_per_file;
extern int g_st_my_file_id;
extern MPI_File gvt_file;
extern MPI_File interval_file;
extern MPI_Comm  stats_comm;
extern int g_st_disable_out;

st_cycle_counters last_cycle_counters;
st_event_counters last_event_counters;

extern void st_collect_data(tw_pe *pe, tw_stime current_rt);
void st_collect_time_ahead_GVT(tw_pe *me, char *data_size);
void st_collect_cycle_counters(tw_pe *pe, char *data);
void st_collect_event_counters(tw_pe *pe, char *data);
void st_collect_memory_usage(char *data);
void st_stats_init();
