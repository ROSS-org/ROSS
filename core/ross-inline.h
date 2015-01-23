#ifndef INC_ross_inline_h
#define	INC_ross_inline_h

static inline tw_event * 
tw_event_grab(tw_pe *pe)
{
  tw_event	*e = tw_eventq_pop(&pe->free_q);

  if (e)
    {
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

static inline void
tw_free_output_messages(tw_event *e, int print_message)
{
    while (e->out_msgs) {
        tw_out *temp = e->out_msgs;
        if (print_message)
            printf("%s", temp->message);
        e->out_msgs = temp->next;
        // Put it back
        tw_kp_put_back_output_buffer(temp);
    }
}

/**
 * @bug There's a bug in this function.  We put dest_gid, which is
 * a 64-bit value, into dest_lp which may be a 32-bit pointer.  
 */
static inline tw_event * 
tw_event_new(tw_lpid dest_gid, tw_stime offset_ts, tw_lp * sender)
{
  tw_pe		*send_pe;
  tw_event	*e;
  tw_stime	 recv_ts;

  send_pe = sender->pe;
  recv_ts = tw_now(sender) + offset_ts;

  if(g_tw_synchronization_protocol == CONSERVATIVE)
  {
    /* keep track of the smallest timestamp offset we have seen */
    if(offset_ts < g_tw_min_detected_offset)
      g_tw_min_detected_offset = offset_ts;
  }

  /* If this event will be past the end time, or there
   * are no more free events available, use abort event.
   */
  if (recv_ts >= g_tw_ts_end) {
    e = send_pe->abort_event;
    send_pe->stats.s_events_past_end++;
  } else {
    e = tw_event_grab(send_pe);
  }

  e->dest_lp = (tw_lp *) dest_gid;
  e->src_lp = sender;
  e->recv_ts = recv_ts;

  tw_free_output_messages(e, 0);

  return e;
}

static inline void 
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

  tw_free_output_messages(e, 0);

  if (e->delta_buddy) {
    tw_clock start = tw_clock_read();
    buddy_free(e->delta_buddy);
    g_tw_pe[0]->stats.s_buddy += (tw_clock_read() - start);
    e->delta_buddy = 0;
  }

  e->state.owner = TW_pe_free_q;

  tw_eventq_unshift(&pe->free_q, e);
}

#ifdef ROSS_MEMORY
static inline tw_memory * 
tw_event_memory_get(tw_lp * lp)
{
  tw_memory      *m = lp->pe->cur_event->memory;

  if(m && m->next)
    lp->pe->cur_event->memory = lp->pe->cur_event->memory->next;
  else
    lp->pe->cur_event->memory = NULL;

  return m;
}

static inline void 
tw_event_memory_get_rc(tw_lp * lp, tw_memory * m, tw_fd fd)
{
  m->next = lp->pe->cur_event->memory;
  lp->pe->cur_event->memory = m;
}

static inline void 
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

static inline void 
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

static inline void 
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

static inline void * 
tw_memory_data(tw_memory * memory)
{
  return memory + 1;
}
#endif

static inline void * 
tw_event_data(tw_event * event)
{
  return event + 1;
}

#endif
