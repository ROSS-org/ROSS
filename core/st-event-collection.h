#include <ross.h>

typedef void (*rbev_col_f) (void *msg, tw_lp *lp, char *buffer);
typedef void (*ev_col_f) (void *msg, tw_lp *lp, char *buffer);
typedef struct st_event_collect st_event_collect;

// each LP type needs to have this
// can turn on/off event collection for each LP type, but not at event level
struct st_event_collect{
    rbev_col_f rbev_col; /**< @brief function pointer to collect data about events causing rollbacks */
    size_t rbev_sz;      /**< @brief size of data collected from model about events causing rollbacks */
    ev_col_f ev_col;     /**< @brief function pointer to collect data about all events for given LP */
    size_t ev_sz;       /**< @brief size of data collected from model for each event */
};

extern int g_st_ev_rb_collect;
extern int g_st_ev_collect;
extern st_event_collect *g_st_event_types;

void st_evcol_setup_types(tw_lp *lp);
void st_evcol_settype(tw_lpid i, st_event_collect *event_types);
void st_collect_event_data(tw_event *cev, tw_stime recv_rt);
