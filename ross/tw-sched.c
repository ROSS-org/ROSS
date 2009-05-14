#include <ross.h>

/* 
 * Get all events out of my event queue and spin them out into
 * the priority queue so they can be processed in time stamp
 * order.  
 */
static void
tw_sched_event_q(tw_pe * me)
{
	tw_kp	 *dest_kp;
	tw_event *cev;
	tw_event *nev;

	while (me->event_q.size) {
		tw_mutex_lock(&me->event_q_lck);
		cev = tw_eventq_pop_list(&me->event_q);
		tw_mutex_unlock(&me->event_q_lck);

		for (; cev; cev = nev) {
		nev = cev->next;

		if(!cev->state.owner || cev->state.owner == TW_pe_free_q)
			tw_error(TW_LOC, "no owner!");

		if (cev->state.cancel_q)
		{
			cev->state.owner = TW_pe_anti_msg;
			cev->next = cev->prev = NULL;
			continue;
		}

		switch (cev->state.owner) {
		case TW_pe_event_q:
			dest_kp = cev->dest_lp->kp;

			if (dest_kp->last_time > cev->recv_ts) {
				/* cev is a straggler message which has arrived
				 * after we processed events occuring after it.
				 * We need to jump back to before cev's timestamp.
				 */
				tw_kp_rollback_to(dest_kp, cev->recv_ts);
			}

			tw_pq_enqueue(me->pq, cev);
			break;

		default:
			tw_error(
				TW_LOC,
				"Event in event_q, but owner %d not recognized",
				cev->state.owner);
		}
		}
	}
}

/*
 * OPT: need to link events into canq in reverse order so 
 *      that when we rollback the 1st event, we should not
 *	need to do any further rollbacks.
 */
static void
tw_sched_cancel_q(tw_pe * me)
{
	tw_event 	*cev;
	tw_event	*nev;

	while (me->cancel_q) {
		tw_mutex_lock(&me->cancel_q_lck);
		cev = me->cancel_q;
		me->cancel_q = NULL;
		tw_mutex_unlock(&me->cancel_q_lck);

		for (; cev; cev = nev) {
			nev = cev->cancel_next;

			if (!cev->state.cancel_q)
				tw_error(TW_LOC, "No cancel_q bit on event in cancel_q");

			if(!cev->state.owner || cev->state.owner == TW_pe_free_q)
				tw_error(TW_LOC, "Cancelled event, no owner!");

			switch (cev->state.owner) {
			case TW_pe_event_q:
				/* This event hasn't been added to our pq yet and we
				 * have not officially received it yet either.  We'll
				 * do the actual free of this event when we receive it
				 * as we spin out the event_q chain.
				 */
				tw_mutex_lock(&me->event_q_lck);
				tw_eventq_delete_any(&me->event_q, cev);
				tw_mutex_unlock(&me->event_q_lck);

				tw_event_free(me, cev);
				break;

			case TW_pe_anti_msg:
				tw_event_free(me, cev);
				break;

			case TW_pe_pq:
				/* Event was not cancelled directly from the event_q
				 * because the cancel message came after we popped it
				 * out of that queue but before we could process it.
				 */
				tw_pq_delete_any(me->pq, cev);
				tw_event_free(me, cev);
				break;

			case TW_kp_pevent_q:
				/* The event was already processed. 
				 * SECONDARY ROLLBACK
				 */
				tw_kp_rollback_event(cev);
				tw_event_free(me, cev);
				break;

			default:
				tw_error(
					TW_LOC,
					"Event in cancel_q, but owner %d not recognized",
					cev->state.owner);
			}
		}
	}
}

