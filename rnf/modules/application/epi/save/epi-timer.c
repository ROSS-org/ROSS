#include <epi.h>

/*
 * the ROSS timer API is not so great.. 
 */

void
epi_timer_cancel(tw_event * timer, tw_lp * lp)
{
	if(!timer->memory)
		tw_error(TW_LOC, "No membuf on timer!");

	tw_memory_free(lp, timer->memory, g_epi_fd);
	timer->memory = NULL;

	rn_timer_cancel(lp, &timer);
}

tw_event *
epi_timer_start(epi_agent * a, tw_event * timer, tw_stime ts, tw_lp * lp)
{
	if(!timer || timer == lp->cur_event || timer->state.b.abort == 1)
	{
		timer = NULL;
		timer = rn_timer_init(lp, ts);
	} else
	{
		rn_timer_reset(lp, &timer, ts);
	}

	if(NULL != timer)
		timer->memory = (tw_memory *) a;

	if(timer && timer->state.b.abort == 1)
		tw_error(TW_LOC, "abort timer?");

	return timer;
}
