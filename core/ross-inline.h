#ifndef INC_ross_inline_h
#define INC_ross_inline_h

static inline tw_event *
tw_event_grab(tw_pe *pe)
{
  tw_event *e = tw_eventq_pop(&pe->free_q);

  if (e)
    {
      e->cancel_next = NULL;
      e->caused_by_me = NULL;
      e->cause_next = NULL;
      e->prev = e->next = NULL;

      memset(&e->state, 0, sizeof(e->state));
      memset(&e->event_id, 0, sizeof(e->event_id));
    }
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
  tw_pe *send_pe;
  tw_event *e;
  tw_stime recv_ts;

  if (TW_STIME_DBL(offset_ts) < 0.0) {
    tw_error(TW_LOC, "Cannot send events into the past! Sending LP: %lu\n", sender->gid);
  }

  send_pe = sender->pe;
  recv_ts = TW_STIME_ADD(tw_now(sender), offset_ts);

  if(g_tw_synchronization_protocol == CONSERVATIVE)
  {
    /* keep track of the smallest timestamp offset we have seen */
  if(TW_STIME_DBL(offset_ts) < g_tw_min_detected_offset)
    g_tw_min_detected_offset = TW_STIME_DBL(offset_ts);
  }

  /* If this event will be past the end time, or there
   * are no more free events available, use abort event.
   */
  if (TW_STIME_DBL(recv_ts) >= g_tw_ts_end) {
#ifdef USE_RIO
    e = io_event_grab(send_pe);
#else
    e = send_pe->abort_event;
#endif
    send_pe->stats.s_events_past_end++;
  } else {
    e = tw_event_grab(send_pe);
    if (!e) {
        if (g_tw_synchronization_protocol == CONSERVATIVE
                || g_tw_synchronization_protocol == SEQUENTIAL) {
        tw_error(TW_LOC,
                "No free event buffers. Try increasing via g_tw_events_per_pe"
                " or --extramem");
        }
        else
            e = send_pe->abort_event;
    }
  }

  e->send_pe = sender->pe->id;
  e->dest_lp = (tw_lp *) dest_gid;
  e->dest_lpid = dest_gid;
  e->src_lp = sender;
  e->recv_ts = recv_ts;
  e->send_ts = tw_now(sender);
  e->critical_path = sender->critical_path + 1;

#ifdef USE_RAND_TIEBREAKER
  e->event_tiebreaker = tw_rand_unif(sender->core_rng); //create a random number used to deterministically break event ties, this is rolled back in tw_event_rollback() during the sender LP cancel loop
  e->sig = (tw_event_sig){e->recv_ts, e->event_tiebreaker};
#endif

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
  tw_free_output_messages(e, 0);

  if (e->delta_buddy) {
    tw_clock start = tw_clock_read();
    buddy_free(e->delta_buddy);
    g_tw_pe->stats.s_buddy += (tw_clock_read() - start);
    e->delta_buddy = 0;
  }

  e->state.owner = TW_pe_free_q;

  tw_eventq_unshift(&pe->free_q, e);
}

static inline void *
tw_event_data(tw_event * event)
{
  return event + 1;
}

#endif
