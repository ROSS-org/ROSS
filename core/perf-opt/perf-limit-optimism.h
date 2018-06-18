#ifndef INC_perf_limit_optimism_h
#define INC_perf_limit_optimism_h

#include <ross.h>

// keep track of limiting optimism
typedef enum{
    LIMIT_OFF,
    LIMIT_RECOVER,
    LIMIT_ON,
    LIMIT_AGGRESSIVE,
} limit_opt_states;

typedef enum {
    EFF_MIN,
    EFF_MAX,
    EFF_MEAN,
    TIME_MIN,
    TIME_MAX,
    TIME_MEAN,
    NUM_RED_ELEM
} reduction_data_elem;

typedef struct pe_data pe_data;
struct pe_data
{
    tw_stat *nevent_processed;
    tw_stat *e_rbs;
};

extern int g_perf_disable_opt;

extern const tw_optdef *perf_opts(void);
extern void perf_init();
extern void perf_adjust_optimism(tw_pe *pe);
extern void perf_custom_reduction(double *invec, double *inoutvec, int *len, MPI_Datatype *dt);
extern void perf_finalize();

#endif
