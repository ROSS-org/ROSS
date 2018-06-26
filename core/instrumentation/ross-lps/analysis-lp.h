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

struct model_sample_data
{
    model_sample_data *prev;
    model_sample_data *next;
    tw_stime timestamp;
    void **lp_data;          /* data for each LP on the associated KP at this sampling point */
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
};

void analysis_init(analysis_state *s, tw_lp *lp);
void analysis_event(analysis_state *s, tw_bf *bf, analysis_msg *m, tw_lp *lp);
void analysis_event_rc(analysis_state *s, tw_bf *bf, analysis_msg *m, tw_lp *lp);
void analysis_commit(analysis_state *s, tw_bf *bf, analysis_msg *m, tw_lp *lp);
void analysis_finish(analysis_state *s, tw_lp *lp);
void collect_sim_engine_data(tw_pe *pe, tw_lp *lp, analysis_state *s, tw_stime current_rt);
tw_peid analysis_map(tw_lpid gid);

extern tw_lpid analysis_start_gid;
void st_analysis_lp_settype(tw_lpid lpid);
#endif
