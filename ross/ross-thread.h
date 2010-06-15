#ifndef INC_ross_thread_h
#define	INC_ross_thread_h

struct tw_mutex_tag;
typedef struct tw_mutex_tag tw_mutex;
struct tw_barrier_tag;
typedef struct tw_barrier_tag tw_barrier;

void tw_mutex_create(tw_mutex * lck);
void tw_thread_create(void (*fun) (void *), void *arg);
void tw_barrier_create(tw_barrier * barrier);

INLINE(void) tw_mutex_lock(tw_mutex * lck);
INLINE(void) tw_mutex_unlock(tw_mutex * lck);
INLINE(void) tw_barrier_sync(tw_barrier * barrier);

#endif
