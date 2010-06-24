#include <ross.h>

void
tw_mutex_create(tw_mutex *lck)
{
	pthread_mutex_init(&lck->lock, NULL);
}

struct thread_ctx
{
	void (*fun)(void*);
	void *arg;
};

static void*
run(void * arg)
{
	struct thread_ctx *ctx = arg;
	ctx->fun(ctx->arg);
	return NULL;
}

void
tw_thread_create(void (*fun)(void*), void *arg)
{
	pthread_t	tid;
	struct thread_ctx *ctx;

	ctx = tw_calloc(TW_LOC, "pthread", sizeof(*ctx), 1);
	ctx->fun = fun;
	ctx->arg = arg;

	if (pthread_create(&tid, NULL, run, ctx))
		tw_error(TW_LOC, "Could not create POSIX thread.");
}

void
tw_barrier_create(tw_barrier * b)
{
	pthread_mutex_init(&(b->lock), NULL);
	b->n_clients = g_tw_npe;
	b->n_waiting = 0;
	b->phase = 0;
}
