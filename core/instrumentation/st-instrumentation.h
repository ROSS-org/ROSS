#ifndef ST_INSTRUMENTATION_H
#define ST_INSTRUMENTATION_H

/*
 * Header file for all of the ROSS instrumentation
 */

#include <ross.h>
#include <inttypes.h>

#define INST_MAX_LENGTH 4096

/**
 * @brief Callback function for event tracing
 * @param[in] msg Pointer to the event to be traced
 * @param[in] lp Pointer to the LP this event was sent to
 * @param[in] buffer Pointer to buffer space where event data should be saved
 * @param[out] collect_flag Set to 0 if this event should not be collected (set to 1 by default)
 */
typedef void (*ev_trace_f) (void *msg, tw_lp *lp, char *buffer, int *collect_flag);

/**
 * @brief Callback function for collecting model data in RT and GVT-based sampling modes
 *
 * @param[in] sv Pointer to the LP's state
 * @param[in] lp Pointer to the LP
 *
 * Note: Model data collected with this function is not guaranteed to be causally correct.
 */
typedef void (*rt_event_f) (void *sv, tw_lp *lp);

/**
 * @brief Callback function for collecting model data in VT sampling mode
 *
 * @param[in] state Pointer to the LP's state
 * @param[in] bf Pointer to bitfield to be used for rollbacks
 * @param[in] lp Pointer to the LP
 *
 */
typedef void (*vts_event_f)(void *state, tw_bf *b, tw_lp *lp);
typedef void (*vts_revent_f)(void *state, tw_bf *b, tw_lp *lp);

typedef enum st_model_typename
{
    MODEL_INT,
    MODEL_LONG,
    MODEL_FLOAT,
    MODEL_DOUBLE,
    MODEL_OTHER
} st_model_typename;

typedef struct st_model_var
{
    const char* var_name;     /**< @brief name of model variable */
    st_model_typename type;   /**< @brief data type for this model variable */
    int num_elems;   /**< @brief number of elements for this variable */
} st_model_var;

/**
 * Struct to help ROSS collect model-level data
 * */
typedef struct st_model_types st_model_types;
struct st_model_types {
    // lp_name and each var_name should be null terminated
    const char* lp_name;
    st_model_var *model_vars;
    int num_vars;
    vts_event_f vts_event_fn;
    vts_revent_f vts_revent_fn;
    rt_event_f rt_event_fn;  /**< @brief function pointer to collect model level data for RT and GVT-based instrumentation */
    ev_trace_f ev_trace;         /**< @brief function pointer to collect data about all events for given LP */
    size_t ev_sz;                /**< @brief size of data collected from model for each event */
};

/**
 * @brief Set up model_types for the given LP
 *
 * This is called by tw_lp_setup_types() and does not need to be directly called by the user.
 * If the model uses tw_lp_settype() instead, see documentation for st_model_settype().
 * Requires that g_st_model_types is defined.
 *
 * @param[in] lp LP for which to set model_types
 */
void st_model_setup_types(tw_lp *lp);

/**
 * @brief Set up model_types for the given LP
 *
 * If the model uses tw_lp_settype(), use this function to also set up model_types.
 *
 * @param[in] i local id of LP
 * @param[in] model_types pointer to st_model_types for specified LP
 */
void st_model_settype(tw_lpid i, st_model_types *model_types);
void st_save_model_variable(tw_lp* lp, const char* var_name, void* data);
void* st_get_model_variable(tw_lp* lp, const char* var_name, size_t* data_size);

/* st-instrumentation.c */
extern int g_st_model_stats;
extern int g_st_pe_data;
extern int g_st_kp_data;
extern int g_st_lp_data;
extern int g_st_num_gvt;
extern int g_st_rt_sampling;
extern tw_clock g_st_rt_interval;
extern tw_clock g_st_rt_samp_start_cycles;

const tw_optdef *st_inst_opts();
void st_inst_init(void);
void st_inst_finish_setup(void);
void st_inst_finalize(tw_pe *me);
void st_inst_sample(tw_pe *me, int inst_type);

typedef enum {
    GVT_INST,
    RT_INST,
    VT_INST,
    ET_INST,
    NUM_INST_MODES
} inst_modes;

typedef enum{
    NO_STATS,
    GVT_STATS,
    RT_STATS,
    VT_STATS,
    ALL_STATS
} stats_types_enum;

extern short engine_modes[NUM_INST_MODES];
extern short model_modes[NUM_INST_MODES];

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
    unsigned int s_rb_primary;
    unsigned int s_rb_secondary;
    unsigned int s_fc_attempts;
    unsigned int s_pq_qsize;
    unsigned int s_nsend_network;
    unsigned int s_nread_network;
    unsigned int s_ngvts;
    unsigned int s_pe_event_ties;
    unsigned int all_reduce_count;
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
    unsigned int kpid;
    unsigned int s_nevent_processed;
    unsigned int s_nevent_abort;
    unsigned int s_e_rbs;
    unsigned int s_rb_total;
    unsigned int s_rb_primary;
    unsigned int s_rb_secondary;
    unsigned int s_nsend_network;
    unsigned int s_nread_network;
    float time_ahead_gvt;
};

struct st_lp_stats{
    unsigned int kpid;
    unsigned int lpid;
    unsigned int s_nevent_processed;
    unsigned int s_nevent_abort;
    unsigned int s_e_rbs;
    unsigned int s_nsend_network;
    unsigned int s_nread_network;
};

/*
 * st-event-trace.c 
 */
typedef enum{
    NO_TRACE,
    FULL_TRACE,
    RB_TRACE,
    COMMIT_TRACE
} traces_enum;


extern int g_st_ev_trace;

void st_collect_event_data(tw_event *cev, tw_stime recv_rt);

/*
 * ross-lps/analysis-lp.c
 */
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

typedef struct {
    unsigned int peid;
    unsigned int kpid;
    unsigned int lpid;
    float gvt;
    int stats_type;
    unsigned int model_sz;
} model_metadata;

#endif // ST_INSTRUMENTATION_H
