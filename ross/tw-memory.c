#include <ross.h>

#if 0
/*
Garrett points out that since the mem qs are on the KP now, we do not need to put 
mem bufs onto both a RC Q and a processed mem queue since the KP will always roll back
properly, and take the events/mem bufs with it.
*/
#endif

#define ROSS_VERIFY_MEMORY 0

/*
 * Use this function to allocate a memory buffer in model event handlers.
 */
tw_memory         *
tw_memory_alloc(tw_lp * lp, tw_fd fd)
{
	tw_kp			*kp;
	tw_memoryq		*q;
	tw_memory		*rv;

	kp = lp->kp;

	q = tw_kp_getqueue(kp, fd);

	/*
	 * First try to fossil collect.. then allocate more
	 */	
	if(!q->size)
		tw_kp_fossil_memory(kp);

	if(!q->size)
	{
#if 1
		kp->s_mem_buffers_used += tw_memory_allocate(q);
#endif

#if 1
		printf("Allocating %ld bytes in memory fd: %ld \n", 
			((sizeof(tw_memory) + q->d_size) * q->start_size * q->grow), 
			fd);
#endif
	}

	if(NULL == (rv = tw_memoryq_pop(q)))
		tw_error(TW_LOC, "No memory buffers to allocate!");

	rv->next = rv->prev = NULL;
	rv->lp_next = NULL;

	//printf("%ld: mem alloc, remain: %d, fd %ld \n", lp->id, q->size, fd);

	return rv;
}

/*
 * The RC of an alloc is to place the buffer back into the 
 * free_q of memory buffers -- this may be bad to rely on the
 * model to call this method.. but I cannot see any other way to
 * give the model a chance to fix their space up first.
 *
 * Not in pmemory_q or in rc_memory_q
 */
void
tw_memory_alloc_rc(tw_lp * lp, tw_memory * m, tw_fd fd)
{
	tw_memoryq_push(tw_kp_getqueue(lp->kp, fd), m);
}

void
tw_memory_free(tw_lp * lp, tw_memory * m, tw_fd fd)
{
	tw_kp		*kp;
	tw_memoryq	*q;

	kp = lp->kp;

	/* If seq sim, just reclaim buffer now. */
	if((tw_nnodes() * g_tw_npe) == 1)
		q = tw_kp_getqueue(kp, fd);
	else
		q = tw_kp_getqueue(kp, fd + 1);

	m->ts = tw_now(lp);
	m->bit = 1;

	/*
	 * Now we need to link this buffer into the LPs queue
	 * for RC memory and the processed memory queue.
	 */
	tw_memoryq_push(q,  m);

	//printf("%d: mem free, remain: %d, fd %d \n", lp->id, q->size, fd);
}

tw_memory         *
tw_memory_free_rc(tw_lp * lp, tw_fd fd)
{
	tw_memoryq	*q;
	tw_memory	*m;

	q = tw_kp_getqueue(lp->kp, fd+1);

	/*
	 * We also need to unthread this event from the PE
	 * processed memory Q
	 */
	m = tw_memoryq_pop(q);
	m->bit = 0;

	if(m && m->ts < lp->pe->GVT)
	{
		tw_error(TW_LOC, "You are attempting to rollback a buffer"
				 " with ts (%g) < GVT (%g)!",
				 m->ts, lp->pe->GVT);
	}

	return m;
}

void
tw_memory_free_single(tw_kp * kp, tw_memory * m, tw_fd fd)
{
	m->bit = 0;
	tw_memoryq_push(tw_kp_getqueue(kp, fd),  m);
}

#if 0
void
tw_memory_free_list(tw_pe * pe, tw_memory * h, tw_memory * t, int cnt, tw_fd fd)
{
	tw_memoryq_push_list(tw_pe_getqueue(pe, fd - 1), (tw_memory *)h, (tw_memory *)t, cnt);
}
#endif

size_t
tw_memory_getsize(tw_kp * kp, int fd)
{
	return tw_kp_getqueue(kp, fd)->d_size + g_tw_memory_sz;
}

size_t
tw_memory_allocate(tw_memoryq * q)
{
	tw_memory	*head;
	tw_memory	*tail;

	void           *d;
	size_t		mem_sz;
	size_t		mem_len;

	size_t          align;

	unsigned 	cnt;

	align = max(sizeof(double), sizeof(void *));
        mem_len = sizeof(tw_memory) + q->d_size;

        if(mem_len & (align - 1))
                mem_len += align - (mem_len & (align - 1));

	mem_sz = floor(mem_len * q->start_size * q->grow);

	q->size += floor(q->start_size * q->grow);
	q->grow *= q->grow;

#if ROSS_VERIFY_MEMORY
	printf("Allocating %d bytes in memory subsystem...", mem_sz);
#endif

	d = head = tail = tw_calloc(TW_LOC, "Memory Queue", mem_sz, 1);

	if (!d)
		tw_error(TW_LOC, "\nCannot allocate %u events! \n", q->size);

	cnt = q->size;
	while (--cnt)
	{
		tail->next = (tw_memory *) (((char *)tail) + mem_len);
		tail->prev = (tw_memory *) (((char *)tail) - mem_len);
		tail = tail->next;
	}

	head->prev = tail->next = NULL;

	if(q->head == NULL)
	{
		q->head = head;
		q->tail = tail;
	} else
	{
		q->tail->next = head;
		head->prev = q->tail;
		q->tail = tail;
	}

#if ROSS_VERIFY_MEMORY
	printf("successful! \n");
#endif

	return q->start_size * q->grow;
}
