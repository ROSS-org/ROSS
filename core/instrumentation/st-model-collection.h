#include <ross.h>

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
    NO_MODEL_STATS,
    MODEL_GVT,
    MODEL_RT,
    MODEL_BOTH
} model_stats_enum;

extern st_model_types *g_st_model_types;

void st_model_setup_types(tw_lp *lp);
void st_model_settype(tw_lpid i, st_model_types *model_types);
void st_collect_model_data(tw_pe *pe, tw_stime current_rt, int stats_type);
