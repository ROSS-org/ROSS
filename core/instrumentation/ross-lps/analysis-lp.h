#ifndef INC_analysis_lp_h
#define INC_analysis_lp_h

#include <ross.h>

typedef struct analysis_state analysis_state;
typedef struct analysis_msg analysis_msg;
typedef struct model_sample_data model_sample_data;
typedef struct lp_metadata lp_metadata;

typedef enum{
    ALP_NONE,
    ALP_FULL, // LP, KP, and PE
    ALP_KP, // KP and PE
    ALP_PE, //PE only
    ALP_MODEL
} analysis_type_flag;

struct analysis_msg
{
    tw_lpid src;
    tw_stime timestamp;
    void *rc_data;
    size_t offset;
};

struct lp_metadata
{
    tw_lpid lpid;
    tw_kpid kpid;
    tw_peid peid;
    tw_stime ts;
    tw_stime real_time;
    int sample_sz;
    int flag; // 0 == PE, 1 == KP, 2 == LP, 3 == model
};

typedef struct model_var_data
{
    st_model_var var;
    void *data;
} model_var_data;

typedef struct model_lp_sample
{
    const char* lp_name;
    tw_lpid lpid;
    int num_vars;
    model_var_data *vars;
} model_lp_sample;

// sample for all entities at a given sampling time
struct model_sample_data
{
    model_sample_data *prev;
    model_sample_data *next;
    tw_stime vts;
    tw_stime rts;
    int num_lps;
    model_lp_sample *lp_data;          /* data for each LP on the associated KP at this sampling point */
};

struct analysis_state
{
    tw_lpid analysis_id; // id among analysis LPs only
    int num_lps;
    int num_lps_sim;
    tw_lpid *lp_list; // list of LPs that the analysis LP is responsible for
    tw_lpid *lp_list_sim;
    model_sample_data *model_samples_head;
    model_sample_data *model_samples_current;
    model_sample_data *model_samples_tail;
    tw_statistics last_pe_stats;
    int event_id;
    size_t model_sample_size;
};

void analysis_init(analysis_state *s, tw_lp *lp);
void analysis_event(analysis_state *s, tw_bf *bf, analysis_msg *m, tw_lp *lp);
void analysis_event_rc(analysis_state *s, tw_bf *bf, analysis_msg *m, tw_lp *lp);
void analysis_commit(analysis_state *s, tw_bf *bf, analysis_msg *m, tw_lp *lp);
void analysis_finish(analysis_state *s, tw_lp *lp);
void collect_sim_engine_data(tw_pe *pe, tw_lp *lp, analysis_state *s, tw_stime current_rt);
tw_peid analysis_map(tw_lpid gid);
tw_lpid *get_sim_lp_list(analysis_state *s, int* num_lps);
size_t get_model_data_size(analysis_state *s, int* num_lps);

extern tw_lpid analysis_start_gid;
void st_analysis_lp_settype(tw_lpid lpid);
#endif
