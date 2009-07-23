#include <tcp.h>

/*
 * tcp_timer_reset: reset the RTO timer properly
 */
tw_event	*
tcp_timer_reset(tw_event * timer, tw_bf * bf, tcp_message * old, 
		tw_stime ts, int sn, tw_lp * lp)
{
	tcp_message	*m;

	// if this IS the timer firing, then do create new timer!
	if((bf->c14 = (!timer || timer == lp->cur_event)))
	{
		old->RC.timer = timer;
		timer = NULL;
		timer = rn_timer_init(lp, ts);
	} else
	{
		old->RC.timer_ts = timer->recv_ts;
		rn_timer_reset(lp, &timer, ts);
	}

	if((bf->c13 = (timer != NULL)))
	{
		if(!timer->memory)
			timer->memory = tw_memory_alloc(lp, g_tcp_fd);

#if TCP_DEBUG
		printf("\t%ld: reset RTO to %lf (%ld)\n", lp->id, ts, (long int)timer);
#endif

		m = tw_memory_data(timer->memory);

		// do I need this?
		old->RC.timer_seq = m->seq_num;

		m->src = old->src;
		m->seq_num = sn;
	}

	if(timer && !timer->memory)
		tw_error(TW_LOC, "no membuf here!");

	return timer;
}

tw_event	*
tcp_rc_timer_reset(tw_event * timer, tw_bf * bf, tcp_message * old, tw_lp * lp)
{
	if(bf->c14)
	{
		// If I init'd a timer, cancel it.
		if(timer)
		{
			tw_memory_alloc_rc(lp, timer->memory, g_tcp_fd);
			timer->memory = NULL;

#if TCP_DEBUG
			printf("\t%ld: cancel timer at %lf (%ld)\n", 
				lp->id, timer->recv_ts, (long int) timer);
#endif

			rn_timer_cancel(lp, &timer);
		}

		// restore pointer to previous timer
		timer = old->RC.timer;
		old->RC.timer = NULL;
	} else
	{
		rn_timer_reset(lp, &timer, old->RC.timer_ts);
	}

	if(bf->c13 == 0)
		return timer;

	if(timer)
	{
		tcp_message	*m;

#if TCP_DEBUG
		printf("\t%ld: new RTO at %lf (%ld)\n", 
			lp->id, tw_now(lp), (long int) timer);
#endif

		if(NULL == timer->memory)
			timer->memory = tw_memory_alloc(lp, g_tcp_fd);

		m = tw_memory_data(timer->memory);
		m->seq_num = old->RC.timer_seq;
		m->src = old->src;
	}

	if(timer && !timer->memory)
		tw_error(TW_LOC, "no membuf here!");

	return timer;
}

void
tcp_timer_cancel(tw_event * timer, tw_bf * bf, tcp_message * old, tw_lp * lp)
{
	old->RC.timer_ts = timer->recv_ts;

	if(!timer->memory)
		tw_error(TW_LOC, "No membuf on timer!");

	tw_memory_free(lp, timer->memory, g_tcp_fd);
	timer->memory = NULL;

	rn_timer_cancel(lp, &timer);

#if TCP_DEBUG
	printf("\t%ld: cancel RTO for %lf\n", lp->id, old->RC.timer_ts);
#endif
}

tw_event	*
tcp_rc_timer_cancel(tcp_message * old, tw_lp * lp)
{
	tw_event * timer = NULL;

	if(!old->RC.timer_ts)
		tw_error(TW_LOC, "No prev timer ts!");

	if(!timer)
		tw_error(TW_LOC, "Why cannot I create this timer now?");

	timer = rn_timer_init(lp, old->RC.timer_ts);
	timer->memory = tw_memory_free_rc(lp, g_tcp_fd);

	// more debug
	if(timer->memory)
	{
		tcp_message *m = tw_memory_data(timer->memory);

		if(m->src != old->src)
			tw_error(TW_LOC, "bad membuf!");
	} else
		tw_error(TW_LOC, "Could not unfree membuf!");

	return timer;
}
