#include <ross.h>

// This is the API for the tw queue system
// There are several queue implementations

tw_pq *tw_pq_create(void);
void tw_pq_enqueue(tw_pq *, tw_event *);
tw_event *tw_pq_dequeue(tw_pq *);
tw_stime tw_pq_minimum(tw_pq *);
void tw_pq_delete_any(tw_pq *, tw_event *);
unsigned int tw_pq_get_size(tw_pq *);
unsigned int tw_pq_max_size(tw_pq *);
#ifdef ROSS_QUEUE_kp_splay
tw_eventpq * tw_eventpq_create(void);
#endif

