#include <ross.h>
#include <assert.h>

static inline void link_causality (tw_event *nev, tw_event *cev) {
    nev->cause_next = cev->caused_by_me;
    cev->caused_by_me = nev;
}

void tw_event_send(tw_event * event) {
    tw_lp     *src_lp = event->src_lp;
    tw_pe     *send_pe = src_lp->pe;
    tw_pe     *dest_pe = NULL;
    tw_clock pq_start, net_start;

    tw_peid        dest_peid = -1;
    tw_stime   recv_ts = event->recv_ts;

    if (event == send_pe->abort_event) {
        if (TW_STIME_DBL(recv_ts) < g_tw_ts_end) {
            send_pe->cev_abort = 1;
        }
        return;
    }

#ifdef USE_RIO
    // rio saves events scheduled past end time
     if (recv_ts >= g_tw_ts_end) {
        link_causality(event, send_pe->cur_event);
        return;
    }
#endif

     // moved from network-mpi.c in order to give all events a seq_num
     event->event_id = (tw_eventid) ++send_pe->seq_num;

    // call LP remote mapping function to get dest_pe
    dest_peid = (*src_lp->type->map) ((tw_lpid) event->dest_lp);
    event->send_lp = src_lp->gid;

    //Trap lookahead violations
    if (g_tw_synchronization_protocol == CONSERVATIVE) {
        if (TW_STIME_DBL(recv_ts) - TW_STIME_DBL(tw_now(src_lp)) < g_tw_lookahead) {
            tw_error(TW_LOC, "Lookahead violation: decrease g_tw_lookahead %f\n"
                    "Event causing violation: src LP: %lu, src PE: %lu\n"
                    "dest LP %lu, dest PE %lu, recv_ts %f\n",
                    g_tw_lookahead, src_lp->gid, send_pe->id, event->dest_lpid,
                    dest_peid, recv_ts);
        }
    }

    if (event->out_msgs) {
        tw_error(TW_LOC, "It is an error to send an event with pre-loaded output message.");
    }

    link_causality(event, send_pe->cur_event);

    if (dest_peid == g_tw_mynode) {
        event->dest_lp = tw_getlocal_lp((tw_lpid) event->dest_lp);
        dest_pe = event->dest_lp->pe;

#ifdef USE_RAND_TIEBREAKER
        if (send_pe == dest_pe && tw_event_sig_compare(event->dest_lp->kp->last_sig, event->sig) <= 0) {
#else
        if (send_pe == dest_pe && TW_STIME_CMP(event->dest_lp->kp->last_time, recv_ts) <= 0) {
#endif
            /* Fast case, we are sending to our own PE and there is
            * no rollback caused by this send.  We cannot have any
            * transient messages on local sends so we can return.
            */
            pq_start = tw_clock_read();
            tw_pq_enqueue(send_pe->pq, event);
            send_pe->stats.s_pq += tw_clock_read() - pq_start;
            return;
        } else {
            /* Slower, but still local send, so put into top of
            * dest_pe->event_q.
            */
            event->state.owner = TW_pe_event_q;

            tw_eventq_push(&dest_pe->event_q, event);

            if(send_pe != dest_pe) {
                send_pe->stats.s_nsend_loc_remote++;
            }
        }
    } else {
        /* Slowest approach of all; this is not a local event.
        * We need to send it over the network to the other PE
        * for processing.
        */
        send_pe->stats.s_nsend_net_remote++;
        //event->src_lp->lp_stats->s_nsend_net_remote++;
        event->state.owner = TW_net_asend;
        net_start = tw_clock_read();
        tw_net_send(event);
        send_pe->stats.s_net_other += tw_clock_read() - net_start;
    }

    if(tw_gvt_inprogress(send_pe)) {
#ifdef USE_RAND_TIEBREAKER
        send_pe->trans_msg_sig = (tw_event_sig_compare(send_pe->trans_msg_sig, event->sig) < 0) ? send_pe->trans_msg_sig : event->sig;
#else
        send_pe->trans_msg_ts = (TW_STIME_CMP(send_pe->trans_msg_ts, recv_ts) < 0) ? send_pe->trans_msg_ts : recv_ts;
#endif
    }
}

static inline void local_cancel(tw_pe *d, tw_event *event) {
    event->state.cancel_q = 1;

    event->cancel_next = d->cancel_q;
    d->cancel_q = event;
}

static inline void event_cancel(tw_event * event) {
    tw_pe *send_pe = event->src_lp->pe;
    tw_peid dest_peid;
    tw_clock net_start;

    if( event->state.owner == TW_net_asend ||
	event->state.owner == TW_net_outq  || // need to consider this case - Chris 06/13/2018
	event->state.owner == TW_pe_sevent_q) {
        /* Slowest approach of all; this has to be sent over the
        * network to let the dest_pe know it shouldn't have seen
        * it in the first place.
        */
        net_start = tw_clock_read();
        tw_net_cancel(event);
        send_pe->stats.s_nsend_net_remote--;
        send_pe->stats.s_net_other += tw_clock_read() - net_start;
        //event->src_lp->lp_stats->s_nsend_net_remote--;

        if(tw_gvt_inprogress(send_pe)) {
#ifdef USE_RAND_TIEBREAKER
            send_pe->trans_msg_sig = (tw_event_sig_compare(send_pe->trans_msg_sig, event->sig) < 0) ? send_pe->trans_msg_sig : event->sig;
#else
            send_pe->trans_msg_ts = (TW_STIME_CMP(send_pe->trans_msg_ts, event->recv_ts) < 0) ? send_pe->trans_msg_ts : event->recv_ts;
#endif
        }

        return;
    }

#ifdef USE_RIO
    if (event->state.owner == IO_buffer) {
        io_event_cancel(event);
        return;
    }
#endif

    dest_peid = event->dest_lp->pe->id;

    tw_clock pq_start;
    if (send_pe->id == dest_peid) {
        switch (event->state.owner) {
            case TW_pe_pq:
                /* Currently in our pq and not processed; delete it and
                * free the event buffer immediately.  No need to wait.
                */
                pq_start = tw_clock_read();
                tw_pq_delete_any(send_pe->pq, event);
                send_pe->stats.s_pq += tw_clock_read() - pq_start;
                tw_event_free(send_pe, event);
                break;

            case TW_pe_event_q:
            case TW_kp_pevent_q:
                local_cancel(send_pe, event);

                if(tw_gvt_inprogress(send_pe)) {
#ifdef USE_RAND_TIEBREAKER
                    send_pe->trans_msg_sig = (tw_event_sig_compare(send_pe->trans_msg_sig, event->sig) < 0) ? send_pe->trans_msg_sig : event->sig;
#else
                    send_pe->trans_msg_ts = (TW_STIME_CMP(send_pe->trans_msg_ts, event->recv_ts) < 0) ? send_pe->trans_msg_ts : event->recv_ts;
#endif
                }
                break;

            default:
                tw_error(TW_LOC, "unknown fast local cancel owner %d", event->state.owner);
        }
    } else if ((unsigned long)send_pe->id == dest_peid) {
        /* Slower, but still a local cancel, so put into
        * top of dest_pe->cancel_q for final deletion.
        */
        local_cancel(event->dest_lp->pe, event);
        send_pe->stats.s_nsend_loc_remote--;

        if(tw_gvt_inprogress(send_pe)) {
#ifdef USE_RAND_TIEBREAKER
            send_pe->trans_msg_sig = (tw_event_sig_compare(send_pe->trans_msg_sig, event->sig) < 0) ? send_pe->trans_msg_sig : event->sig;
#else
            send_pe->trans_msg_ts = (TW_STIME_CMP(send_pe->trans_msg_ts, event->recv_ts) < 0) ? send_pe->trans_msg_ts : event->recv_ts;
#endif
        }
    } else {
        tw_error(TW_LOC, "Should be remote cancel!");
    }
}

void tw_event_rollback(tw_event * event) {
    tw_event  *e = event->caused_by_me;
    tw_lp     *dest_lp = event->dest_lp;

    tw_free_output_messages(event, 0);

    dest_lp->pe->cur_event = event;
#ifdef USE_RAND_TIEBREAKER
    dest_lp->kp->last_sig = event->sig;
#else
    dest_lp->kp->last_time = event->recv_ts;
#endif

    if( dest_lp->suspend_flag &&
	dest_lp->suspend_event == event &&
	// Must test time stamp since events are reused once GVT sweeps by
#ifdef USE_RAND_TIEBREAKER
    tw_event_sig_compare(dest_lp->suspend_sig, event->sig) == 0)
#else
	TW_STIME_CMP(dest_lp->suspend_time, event->recv_ts) == 0)
#endif
      {
	// unsuspend the LP
	dest_lp->suspend_flag = 0;
	dest_lp->suspend_event = NULL;
#ifdef USE_RAND_TIEBREAKER
    dest_lp->suspend_sig = (tw_event_sig){0,0};
#else
	dest_lp->suspend_time = TW_STIME_CRT(0.0);
#endif
	dest_lp->suspend_error_number = 0;

	if( dest_lp->suspend_do_orig_event_rc == 0 )
	  {
	    goto jump_over_rc_event_handler;
	  }
	else
	  { // reset
	    dest_lp->suspend_do_orig_event_rc = 0;
	    // note, should fall thru and process reverse events
	  }
      }
    else if( dest_lp->suspend_flag )
      { // don't rc this event since it was never forward processed
	goto jump_over_rc_event_handler;
      }

    (*dest_lp->type->revent)(dest_lp->cur_state, &event->cv, tw_event_data(event), dest_lp);

    // reset critical path
    dest_lp->critical_path = event->critical_path;

jump_over_rc_event_handler:
    if (event->delta_buddy) {
        tw_clock start = tw_clock_read();
        buddy_free(event->delta_buddy);
        g_tw_pe->stats.s_buddy += (tw_clock_read() - start);
        event->delta_buddy = 0;
    }

    while (e) {
        tw_event *n = e->cause_next;
        e->cause_next = NULL;
#ifdef USE_RAND_TIEBREAKER
        tw_rand_reverse_unif(e->src_lp->core_rng); //undo the tiebreaker rng advanced by this LP for the subsequent event
#endif
        event_cancel(e);
        e = n;
    }

    event->caused_by_me = NULL;

    dest_lp->kp->s_e_rbs++;
    // instrumentation
    dest_lp->kp->kp_stats->s_e_rbs++;
    dest_lp->lp_stats->s_e_rbs++;
}
