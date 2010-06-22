#ifndef INC_ross_thread_h
#define	INC_ross_thread_h

typedef struct tw_mutex tw_mutex;
typedef struct tw_barrier tw_barrier;

void tw_mutex_create(tw_mutex * lck);
void tw_thread_create(void (*fun) (void *), void *arg);
void tw_barrier_create(tw_barrier * barrier);

static inline void  tw_mutex_lock(tw_mutex * lck);
static inline void  tw_mutex_unlock(tw_mutex * lck);
static inline void  tw_barrier_sync(tw_barrier * barrier);

#endif
