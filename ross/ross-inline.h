#ifndef INC_ross_inline_h
#define	INC_ross_inline_h

INLINE(tw_memoryq *)
tw_kp_getqueue(tw_kp * kp, tw_fd fd)
{
	if(fd == -1)
		tw_error(TW_LOC, "Queue not found!");

	return &kp->queues[fd];
}

INLINE(int)
tw_ismaster(void)
{
	return tw_node_eq(&g_tw_mynode, &g_tw_masternode);
}

INLINE(void *)
tw_getstate(tw_lp * lp)
{
	return lp->cur_state;
}

INLINE(tw_kp *)
tw_getkp(tw_kpid id)
{
	if (id >= g_tw_nkp)
		tw_error(TW_LOC, "ID %d exceeded MAX KPs", id);
	if (id != g_tw_kp[id].id)
		tw_error(TW_LOC, "Inconsistent KP Mapping");
	return &g_tw_kp[id];
}

INLINE(tw_stime)
tw_now(tw_lp * lp)
{
	return (lp->kp->last_time);
}

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
	}

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
	else {
		e = tw_event_grab(send_pe);
		if (!e)
			e = send_pe->abort_event;
	}

#if 0
#ifndef ROSS_NETWORK_none
	if(e->event_id && e->state.remote)
	{
		tw_hash_remove(send_pe->hash_t, e, e->src_lp->pe->id);
		e->state.remote = 0;
		e->event_id = 0;
	}
#endif
#endif

	e->dest_lp = (tw_lp *) dest_gid;
	e->src_lp = sender;
	e->recv_ts = recv_ts;

	return e;
}

INLINE(void)
tw_event_free(tw_pe *pe, tw_event *e)
{
	e->state.owner = TW_pe_free_q;
	tw_eventq_unshift(&pe->free_q, e);
}

/*
 * This function returns all of the memory buffers send on the event.
 */
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
	m->prev = (tw_memory *)fd;
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

	m->prev = (tw_memory *)fd;

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
	m->prev = (tw_memory *)fd;
	e->memory = m;
}

INLINE(void *)
tw_memory_data(tw_memory * m)
{
	return (void *)m->data;
}

INLINE(void *)
tw_event_data(tw_event * event)
{
	return event + 1;
}

#endif
