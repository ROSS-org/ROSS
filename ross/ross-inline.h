#ifndef INC_ross_inline_h
#define	INC_ross_inline_h

INLINE(tw_event *)
tw_event_grab(tw_pe *pe)
{
	tw_event	*e = tw_eventq_pop(&pe->free_q);

	if (e)
	{
#ifndef ROSS_NETWORK_none
		// have to reclaim non-cancelled remote events from hash table
		if(e->event_id && e->state.remote)
			tw_hash_remove(pe->hash_t, e, e->send_pe);
#endif

		e->cancel_next = NULL;
		e->caused_by_me = NULL;
		e->cause_next = NULL;
		e->prev = e->next = NULL;

		memset(&e->state, 0, sizeof(e->state));
		memset(&e->event_id, 0, sizeof(e->event_id));

#ifdef ROSS_MEMORY
		if(e->memory)
		{
			if(!e->memory->nrefs)
				tw_printf(TW_LOC, "membuf remaining on event: %d", 
					  e->memory->nrefs);
			e->memory = NULL;
		}
#endif
	} else
		e = pe->abort_event;

	return e;
}

INLINE(tw_event *)
tw_event_new(tw_lpid dest_gid, tw_stime offset_ts, tw_lp * sender)
{
	tw_pe		*send_pe;
	tw_event	*e;
	tw_stime	 recv_ts;

	send_pe = sender->pe;
	recv_ts = tw_now(sender) + offset_ts;

	/* If this event will be past the end time, or there
	 * are no more free events available, use abort event.
	 */
	if (recv_ts >= g_tw_ts_end)
		e = send_pe->abort_event;
	else 
		e = tw_event_grab(send_pe);

	e->dest_lp = (tw_lp *) dest_gid;
	e->src_lp = sender;
	e->recv_ts = recv_ts;

	return e;
}

INLINE(void)
tw_event_free(tw_pe *pe, tw_event *e)
{
	/*
	 * During the course of a rollback, events are supposed to put
	 * the membufs back on the event.  The event is then cancelled
	 * and freed -- which is how a membuf could end up on a freed
	 * event.
	 */
#ifdef ROSS_MEMORY
	tw_memory	*next;
	tw_memory	*m;

	m = next = e->memory;

	while(m)
	{
		next = m->next;

		if(0 == --m->nrefs)
		{
			if(e->state.owner >= TW_net_outq && e->state.owner <= TW_pe_sevent_q)
				tw_memory_unshift(e->src_lp, m, m->fd);
			else
				tw_memory_unshift(e->dest_lp, m, m->fd);
		}

		m = next;
	}

	e->memory = NULL;
#endif

	e->state.owner = TW_pe_free_q;

	tw_eventq_unshift(&pe->free_q, e);
}

#ifdef ROSS_MEMORY
INLINE(tw_memory *)
tw_event_memory_get(tw_lp * lp)
{
	tw_memory      *m = lp->pe->cur_event->memory;

	if(m && m->next)
		lp->pe->cur_event->memory = lp->pe->cur_event->memory->next;
	else
		lp->pe->cur_event->memory = NULL;

	return m;
}

INLINE(void)
tw_event_memory_get_rc(tw_lp * lp, tw_memory * m, tw_fd fd)
{
	m->next = lp->pe->cur_event->memory;
	lp->pe->cur_event->memory = m;
}

INLINE(void)
tw_event_memory_setfifo(tw_event * e, tw_memory * m, tw_fd fd)
{
	tw_memory	*b;

	if(e == e->src_lp->pe->abort_event)
	{
		tw_memory_alloc_rc(e->src_lp, m, fd);
		return;
	}

	m->fd = fd;

	if(NULL == e->memory)
	{
		m->next = e->memory;
		e->memory = m;
	} else
	{
		b = e->memory;
		while(b->next)
			b = b->next;

		b->next = m;
		m->next = NULL;
	}
}

INLINE(void)
tw_event_memory_set(tw_event * e, tw_memory * m, tw_fd fd)
{
	if(e == e->src_lp->pe->abort_event)
	{
		tw_memory_alloc_rc(e->src_lp, m, fd);
		return;
	}

	m->next = e->memory;
	e->memory = m;
}

INLINE(void)
tw_event_memory_forward(tw_event * e)
{
	tw_memory	*m;

	if(e == e->src_lp->pe->abort_event)
		return;

	e->memory = e->src_lp->pe->cur_event->memory;

	m = e->memory;
	while(m)
	{
		m->nrefs++;
		m = m->next;
	}
}

INLINE(void *)
tw_memory_data(tw_memory * memory)
{
	return memory + 1;
}
#endif

INLINE(void *)
tw_event_data(tw_event * event)
{
	return event + 1;
}

#endif