static void
tw_sched_batch(tw_pe * me)
{
	unsigned int msg_i;

	/* Process g_tw_mblock events, or until the PQ is empty
	 * (whichever comes first). 
	 */
	for (msg_i = g_tw_mblock; msg_i; msg_i--) {
		tw_event *cev;
		tw_lp *clp;
		tw_kp *ckp;

		/* OUT OF FREE EVENT BUFFERS.  BAD.
		 * Go do fossil collect immediately.
		 */
		if (me->free_q.size <= g_tw_gvt_threshold) {
			tw_gvt_force_update(me);
			break;
		}

		if (!(cev = tw_pq_dequeue(me->pq)))
			break;

		clp = cev->dest_lp;
		ckp = clp->kp;
		me->cur_event = cev;
		ckp->last_time = cev->recv_ts;

		/* Save state if no reverse computation is available */
		if (!clp->type.revent)
			tw_state_save(clp, cev);

		(*clp->type.event)(
			clp->cur_state,
			&cev->cv,
			tw_event_data(cev),
			clp);
		ckp->s_nevent_processed++;

		/* We ran out of events while processing this event.  We
		 * cannot continue without doing GVT and fossil collect.
		 */
		if (me->cev_abort) {
			me->s_nevent_abort++;
			me->cev_abort = 0;

			tw_event_rollback(cev);
			tw_pq_enqueue(me->pq, cev);

			cev = tw_eventq_peek(&ckp->pevent_q);
			ckp->last_time = cev ? cev->recv_ts : me->GVT;

			tw_gvt_force_update(me);
			break;
		}

		/* Thread current event into processed queue of kp */
		cev->state.owner = TW_kp_pevent_q;
		tw_eventq_unshift(&ckp->pevent_q, cev);
	}
}

static void
tw_sched_init(tw_pe * me)
{
	(*me->type.pre_lp_init)(me);
	tw_init_kps(me);
	tw_init_lps(me);
	(*me->type.post_lp_init)(me);

	tw_barrier_sync(&g_tw_simstart);
	tw_net_barrier(me);

	/*
	 * Recv all of the startup events out of the network before
	 * starting simulation.. at this point, all LPs are done with init.
	 */
	if (tw_nnodes() > 1)
	{
		tw_net_read(me);
		tw_net_barrier(me);
	}

	tw_barrier_sync(&g_tw_simstart);

	if (tw_nnodes() > 1)
		tw_clock_init(me);

	/* This lets the signal handler know that we have started
	 * the scheduler loop, and to print out the stats before
	 * finishing if someone should type CTRL-c
	 */
	if((tw_nnodes() > 1 || g_tw_npe > 1) &&
		tw_node_eq(&g_tw_mynode, &g_tw_masternode) && 
		me->local_master)
		printf("*** START PARALLEL SIMULATION ***\n\n");
	else if(tw_nnodes() == 1 && g_tw_npe == 1)
		printf("*** START SEQUENTIAL SIMULATION ***\n\n");

	if (me->local_master)
		g_tw_sim_started = 1;
}

void
tw_scheduler(tw_pe * me)
{
	tw_sched_init(me);
	tw_wall_now(&me->start_time);

	for (;;)
	{
		if (tw_nnodes() > 1)
			tw_net_read(me);

		tw_gvt_step1(me);

		tw_sched_event_q(me);
		tw_sched_cancel_q(me);

		tw_gvt_step2(me);
		if (me->GVT > g_tw_ts_end)
			break;

		tw_sched_batch(me);
	}

	tw_wall_now(&me->end_time);

	if((tw_nnodes() > 1 || g_tw_npe > 1) &&
		tw_node_eq(&g_tw_mynode, &g_tw_masternode) && 
		me->local_master)
		printf("*** END SIMULATION ***\n\n");

	tw_barrier_sync(&g_tw_simend);
	tw_net_barrier(me);

	// call the model PE finalize function
	(*me->type.final)(me);

	tw_stats(me);
}

void
tw_scheduler_seq(tw_pe * me)
{
	tw_event *cev;

	tw_sched_init(me);
	tw_wall_now(&me->start_time);
	while ((cev = tw_pq_dequeue(me->pq))) {
		tw_lp *clp = (tw_lp*)cev->dest_lp;
		tw_kp *ckp = clp->kp;

		me->cur_event = cev;
		ckp->last_time = cev->recv_ts;

		(*clp->type.event)(
			clp->cur_state,
			&cev->cv,
			tw_event_data(cev),
			clp);

		if (me->cev_abort)
			tw_error(TW_LOC, "insufficient event memory");

		ckp->s_nevent_processed++;
		tw_event_free(me, cev);
	}
	tw_wall_now(&me->end_time);

	printf("*** END SIMULATION ***\n\n");

	tw_stats(me);

	(*me->type.final)(me);
}
