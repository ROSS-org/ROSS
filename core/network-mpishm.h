#ifndef INC_network_mpishm_h
#define INC_network_mpishm_h
#include<pthread.h>

extern MPI_Comm MPI_COMM_ROSS;

typedef unsigned int tw_eventid;
typedef long tw_node;

extern void *network_mpishm_shared_memory_pool;      /* shm memory to be allocated on each node */

typedef pthread_mutex_t	tw_mutex;

static inline void
tw_mutex_lock(tw_mutex *lck)
{
    while(pthread_mutex_trylock((pthread_mutex_t*)lck) == EBUSY);
}

static inline void
tw_mutex_unlock(tw_mutex *lck)
{
	pthread_mutex_unlock((pthread_mutex_t*)lck);
}

#endif
