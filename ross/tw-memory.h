#ifndef INC_tw_memory_h
#define INC_tw_memory_h

#define TW_MEMORY_BUFFER_SIZE 500

/*
 * Use this function to allocate a memory buffer in model event handlers.
 */
static inline tw_memory * 
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
	// not certain we can rely on membuf timestamps to be sorted
	if(!q->size)
		tw_kp_fossil_memoryq(lp->kp, fd);

	if(!q->size)
	{
#if 1
		int cnt = tw_memory_allocate(q);
		pe->stats.s_mem_buffers_used += cnt;
		printf("%d: Allocating %d buffers in memory fd: %ld \n", 
			g_tw_mynode, cnt, fd);
#else
		tw_error(TW_LOC, "Out of buffers in fd: %ld\n", fd);
#endif
	}

	if(NULL == (m = tw_memoryq_pop(q)))
		tw_error(TW_LOC, "No memory buffers to allocate!");

	m->next = m->prev = NULL;
	m->nrefs = 1;

	//printf("%ld: mem alloc, remain: %d, fd %ld \n", lp->id, q->size, fd);

	return m;
}

/*
 * The RC of an alloc is to place the buffer back into the 
 * free_q of memory buffers.
 */
static inline void 
tw_memory_alloc_rc(tw_lp * lp, tw_memory * m, tw_fd fd)
{
	if(--m->nrefs)
		tw_error(TW_LOC, "membuf stills has refs: %d", m->nrefs);

	tw_memoryq_push(tw_pe_getqueue(lp->pe, fd), m);
}

static inline void 
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
}

static inline tw_memory * 
tw_memory_free_rc(tw_lp * lp, tw_fd fd)
{
	tw_memory *m = tw_memoryq_pop(tw_kp_getqueue(lp->kp, fd));

	if(m && m->ts < lp->pe->GVT)
	{
		tw_error(TW_LOC, "You are attempting to rollback a buffer"
				 " with ts (%g) < GVT (%g)!",
				 m->ts, lp->pe->GVT);
	}

	return m;
}

static inline void 
tw_memory_unshift(tw_lp * lp, tw_memory * m, tw_fd fd)
{
	if(m->nrefs)
		return;

	tw_memoryq_push(tw_pe_getqueue(lp->pe, fd),  m);
}

static inline size_t 
tw_memory_getsize(tw_pe * pe, int fd)
{
	return tw_pe_getqueue(pe, fd)->d_size + g_tw_memory_sz;
}

static inline tw_fd 
tw_memory_init(size_t n_mem, size_t d_sz, tw_stime mult)
{
#ifndef ROSS_MEMORY
	tw_error(TW_LOC, "ROSS memory library disabled!");
#endif

	return tw_pe_memory_init(tw_getpe(0), n_mem, d_sz, mult);
}

#endif
