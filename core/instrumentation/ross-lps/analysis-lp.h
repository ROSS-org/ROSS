#ifndef INC_analysis_lp_h
#define INC_analysis_lp_h

#include <ross.h>

typedef struct analysis_state analysis_state;
typedef struct analysis_msg analysis_msg;
typedef struct sim_engine_data_pe sim_engine_data_pe;
typedef struct sim_engine_data_kp sim_engine_data_kp;
typedef struct sim_engine_data_lp sim_engine_data_lp;
typedef struct model_sample_data model_sample_data;
typedef struct lp_metadata lp_metadata;

typedef enum{
    PE_TYPE,
    KP_TYPE,
    LP_TYPE,
    MODEL_TYPE
} data_type_flag;

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

struct sim_engine_data_pe
{
    tw_stime last_gvt;
    tw_stat ngvt;
    tw_stat rb_total; 
    tw_stat rb_secondary;
    tw_stat nevent_processed;
    tw_stat nevent_abort;
    tw_stat e_rbs;
    tw_stat nsend_network;
    tw_stat nread_network;
    tw_stat fc_attempts;
    tw_stat pq_qsize;
    tw_stat pe_event_ties;
    tw_stime s_net_read;
    tw_stime s_gvt;
    tw_stime s_fossil_collect;
    tw_stime s_event_abort;
    tw_stime s_event_process;
    tw_stime s_pq;
    tw_stime s_rollback;
    tw_stime s_cancel_q;
    tw_stime s_avl;
    //tw_stime s_buddy;
    //tw_stime s_lz4;
    unsigned long long max_opt_lookahead;
};

struct sim_engine_data_kp
{
    tw_stime time_ahead_gvt;
    double efficiency;
    tw_stat rb_total; 
    tw_stat rb_secondary;
    tw_stat nevent_processed;
    tw_stat e_rbs;
    tw_stat nsend_network;
    tw_stat nread_network;
};

struct sim_engine_data_lp
{
    tw_stat nevent_processed;
    tw_stat e_rbs;
    tw_stat nsend_network;
    tw_stat nread_network;
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
    sim_engine_data_pe prev_data_pe;
    sim_engine_data_kp prev_data_kp;
    sim_engine_data_lp *prev_data_lp;
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
