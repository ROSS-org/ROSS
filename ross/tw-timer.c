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
#include <tw-timer.h>

tw_timer
tw_timer_event_new(tw_lp * dest, tw_stime recv_ts, tw_lp * src)
{
	tw_pe          *send_pe;
	tw_event       *e;

	send_pe = src->pe;

	if (recv_ts >= g_tw_ts_end)
		e = send_pe->abort_event;
	else
		e = tw_event_grab(send_pe);

	if(e)
		e->next = NULL;
	else
		e = send_pe->abort_event;

	//e->memory = NULL;
	e->state.owner = 0;
	e->prev = NULL;
	e->cancel_next = NULL;
	e->caused_by_me = NULL;
	e->recv_ts = recv_ts;
	e->dest_lp = dest;
	e->src_lp = src;

	return e;
}

void
tw_timer_event_send(tw_timer event)
{
	tw_pe          *dest_pe;
	tw_pe          *send_pe;
	tw_lp          *src_lp;
	tw_stime        last_sent_ts;

	src_lp = event->src_lp;
	send_pe = event->src_lp->pe;
	dest_pe = event->dest_lp->pe;
	last_sent_ts = event->recv_ts;

	if(event == send_pe->abort_event)
		return;

	/*
	 * do not thread timer events into cause list
	 */

	if (send_pe == dest_pe && event->dest_lp->kp->last_time <= last_sent_ts)
	{
		tw_pq_enqueue(dest_pe->pq, event);
	} else
	{
		tw_error(TW_LOC, "Attempt to schedule timer with ts < KP last_time\n");
	}
}

tw_timer
tw_timer_init(tw_lp * lp, tw_stime ts)
{
	tw_event       *event = NULL;

	if (ts < g_tw_ts_end)
	{
		if ((event = tw_timer_event_new(lp, ts, lp)) != NULL)
			tw_timer_event_send(event);
	}

	return event;
}

void
tw_timer_cancel(tw_lp * lp, tw_timer * e)
{
	tw_event	*event = *e;

#if 0
	if(event->memory && event->recv_ts < g_tw_ts_end)
		tw_error(TW_LOC, "Memory buffer remains on timer!");
#endif

	if(e == NULL)
		tw_error(TW_LOC, "Fatal: Event address is NULL!\n");

	if(event && event != lp->pe->abort_event)
	{
		if(event->state.owner == 0)
			return;
	
		if(event->state.owner == TW_pe_pq)
			tw_pq_delete_any(event->dest_lp->pe->pq, event);

		tw_event_free(lp->pe, event);
		*e = NULL;
	}
}

void
tw_timer_reset(tw_lp * lp, tw_timer * e, tw_stime ts)
{
	tw_event	*event = *e;

	if (ts < g_tw_ts_end)
	{
		if(event)
		{
			if(event->state.owner == TW_pe_pq)
				tw_pq_delete_any(event->dest_lp->pe->pq, event);

			event->recv_ts = ts;
			tw_timer_event_send(event);
		} else
		{
			*e = tw_timer_init(lp, ts);
		}
	} else if(event != NULL)
	{
		// do not free if current event, may be rolled back!
		if(event == lp->pe->cur_event)
			return;

		// printf("Kill old timer and don't create new timer \n");
		event->recv_ts = ts;

		tw_timer_cancel(lp, e);
		*e = NULL;
	}
}
