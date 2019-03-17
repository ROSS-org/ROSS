#ifndef ST_INSTRUMENTATION_INTERNAL_H
#define ST_INSTRUMENTATION_INTERNAL_H

#include <ross.h>

/* st-stats-buffer.c */
extern int g_st_buffer_size;
extern int g_st_buffer_free_percent;

/* st-instrumentation.c */
size_t engine_data_sizes[NUM_INST_MODES];
size_t model_data_sizes[NUM_INST_MODES];
int model_num_lps[NUM_INST_MODES];

typedef struct sample_metadata sample_metadata;

typedef struct file_metadata
{
    unsigned int num_pe;
    unsigned int num_kp_pe;
    int inst_mode;
} file_metadata;

struct sample_metadata
{
    double last_gvt;
    double vts;
    double rts;
    unsigned int peid;
    int has_pe;
    int has_kp;
    int has_lp;
    int has_model;
    int num_model_lps;
};

extern char g_st_stats_out[INST_MAX_LENGTH];
extern char g_st_stats_path[INST_MAX_LENGTH];
extern int g_st_disable_out;
extern void st_inst_dump();
void inst_sample(tw_pe *me, int inst_type, tw_lp* lp, int vts_commit);

extern int g_st_engine_stats;

#endif // ST_INSTRUMENTATION_INTERNAL_H
