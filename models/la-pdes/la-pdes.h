#ifndef INC_lapdes_h
#define INC_lapdes_h

#include <ross.h>

/*
 * LA-PDES Types
 */

typedef struct lapdes_state lapdes_state;
typedef struct lapdes_message lapdes_message;

enum {
    SEND,
    RECV
};

struct lapdes_state
{
    tw_stime local_intersend_delay;
    unsigned int ops;
    unsigned int active_elements;
    unsigned int list_size;
    double *list;
    double q_target;
    unsigned int q_size;
    tw_stime last_scheduled;
    unsigned long long send_count;
    unsigned long long recv_count;
    unsigned long long ops_max;
    unsigned long long ops_min;
    unsigned long long ops_mean;
    
};

struct lapdes_message
{
    int event_type;
};

/*
 * LA-PDES Globals
 */
static unsigned int n_ent = 10;
static unsigned int s_ent = 10000;
static double q_avg = 1.0;
static double p_recv = 0.0;
static double p_send = 0.0;
static unsigned int invert = 0;
static unsigned int m_ent = 1000;
static double p_list = 0;
static double ops_ent = 1000;
static double ops_sigma = 0;
static double cache_friendliness = 0.5;
static unsigned int init_seed = 1;
static unsigned int time_bins = 10;
static unsigned int compute_end = 0;
static double min_delay = 1.0;

static unsigned long target_global_sends;
static double r_min;

#endif
