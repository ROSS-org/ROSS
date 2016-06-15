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

extern void st_collect_data(tw_pe *pe, tw_stime current_rt);
void get_time_ahead_GVT(tw_pe *me, char *data_size);
void st_collect_cycle_counters(tw_pe *pe, char *data);
void st_stats_init();
