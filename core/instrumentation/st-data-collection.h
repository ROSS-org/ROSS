#include <ross.h>

#define MAX_LENGTH 1024

typedef struct st_lp_counters st_lp_counters;

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
    unsigned int s_nevent_processed;
    unsigned int s_nevent_abort;
    unsigned int s_e_rbs;

    unsigned int s_rb_total;
    unsigned int s_rb_secondary;
    unsigned int s_fc_attempts;

    unsigned int s_pq_qsize;
    unsigned int s_nsend_network;
    unsigned int s_nread_network;

    unsigned int s_nsend_net_remote;
    unsigned int s_pe_event_ties;

    unsigned int s_ngvts;
} st_event_counters;

struct st_lp_counters{
    unsigned int s_nevent_processed;
    unsigned int s_e_rbs;

    unsigned int s_nsend_network;
    unsigned int s_nread_network;

    unsigned int s_nsend_net_remote;
};

extern const tw_optdef *tw_stats_setup();
extern char g_st_stats_out[MAX_LENGTH];
extern int g_st_stats_enabled;
extern int g_st_disable_out;
extern int g_st_granularity;
extern tw_clock g_st_stat_write_ctr;
extern tw_clock g_st_stat_comp_ctr;
extern tw_clock g_st_real_time_samp;
extern tw_clock g_st_real_samp_start_cycles;

extern st_cycle_counters last_cycle_counters;
extern st_event_counters last_event_counters;

extern void st_collect_data(tw_pe *pe, tw_stime current_rt);
void st_collect_time_ahead_GVT(tw_pe *me, char *data_size);
void st_collect_cycle_counters(tw_pe *pe, char *data);
void st_collect_event_counters_pes(tw_pe *pe, char *data);
void st_collect_event_counters_lps(tw_pe *pe, char *data);
void st_stats_init();
void st_gvt_log(tw_pe *me, tw_stime gvt, tw_statistics *s, tw_stat all_reduce_cnt);
void st_gvt_log_pes(tw_pe *me, tw_stime gvt, tw_statistics *s, tw_stat all_reduce_cnt);
void st_gvt_log_lps(tw_pe *me, tw_stime gvt, tw_statistics *s, tw_stat all_reduce_cnt);
