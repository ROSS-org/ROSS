#ifndef INC_thread_none_h
#define INC_thread_none_h

struct tw_mutex
struct tw_barrier
#define TW_MUTEX_INIT {}

static inline void 
tw_mutex_lock(tw_mutex * lck)
{
}

static inline void 
tw_mutex_unlock(tw_mutex * lck)
{
}

static inline void 
tw_barrier_sync(tw_barrier * barrier)
{
}

#endif
