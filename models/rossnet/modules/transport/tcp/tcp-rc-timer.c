#include <tcp.h>

#if 0
tcp_timer_reset(tw_event * timer, tw_bf * bf, tcp_message * old, 

	tcp_message	*m;

	if(timer && timer->recv_ts == ts)
		tw_error(TW_LOC, "%ld: attempt to reset timer to cur ts %lf!", 
			 lp->gid, ts);

	// if this IS the timer firing, then do create new timer!
	if((bf->c14 = (!timer || timer == lp->pe->cur_event)))
		old->RC.timer = timer;

		timer = NULL;
		timer = rn_timer_init(lp, ts);
	else
		old->RC.timer_ts = timer->recv_ts;
		rn_timer_reset(lp, &timer, ts);

	if((bf->c13 = (timer != NULL)))
		if(!timer->memory)
			timer->memory = tw_memory_alloc(lp, g_tcp_fd);

		m = tw_memory_data(timer->memory);
		old->RC.timer_seq = m->seq_num;
		m->src = old->src;
		m->seq_num = sn;
#endif

tw_event	*
tcp_rc_timer_reset(tw_event * timer, tw_bf * bf, tcp_message * old, tw_lp * lp)
{
	if(bf->c13)
	{
		tcp_message	*m;

		if(NULL == timer->memory)
			timer->memory = tw_memory_alloc(lp, g_tcp_fd);

		m = tw_memory_data(timer->memory);
		m->seq_num = old->RC.timer_seq;
		m->src = old->src;
	}

	if(bf->c14)
	{
		// If I init'd a timer, cancel it.
		if(timer)
		{
#if VERIFY_TCP_TIMER
			printf("\t%lld: RC reset, cancel timer at %lf (%ld)\n", 
				lp->gid, timer->recv_ts, (long int) timer);
#endif

			tw_memory_alloc_rc(lp, timer->memory, g_tcp_fd);
			timer->memory = NULL;

			rn_timer_cancel(lp, &timer);
		}

		// restore pointer to previous timer
		timer = old->RC.timer;
		old->RC.timer = NULL;

	} else if(bf->c17)
	{
		return NULL;
	} else
	{
		if(!old->RC.timer_ts)
			tw_error(TW_LOC, "%lld: No timer ts: now %lf",
				 lp->gid, tw_now(lp));

		if(timer == NULL)
		{
			timer = rn_timer_init(lp, old->RC.timer_ts);
			old->RC.timer_ts = 0.0;
		} else
		{
			rn_timer_reset(lp, &timer, old->RC.timer_ts);
			old->RC.timer_ts = 0.0;
		}

		if(timer)
			printf("\t%lld: reset timer to %lf\n", lp->gid, timer->recv_ts);
		else
			printf("\t%lld: unable to reset timer\n", lp->gid);
	}

#if VERIFY_TCP_TIMER
	if(timer)
		printf("\t%lld: restored RTO to %lf\n", lp->gid, timer->recv_ts);
	else
		printf("\t%lld: restored RTO no previous\n", lp->gid);
#endif

	return timer;
}

#if 0
tcp_timer_cancel(tw_event * timer, tw_bf * bf, tcp_message * old, tw_lp * lp)

	old->RC.timer_ts = timer->recv_ts;

	if(timer == lp->pe->cur_event)
		return;

	tw_memory_free(lp, timer->memory, g_tcp_fd);
	timer->memory = NULL;

	rn_timer_cancel(lp, &timer);
#endif

tw_event	*
tcp_rc_timer_cancel(tcp_message * old, tw_lp * lp)
{
	tw_event * timer = NULL;

	if(!old->RC.timer_ts)
		tw_error(TW_LOC, "%lld: No previous timer!", lp->gid);

	timer = rn_timer_init(lp, old->RC.timer_ts);
	timer->memory = tw_memory_free_rc(lp, g_tcp_fd);

#if VERIFY_TCP_TIMER
	if(timer)
		printf("%lld: TCP RC cancel at %lf: new %lf\n",
			lp->gid, tw_now(lp), timer->recv_ts);
#endif

	return timer;
}
