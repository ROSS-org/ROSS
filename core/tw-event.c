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

    tw_peid        dest_peid = -1;
    tw_stime   recv_ts = event->recv_ts;

    if (event == send_pe->abort_event) {
        if (recv_ts < g_tw_ts_end) {
            send_pe->cev_abort = 1;
        }
        return;
    }

    //Trap lookahead violations in debug mode
    //Note that compiling with the -DNDEBUG flag will turn this off!
    if (g_tw_synchronization_protocol == CONSERVATIVE) {
        if (recv_ts - tw_now(src_lp) < g_tw_lookahead) {
            tw_error(TW_LOC, "Lookahead violation: decrease g_tw_lookahead");
        }
    }

    if (event->out_msgs) {
        tw_error(TW_LOC, "It is an error to send an event with pre-loaded output message.");
    }

    link_causality(event, send_pe->cur_event);

    // call LP remote mapping function to get dest_pe
    dest_peid = (*src_lp->type->map) ((tw_lpid) event->dest_lp);

    if (dest_peid == g_tw_mynode) {
        event->dest_lp = tw_getlocal_lp((tw_lpid) event->dest_lp);
        dest_pe = event->dest_lp->pe;

        if (send_pe == dest_pe && event->dest_lp->kp->last_time <= recv_ts) {
            /* Fast case, we are sending to our own PE and there is
            * no rollback caused by this send.  We cannot have any
            * transient messages on local sends so we can return.
            */
            tw_pq_enqueue(send_pe->pq, event);
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
        event->state.owner = TW_net_asend;
        tw_net_send(event);
    }

    if(tw_gvt_inprogress(send_pe)) {
        send_pe->trans_msg_ts = ROSS_MIN(send_pe->trans_msg_ts, recv_ts);
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

    if(event->state.owner == TW_net_asend || event->state.owner == TW_pe_sevent_q) {
        /* Slowest approach of all; this has to be sent over the
        * network to let the dest_pe know it shouldn't have seen
        * it in the first place.
        */
        tw_net_cancel(event);
        send_pe->stats.s_nsend_net_remote--;

        if(tw_gvt_inprogress(send_pe)) {
            send_pe->trans_msg_ts = ROSS_MIN(send_pe->trans_msg_ts, event->recv_ts);
        }

        return;
    }

    dest_peid = event->dest_lp->pe->id;

    if (send_pe->id == dest_peid) {
        switch (event->state.owner) {
            case TW_pe_pq:
                /* Currently in our pq and not processed; delete it and
                * free the event buffer immediately.  No need to wait.
                */
                tw_pq_delete_any(send_pe->pq, event);
                tw_event_free(send_pe, event);
                break;

            case TW_pe_event_q:
            case TW_kp_pevent_q:
                local_cancel(send_pe, event);

                if(tw_gvt_inprogress(send_pe)) {
                    send_pe->trans_msg_ts = ROSS_MIN(send_pe->trans_msg_ts, event->recv_ts);
                }
                break;

            default:
                tw_error(TW_LOC, "unknown fast local cancel owner %d", event->state.owner);
        }
    } else if (send_pe->node == dest_peid) {
        /* Slower, but still a local cancel, so put into
        * top of dest_pe->cancel_q for final deletion.
        */
        local_cancel(event->dest_lp->pe, event);
        send_pe->stats.s_nsend_loc_remote--;

        if(tw_gvt_inprogress(send_pe)) {
            send_pe->trans_msg_ts = ROSS_MIN(send_pe->trans_msg_ts, event->recv_ts);
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
    dest_lp->kp->last_time = event->recv_ts;
    (*dest_lp->type->revent)(dest_lp->cur_state, &event->cv, tw_event_data(event), dest_lp);


    if (event->delta_buddy) {
        tw_clock start = tw_clock_read();
        buddy_free(event->delta_buddy);
        g_tw_pe[0]->stats.s_buddy += (tw_clock_read() - start);
        event->delta_buddy = 0;
    }

    while (e) {
        tw_event *n = e->cause_next;
        e->cause_next = NULL;

        event_cancel(e);
        e = n;
    }

    event->caused_by_me = NULL;

    dest_lp->kp->s_e_rbs++;
}
