#ifndef INC_analysis_lp_h
#define INC_analysis_lp_h

#include <ross.h>

typedef struct analysis_state analysis_state;
typedef struct analysis_msg analysis_msg;
//typedef struct sim_engine_data sim_engine_data;
typedef struct model_sample_data model_sample_data;
typedef struct lp_metadata lp_metadata;

struct analysis_state
{
    tw_lpid analysis_id; // id among analysis LPs only
    int num_lps;
    tw_lpid *lp_list; // list of LPs that the analysis LP is responsible for
    model_sample_data *model_samples;
    int start_sample;
    int current_sample;
    unsigned long long sample_sz;
};

struct analysis_msg
{
    tw_lpid src;
    tw_stime timestamp;

};

struct lp_metadata
{
    tw_lpid lpid;
    tw_stime ts;
    int64_t sample_sz;
};

//struct sim_engine_data
//{
//
//};

struct model_sample_data
{
    tw_stime timestamp;
    void **lp_data;          /* data for each LP on the associated KP at this sampling point */
};

void analysis_init(analysis_state *s, tw_lp *lp);
void analysis_event(analysis_state *s, tw_bf *bf, analysis_msg *m, tw_lp *lp);
void analysis_event_rc(analysis_state *s, tw_bf *bf, analysis_msg *m, tw_lp *lp);
void analysis_commit(analysis_state *s, tw_bf *bf, analysis_msg *m, tw_lp *lp);
void analysis_finish(analysis_state *s, tw_lp *lp);
tw_peid analysis_map(tw_lpid gid);

extern tw_lpid analysis_start_gid;
void st_analysis_lp_settype(tw_lpid lpid);
#endif
