#include <tcp.h>

/*
 * tcp_timer_reset: reset the RTO timer properly
 */
tw_event	*
tcp_timer_reset(tw_event * timer, tw_bf * bf, tcp_message * old, 
		tw_stime ts, int sn, tw_lp * lp)
{
	tcp_message	*m;

	// this can happen because sometimes we rollback more events
	// than we need to when nkp < nlp.  So, sometimes our events
	// our rolled back that do NOT reset the RTO, and therefore
	// it might seem that we need to RC reset the timer, when in fact
	// we do not.
	if(timer && timer->recv_ts == ts)
		tw_error(TW_LOC, "%ld: bad timer reset: cur ts %lf!", lp->gid, ts);

	// if this IS the timer firing, then init new timer
	if((bf->c14 = (!timer || timer == lp->pe->cur_event)))
	{
		old->RC.timer = timer;
		timer = rn_timer_init(lp, ts);

#if TCP_DEBUG
		printf("\t%lld: init RTO to %lf (%ld)\n", lp->gid, ts, (long int) timer);
#endif
		
	} else
	{
#if TCP_DEBUG
		printf("\t%lld: reset RTO to %lf (was %lf)\n", 
			lp->gid, ts, timer->recv_ts);
#endif

		old->RC.timer_ts = timer->recv_ts;
		rn_timer_reset(lp, &timer, ts);
	}

	if((bf->c13 = (timer != NULL)))
	{
		if(!timer->memory)
			timer->memory = tw_memory_alloc(lp, g_tcp_fd);

		m = tw_memory_data(timer->memory);

		old->RC.timer_seq = m->seq_num;
		m->src = old->src;
		m->seq_num = sn;
	}

	return timer;
}

void
tcp_timer_cancel(tw_event * timer, tw_bf * bf, tcp_message * old, tw_lp * lp)
{
#if VERIFY_TCP_TIMER
	printf("%lld: TCP cancel at %lf: old %lf\n", 
		lp->gid, tw_now(lp), timer->recv_ts);
#endif

	old->RC.timer_ts = timer->recv_ts;

	//if(timer->recv_ts <= tw_now(lp) || timer->state.uint == 0)
	if(timer == lp->pe->cur_event)
		return;

	if(!timer->memory)
		tw_error(TW_LOC, "No membuf on timer!");

	tw_memory_free(lp, timer->memory, g_tcp_fd);
	timer->memory = NULL;

	rn_timer_cancel(lp, &timer);

#if VERIFY_TCP_TIMER
	printf("\t%lld: CANCELLED RTO for %lf\n\n\n", 
		lp->gid, old->RC.timer_ts);
#endif
}
