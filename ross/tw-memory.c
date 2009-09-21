#include <ross.h>

/*
 * Use this function to allocate a memory buffer in model event handlers.
 */
tw_memory         *
tw_memory_alloc(tw_lp * lp, tw_fd fd)
{
	tw_pe			*pe;
	tw_memoryq		*q;
	tw_memory		*m;

	pe = lp->pe;

	q = tw_pe_getqueue(pe, fd);

	/*
	 * First try to fossil collect.. then allocate more
	 */	
	if(!q->size)
		tw_kp_fossil_memory(lp->kp);

	if(!q->size)
	{
#if 1
		int cnt = tw_memory_allocate(q);
		pe->stats.s_mem_buffers_used += cnt;
		printf("Allocating %d buffers in memory fd: %d \n", cnt, fd);
#else
		tw_error(TW_LOC, "Out of buffers in fd: %ld\n", fd);
#endif
	}

	if(NULL == (m = tw_memoryq_pop(q)))
		tw_error(TW_LOC, "No memory buffers to allocate!");

	m->next = m->prev = NULL;
	m->lp_next = NULL;
	m->nrefs = 1;

	//printf("%ld: mem alloc, remain: %d, fd %ld \n", lp->id, q->size, fd);

	return m;
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
	m->nrefs--;

	if(m->nrefs)
		tw_error(TW_LOC, "membuf stills has refs: %d", m->nrefs);

	tw_memoryq_push(tw_pe_getqueue(lp->pe, fd), m);
}

void
tw_memory_free(tw_lp * lp, tw_memory * m, tw_fd fd)
{
	tw_memoryq	*q;

	/* If seq sim, just reclaim buffer now. */
	if((tw_nnodes() * g_tw_npe) == 1)
		q = tw_pe_getqueue(lp->pe, fd);
	else
		q = tw_kp_getqueue(lp->kp, fd);

	m->ts = tw_now(lp);

	/*
	 * Now we need to link this buffer into the KPs
	 * processed memory queue.
	 */
	tw_memoryq_push(q, m);

	//printf("%d: mem free, remain: %d, fd %d \n", lp->id, q->size, fd);
}

tw_memory         *
tw_memory_free_rc(tw_lp * lp, tw_fd fd)
{
	tw_memoryq	*q;
	tw_memory	*m;

	q = tw_kp_getqueue(lp->kp, fd);

	/*
	 * We also need to unthread this event from the PE
	 * processed memory Q
	 */
	m = tw_memoryq_pop(q);

	if(m && m->ts < lp->pe->GVT)
	{
		tw_error(TW_LOC, "You are attempting to rollback a buffer"
				 " with ts (%g) < GVT (%g)!",
				 m->ts, lp->pe->GVT);
	}

	return m;
}

void
tw_memory_free_single(tw_pe * pe, tw_memory * m, tw_fd fd)
{
	if(m->nrefs)
		return;

	tw_memoryq_push(tw_pe_getqueue(pe, fd),  m);
}

size_t
tw_memory_getsize(tw_pe * pe, int fd)
{
	return tw_pe_getqueue(pe, fd)->d_size + g_tw_memory_sz;
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
	size_t		cnt;
	
	align = max(sizeof(double), sizeof(void *));
        mem_len = sizeof(tw_memory) + q->d_size;

        if(mem_len & (align - 1))
                mem_len += align - (mem_len & (align - 1));

	cnt = align = q->start_size + ceil(q->start_size * q->grow);
	mem_sz = mem_len * cnt;

	q->size += cnt;
	q->start_size += ceil(q->start_size * q->grow);

#if ROSS_VERIFY_MEMORY
	printf("Allocating %d bytes in memory subsystem...", mem_sz);
#endif

	d = head = tail = tw_calloc(TW_LOC, "Memory Queue", mem_sz, 1);

	if (!d)
		tw_error(TW_LOC, "\nCannot allocate %u buffers! \n", q->size);
#if ROSS_VERIFY_MEMORY
	else
		printf("Allocated %ld buffers\n", cnt);
#endif

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

	return align;
}
