#ifndef INC_ross_thread_h
#define	INC_ross_thread_h

FWD(struct, tw_mutex);
FWD(struct, tw_barrier);

void tw_mutex_create(tw_mutex * lck);
void tw_thread_create(void (*fun) (void *), void *arg);
void tw_barrier_create(tw_barrier * barrier);

INLINE(void) tw_mutex_lock(tw_mutex * lck);
INLINE(void) tw_mutex_unlock(tw_mutex * lck);
INLINE(void) tw_barrier_sync(tw_barrier * barrier);

#endif
