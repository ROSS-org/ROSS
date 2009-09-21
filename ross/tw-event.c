/*
 * ROSS: Rensselaer's Optimistic Simulation System.
 * Copyright (c) 1999-2003 Rensselaer Polytechnic Instutitute.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *
 *      This product includes software developed by David Bauer,
 *      Dr. Christopher D.  Carothers, and Shawn Pearce of the
 *      Department of Computer Science at Rensselaer Polytechnic
 *      Institute.
 *
 * 4. Neither the name of the University nor of the developers may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * 5. The use or inclusion of this software or its documentation in
 *    any commercial product or distribution of this software to any
 *    other party without specific, written prior permission is
 *    prohibited.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS
 * IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *
 */

#include <ross.h>

static inline void
link_causality (tw_event *nev, tw_event *cev)
{
	nev->cause_next = cev->caused_by_me;
	cev->caused_by_me = nev;
}

void
tw_event_send(tw_event * event)
{
	tw_lp		*src_lp = event->src_lp;
	tw_pe		*send_pe = src_lp->pe;
	tw_pe		*dest_pe = NULL;

	tw_peid		 dest_peid = -1;
	tw_stime	 recv_ts = event->recv_ts;

	if (event == send_pe->abort_event) {
		if (recv_ts < g_tw_ts_end)
			send_pe->cev_abort = 1;
		return;
	}

	link_causality(event, send_pe->cur_event);

	// call LP remote mapping function to get dest_pe
	dest_peid = (*src_lp->type.map) ((tw_lpid) event->dest_lp);

	if(tw_node_eq(tw_net_onnode(dest_peid), &g_tw_mynode))
	{
		event->dest_lp = tw_getlocal_lp((tw_lpid) event->dest_lp);
		dest_pe = event->dest_lp->pe;

		if (send_pe == dest_pe &&
			event->dest_lp->kp->last_time <= recv_ts)
		{
			/* Fast case, we are sending to our own PE and there is
			 * no rollback caused by this send.  We cannot have any
			 * transient messages on local sends so we can return.
			 */
			tw_pq_enqueue(send_pe->pq, event);
			return;
		} else
		{
			/* Slower, but still local send, so put into top of
			 * dest_pe->event_q. 
			 */
			event->state.owner = TW_pe_event_q;
	
			tw_mutex_lock(&dest_pe->event_q_lck);
			tw_eventq_push(&dest_pe->event_q, event);
			tw_mutex_unlock(&dest_pe->event_q_lck);

			if(send_pe != dest_pe)
				send_pe->stats.s_nsend_loc_remote++;
		}
	} else
	{
		/* Slowest approach of all; this is not a local event.
		 * We need to send it over the network to the other PE
		 * for processing.
		 */
		send_pe->stats.s_nsend_net_remote++;
		event->state.owner = TW_net_asend;
		tw_net_send(event);
	}

	if(tw_gvt_inprogress(send_pe))
		send_pe->trans_msg_ts = min(send_pe->trans_msg_ts, recv_ts);
}

static inline void
local_cancel(tw_pe *d, tw_event *event)
{
	event->state.cancel_q = 1;

	tw_mutex_lock(&d->cancel_q_lck);
	event->cancel_next = d->cancel_q;
	d->cancel_q = event;
	tw_mutex_unlock(&d->cancel_q_lck);
}

static inline void
event_cancel(tw_event * event)
{
	tw_pe *send_pe = event->src_lp->pe;
	tw_peid dest_peid;

	if(event->state.owner == TW_net_asend ||
	   event->state.owner == TW_pe_sevent_q)
	{
		/* Slowest approach of all; this has to be sent over the
		 * network to let the dest_pe know it shouldn't have seen
		 * it in the first place.
		 */
		tw_net_cancel(event);
		send_pe->stats.s_nsend_net_remote--;

		if(tw_gvt_inprogress(send_pe))
			send_pe->trans_msg_ts = min(send_pe->trans_msg_ts, event->recv_ts);

		return;
	}

	dest_peid = event->dest_lp->pe->id;

	if (send_pe->id == dest_peid)
	{
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
		
			if(tw_gvt_inprogress(send_pe))
				send_pe->trans_msg_ts = 
					min(send_pe->trans_msg_ts, event->recv_ts);
			break;

		default:
			tw_error(
				TW_LOC,
				"unknown fast local cancel owner %d",
				event->state.owner);
		}
	} else if (tw_node_eq(&send_pe->node, tw_net_onnode(dest_peid))) {
		/* Slower, but still a local cancel, so put into
		 * top of dest_pe->cancel_q for final deletion.
		 */
		local_cancel(event->dest_lp->pe, event);
		send_pe->stats.s_nsend_loc_remote--;

		if(tw_gvt_inprogress(send_pe))
			send_pe->trans_msg_ts = min(send_pe->trans_msg_ts, event->recv_ts);
	} else {
		tw_error(TW_LOC, "Should be remote cancel!");
	}
}

void
tw_event_rollback(tw_event * event)
{
	tw_event	*e = event->caused_by_me;
	tw_lp		*dest_lp = event->dest_lp;

	tw_state_rollback(dest_lp, event);

	if (e)
	{
		do
		{
			tw_event *n = e->cause_next;
			e->cause_next = NULL;

			event_cancel(e);
			e = n;
		} while (e);
		event->caused_by_me = NULL;
	}

	dest_lp->kp->s_e_rbs++;
}
