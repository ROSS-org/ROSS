#ifndef INC_thread_pthread_h
#define	INC_thread_pthread_h

#include <pthread.h>

struct tw_mutex
{
	pthread_mutex_t	lock;
};
#define TW_MUTEX_INIT { PTHREAD_MUTEX_INITIALIZER }

struct tw_barrier
{
	pthread_mutex_t lock;
	tw_peid n_clients;
	tw_peid n_waiting;
	tw_volatile int phase;
};

static inline void 
tw_mutex_lock(tw_mutex *lck)
{
	while(pthread_mutex_trylock(&lck->lock) == EBUSY);
}

static inline void 
tw_mutex_unlock(tw_mutex *lck)
{
	pthread_mutex_unlock(&lck->lock);
}

static inline void 
tw_barrier_sync(tw_barrier * b)
{
	tw_volatile int my_phase;
	unsigned int	usecs = 0;

	pthread_mutex_lock(&(b->lock));

	my_phase = b->phase;
	if (++b->n_waiting == b->n_clients)
	{
		b->n_waiting = 0;
		b->phase = 1 - my_phase;
	}

	pthread_mutex_unlock(&(b->lock));

	while (b->phase == my_phase)
		usleep(usecs);
}

#endif
