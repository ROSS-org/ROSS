#include <ross.h>
#include <assert.h>
#include "lazy_rollback.h"

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

     // check the lazy_q
     if (send_pe->lazy_q.size != 0) {
         // TODO: is this send or recv TS?????
         if (TW_STIME_CMP(event->send_ts, tw_eventq_peek_head(&send_pe->lazy_q)->send_ts) > 0) {
             // we need to send some anti messages
             lazy_rollback_catchup_to(send_pe, event->send_ts);
         } else {
             if (lazy_q_annihilate(send_pe, event)) {
                 // optimization achieved
                 send_pe->stats.s_n_lazy_opts++;
                 event->state.owner = TW_pe_sevent_q;
                 // can't free the event yet... need to keep in cause list
                 //tw_event_free(send_pe, event);
                 return;
             }
         }
     }

    if (dest_peid == g_tw_mynode) {
        event->dest_lp = tw_getlocal_lp((tw_lpid) event->dest_lp);
        dest_pe = event->dest_lp->pe;

        if (send_pe == dest_pe && TW_STIME_CMP(event->dest_lp->kp->last_time, recv_ts) <= 0) {
            /* Fast case, we are sending to our own PE and there is
            * no rollback caused by this send.  We cannot have any
            * transient messages on local sends so we can return.
            */
            pq_start = tw_clock_read();
            tw_pq_enqueue(send_pe->pq, event);
            send_pe->stats.s_pq += tw_clock_read() - pq_start;
            return;
        } else {
            // ALERT: DEAD CODE! NOW ONLY 1 PE per MPI RANK
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
        send_pe->trans_msg_ts = (TW_STIME_CMP(send_pe->trans_msg_ts, recv_ts) < 0) ? send_pe->trans_msg_ts : recv_ts;
    }
}

void tw_event_rescind(tw_event * event) {
    tw_clock pq_start;
    tw_pe *send_pe = event->src_lp->pe;

    // assert dest LP is the same as the sender
    if (event->dest_lp != event->src_lp) {
        tw_error(TW_LOC, "Cannot rescind an event sent from a diffenet LP");
    }

    // UNDO the "fast case" from tw_event_send
    // pq_delete_any removes the event from the PQ, leaving it ownerless
    pq_start = tw_clock_read();
    tw_pq_delete_any(send_pe->pq, event);
    send_pe->stats.s_pq += tw_clock_read() - pq_start;

    // keep track of the rescinded event, in case of rollback
    // similar to link_causality
    event->rescind_next = send_pe->cur_event->rescinded_by_me;
    send_pe->cur_event->rescinded_by_me = event;

    // final step: walk the rescinded_by_me list during rollback.
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
            send_pe->trans_msg_ts = (TW_STIME_CMP(send_pe->trans_msg_ts, event->recv_ts) < 0) ? send_pe->trans_msg_ts : event->recv_ts;
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
                    send_pe->trans_msg_ts = (TW_STIME_CMP(send_pe->trans_msg_ts, event->recv_ts) < 0) ? send_pe->trans_msg_ts : event->recv_ts;
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
            send_pe->trans_msg_ts = (TW_STIME_CMP(send_pe->trans_msg_ts, event->recv_ts) < 0) ? send_pe->trans_msg_ts : event->recv_ts;
        }
    } else {
        tw_error(TW_LOC, "Should be remote cancel!");
    }
}

void tw_event_rollback(tw_event * event) {
    tw_event  *e = event->caused_by_me;
    tw_event  *r = event->rescinded_by_me;
    tw_lp     *dest_lp = event->dest_lp;

    tw_free_output_messages(event, 0);

    dest_lp->pe->cur_event = event;
    dest_lp->kp->last_time = event->recv_ts;

    if( dest_lp->suspend_flag &&
	dest_lp->suspend_event == event &&
	// Must test time stamp since events are reused once GVT sweeps by
	TW_STIME_CMP(dest_lp->suspend_time, event->recv_ts) == 0)
      {
	// unsuspend the LP
	dest_lp->suspend_flag = 0;
	dest_lp->suspend_event = NULL;
	dest_lp->suspend_time = TW_STIME_CRT(0.0);
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

    // restore events rescinded by this event
    while (r) {
        tw_event *n = r->rescind_next;
        r->rescind_next = NULL;

        // re-do the "fast case" from tw_event_send
        // assumption that dest LP is the sender
        tw_clock pq_start = tw_clock_read();
        tw_pq_enqueue(dest_lp->pe->pq, r);
        dest_lp->pe->stats.s_pq += tw_clock_read() - pq_start;

        r = n;
    }

    // cancel events caused by this event
    while (e) {
        tw_event *n = e->cause_next;
        e->cause_next = NULL;

        event_cancel(e);
        e = n;
    }

    event->caused_by_me = NULL;

    dest_lp->kp->s_e_rbs++;
    // instrumentation
    dest_lp->kp->kp_stats->s_e_rbs++;
    dest_lp->lp_stats->s_e_rbs++;
}
