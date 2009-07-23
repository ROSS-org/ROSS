#include <ospf.h>

tw_event *
ospf_timer_cancel(tw_event * tmr, tw_lp * lp)
{
	tw_memory	*b;

	if(tmr && NULL != tmr->memory)
	{
		b = tmr->memory;
		tmr->memory = NULL;
		tw_memory_free(lp, b, g_ospf_fd);
	}

	if(tmr && tw_now(lp) == tmr->recv_ts)
		tmr = NULL;
	else if(tmr)
	{
		rn_timer_cancel(lp, &tmr);
	}

	return NULL;
}

/*
 * start the timer 
 */
tw_event *
ospf_timer_start(ospf_nbr * nbr, tw_event * tmr, tw_stime ts, int type, tw_lp * lp)
{
	tw_memory	*b;
	ospf_message	*m;

	ts = tw_now(lp) + ts;

/*
	if(tmr && tmr->memory && tmr->memory->next)
		tw_error(TW_LOC, "More than one membuf on timer!");
*/

	/* THIS CONDITIONAL IS NECESSARY!! */
	/* Don't do anything if the timer is already set for this time! */
	if(tmr && tw_now(lp) >= tmr->recv_ts)
		tmr = NULL;
	else if(tmr && ts == tmr->recv_ts)
		return tmr;

	if(!tmr)
		tmr = rn_timer_init(lp, ts);
	else
		rn_timer_reset(lp, &tmr, ts);

	// Timer library will not allocate timers past end time!
	if(!tmr)
		return NULL;

/*
	if(nbr)
		printf("%ld OSPF: TIMER %d ev %d, type: %d, ts %lf \n", 
					lp->id, (int) tmr, type, nbr->id, tmr->recv_ts);
*/

	if(NULL != tmr->memory)
		b = tmr->memory;
	else
	{
		b = tw_memory_alloc(lp, g_ospf_fd);
		tw_event_memory_set(tmr, b, g_ospf_fd);
	}

	m = tw_memory_data(b);

	m->type = type;
	m->data = nbr;

	return tmr;
}
