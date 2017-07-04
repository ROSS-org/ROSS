#ifndef INC_st_instrumentation_h
#define	INC_st_instrumentation_h

/*
 * Header file for all of the ROSS instrumentation
 */

#include <ross.h>
#include <inttypes.h>

#define INST_MAX_LENGTH 1024

/*
 * st-stats-buffer.c
 */
#define st_buffer_free_space(buf) (buf->size - buf->count)
#define st_buffer_write_ptr(buf) (buf->buffer + buf->write_pos)
#define st_buffer_read_ptr(buf) (buf->buffer + buf->read_pos)

typedef struct{
    char *buffer;
    int size;
    int write_pos;
    int read_pos;
    int count;
} st_stats_buffer;

extern char stats_directory[INST_MAX_LENGTH];
extern int g_st_buffer_size;
extern int g_st_buffer_free_percent;
extern FILE *seq_ev_trace, *seq_model;

st_stats_buffer *st_buffer_init(int type);
void st_buffer_push(st_stats_buffer *buffer, char *data, int size);
void st_buffer_write(st_stats_buffer *buffer, int end_of_sim, int type);
void st_buffer_finalize(st_stats_buffer *buffer, int type);

/*
 * st-instrumentation.c
 */
typedef enum{
    GVT_COL,
    RT_COL,
    EV_TRACE,
    MODEL_COL,
    NUM_COL_TYPES
} collection_types;

extern st_stats_buffer **g_st_buffer;
extern int g_st_instrumentation;
extern int g_st_engine_stats;

extern const tw_optdef *st_inst_opts();
extern void st_inst_init(void);
extern void st_stats_init();
extern void st_inst_dump();
extern void st_inst_finalize(tw_pe *me);


/*
 * st-sim-engine.c
 * Simulation Engine related instrumentation
 */
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

extern char g_st_stats_out[INST_MAX_LENGTH];
extern int g_st_gvt_sampling;
extern int g_st_num_gvt;
extern int g_st_rt_sampling;
extern int g_st_disable_out;
extern int g_st_granularity;
extern tw_clock g_st_stat_write_ctr;
extern tw_clock g_st_stat_comp_ctr;
extern tw_clock g_st_rt_interval;
extern tw_clock g_st_rt_samp_start_cycles;
extern int g_st_model_stats;

extern st_cycle_counters last_cycle_counters;
extern st_event_counters last_event_counters;

void print_sim_engine_metadata(FILE *file);
extern void st_collect_data(tw_pe *pe, tw_stime current_rt);
void st_collect_time_ahead_GVT(tw_pe *me, char *data_size);
void st_collect_cycle_counters(tw_pe *pe, char *data);
void st_collect_event_counters_pes(tw_pe *pe, char *data);
void st_collect_event_counters_lps(tw_pe *pe, char *data);
void st_gvt_log(tw_pe *me, tw_stime gvt, tw_statistics *s, tw_stat all_reduce_cnt);
void st_gvt_log_pes(tw_pe *me, tw_stime gvt, tw_statistics *s, tw_stat all_reduce_cnt);
void st_gvt_log_lps(tw_pe *me, tw_stime gvt, tw_statistics *s, tw_stat all_reduce_cnt);

/*
 * st-event-trace.c
 */
typedef enum{
    NO_TRACE,
    FULL_TRACE,
    RB_TRACE,
    COMMIT_TRACE
} traces_enum;

// collect_flag allows for specific events to be turned on/off in tracing
typedef void (*rbev_trace_f) (void *msg, tw_lp *lp, char *buffer, int *collect_flag);
typedef void (*ev_trace_f) (void *msg, tw_lp *lp, char *buffer, int *collect_flag);


extern int g_st_ev_trace;
extern int g_st_buf_size;

void st_collect_event_data(tw_event *cev, tw_stime recv_rt, tw_stime duration);

/*
 * st-model-data.c
 */
// function to be implemented in LP for collection of model level stats
typedef void (*model_stat_f) (void *sv, tw_lp *lp, char *buffer);
typedef struct st_model_types st_model_types;

/*
 * Struct to help ROSS collect model-level data
 * */
struct st_model_types {
    rbev_trace_f rbev_trace; /**< @brief function pointer to collect data about events causing rollbacks */
    size_t rbev_sz;          /**< @brief size of data collected from model about events causing rollbacks */
    ev_trace_f ev_trace;     /**< @brief function pointer to collect data about all events for given LP */
    size_t ev_sz;            /**< @brief size of data collected from model for each event */
    model_stat_f model_stat_fn;            /**< @brief function pointer to collect model level data */
    size_t mstat_sz;         /**< @brief size of data collected from model at sampling points */
};

typedef enum{
    NO_STATS,
    GVT_STATS,
    RT_STATS,
    BOTH_STATS
} stats_types_enum;

extern st_model_types *g_st_model_types;

void st_model_setup_types(tw_lp *lp);
void st_model_settype(tw_lpid i, st_model_types *model_types);
void st_collect_model_data(tw_pe *pe, tw_stime current_rt, int stats_type);

#endif
