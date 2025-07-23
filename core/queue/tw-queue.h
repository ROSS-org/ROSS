#ifndef INC_tw_queue_h
#define INC_tw_queue_h

#include "ross-types.h"

// This is the API for the tw queue system
// There are several queue implementations

tw_pq *tw_pq_create(void);
void tw_pq_enqueue(tw_pq *, tw_event *);
tw_event *tw_pq_dequeue(tw_pq *);
tw_stime tw_pq_minimum(tw_pq *);
#ifdef USE_RAND_TIEBREAKER
/* Returns the mininum value from priority-queue. Do NOT modify or save pointer. It is safe to save *contents* of the struct that the pointer points at. */
tw_event_sig const * tw_pq_minimum_sig_ptr(tw_pq const *);
#endif
void tw_pq_delete_any(tw_pq *, tw_event *);
unsigned int tw_pq_get_size(tw_pq *);
unsigned int tw_pq_max_size(tw_pq *);
#ifdef ROSS_QUEUE_kp_splay
tw_eventpq * tw_eventpq_create(void);
#endif

#endif /* INC_tw_queue_h */
