#ifndef INC_st_instrumentation_h
#define	INC_st_instrumentation_h

/*
 * Header file for all of the ROSS instrumentation
 */

#include <ross.h>
#include <inttypes.h>

#define INST_MAX_LENGTH 4096

/* st-stats-buffer.c */
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
extern FILE *seq_ev_trace, *seq_model, *seq_analysis;

void st_buffer_allocate();
void st_buffer_init(int type);
void st_buffer_push(int type, char *data, int size);
void st_buffer_write(int end_of_sim, int type);
void st_buffer_finalize(int type);

/* st-instrumentation.c */
typedef struct sample_metadata sample_metadata;

typedef enum{
    GVT_COL,
    RT_COL,
    ANALYSIS_LP,
    EV_TRACE,
    MODEL_COL,
    NUM_COL_TYPES
} collection_types;

typedef enum{
    PE_TYPE,
    KP_TYPE,
    LP_TYPE,
    MODEL_TYPE
} inst_data_types;

typedef enum {
    GRAN_PE,
    GRAN_KP,
    GRAN_LP,
    GRAN_ALL
} granularity_types;

struct sample_metadata
{
    int flag; 
    int sample_sz;
    tw_stime ts;
    tw_stime real_time;
};

extern char g_st_stats_out[INST_MAX_LENGTH];
extern char g_st_stats_path[INST_MAX_LENGTH];
extern int g_st_pe_data;
extern int g_st_kp_data;
extern int g_st_lp_data;
extern int g_st_disable_out;

extern int g_st_model_stats;
extern int g_st_engine_stats;

extern int g_st_gvt_sampling;
extern int g_st_num_gvt;

extern int g_st_rt_sampling;
extern tw_clock g_st_rt_interval;
extern tw_clock g_st_rt_samp_start_cycles;

extern const tw_optdef *st_inst_opts();
extern void st_inst_init(void);
extern void st_inst_dump();
extern void st_inst_finalize(tw_pe *me);

/*
 * st-sim-engine.c
 * Simulation Engine related instrumentation
 */
typedef struct st_pe_stats st_pe_stats;
typedef struct st_kp_stats st_kp_stats;
typedef struct st_lp_stats st_lp_stats;

struct st_pe_stats{
    unsigned int peid;

    unsigned int s_nevent_processed;
    unsigned int s_nevent_abort;
    unsigned int s_e_rbs;
    unsigned int s_rb_total;
    unsigned int s_rb_secondary;
    unsigned int s_fc_attempts;
    unsigned int s_pq_qsize;
    unsigned int s_nsend_network;
    unsigned int s_nread_network;
    //unsigned int s_nsend_remote_rb;
    //unsigned int s_nsend_loc_remote;
    //unsigned int s_nsend_net_remote;
    unsigned int s_ngvts;
    unsigned int s_pe_event_ties;
    unsigned int all_reduce_count;
    float efficiency;

    float s_net_read;
    float s_net_other;
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
};

struct st_kp_stats{
    unsigned int peid;
    unsigned int kpid;

    unsigned int s_nevent_processed;
    unsigned int s_nevent_abort;
    unsigned int s_e_rbs;
    unsigned int s_rb_total;
    unsigned int s_rb_secondary;
    unsigned int s_nsend_network;
    unsigned int s_nread_network;
    float time_ahead_gvt;
    float efficiency;
};

struct st_lp_stats{
    unsigned int peid;
    unsigned int kpid;
    unsigned int lpid;

    unsigned int s_nevent_processed;
    unsigned int s_nevent_abort;
    unsigned int s_e_rbs;
    unsigned int s_nsend_network;
    unsigned int s_nread_network;
    float efficiency;
};

void st_collect_engine_data(tw_pe *me, int col_type);
void st_collect_engine_data_pes(tw_pe *pe, sample_metadata *sample_md, tw_statistics *s, int col_type);
void st_collect_engine_data_kps(tw_pe *me, tw_kp *kp, sample_metadata *sample_md, int col_type);
void st_collect_engine_data_lps(tw_lp *lp, sample_metadata *sample_md, int col_type);

/*
 * st-event-trace.c 
 */
typedef enum{
    NO_TRACE,
    FULL_TRACE,
    RB_TRACE,
    COMMIT_TRACE
} traces_enum;

typedef struct {
    unsigned int src_lp;
    unsigned int dest_lp;
    float send_vts;
    float recv_vts;
    float real_ts;
    unsigned int model_data_sz;
} st_event_data;

// collect_flag allows for specific events to be turned on/off in tracing
typedef void (*ev_trace_f) (void *msg, tw_lp *lp, char *buffer, int *collect_flag);

extern int g_st_ev_trace;

void st_collect_event_data(tw_event *cev, tw_stime recv_rt);

/*
 * ross-lps/analysis-lp.c
 */
typedef void (*sample_event_f)(void *state, tw_bf *b, tw_lp *lp, void *sample);
typedef void (*sample_revent_f)(void *state, tw_bf *b, tw_lp *lp, void *sample);
extern void specialized_lp_setup();
extern void specialized_lp_init_mapping();
extern void specialized_lp_run();
extern const tw_optdef *st_special_lp_opts(void);
extern int g_st_use_analysis_lps;
extern tw_lpid g_st_analysis_nlp;
extern tw_stime g_st_vt_interval;
extern tw_stime g_st_sampling_end;
extern tw_lpid g_st_total_model_lps;
extern int g_st_sample_count;

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
    ev_trace_f ev_trace;         /**< @brief function pointer to collect data about all events for given LP */
    size_t ev_sz;                /**< @brief size of data collected from model for each event */
    model_stat_f model_stat_fn;  /**< @brief function pointer to collect model level data for RT and GVT-based instrumentation */
    size_t mstat_sz;             /**< @brief size of data collected from model at sampling points */
    sample_event_f sample_event_fn;
    sample_revent_f sample_revent_fn;
    size_t sample_struct_sz;
};

typedef enum{
    NO_STATS,
    GVT_STATS,
    RT_STATS,
    VT_STATS,
    ALL_STATS
} stats_types_enum;

typedef struct {
    unsigned int peid;
    unsigned int kpid;
    unsigned int lpid;
    float gvt;
    int stats_type;
    unsigned int model_sz;
} model_metadata;

extern st_model_types *g_st_model_types;

void st_model_setup_types(tw_lp *lp);
void st_model_settype(tw_lpid i, st_model_types *model_types);
void st_collect_model_data(tw_pe *pe, tw_stime current_rt, int stats_type);

#endif
