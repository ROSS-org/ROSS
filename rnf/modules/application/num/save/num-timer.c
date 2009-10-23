/* num-timer.c  */
#include <num.h>

/*
 * the ROSS timer API is not so great.. 
 */

void
num_timer_cancel(tw_event * timer, tw_lp * lp)
{
	if(timer->memory)
		tw_error(TW_LOC, "Membuf on timer!");

	rn_timer_cancel(lp, &timer);
}

tw_event *
num_timer_start(tw_event * timer, tw_stime ts, tw_lp * lp)
{
	ts += tw_now(lp);

	if(timer && timer->memory)
		tw_error(TW_LOC, "Membuf on timer!");

	if(timer && timer->state.b.abort)
		tw_error(TW_LOC, "Timer ABORT event!");

	if(!timer || timer == lp->cur_event)
		timer = rn_timer_init(lp, ts);
	else
		rn_timer_reset(lp, &timer, ts);
//DEBUG
	//if (timer == 0x1aec81d0)
	//	tw_error(TW_LOC, "Event 0x1aec81d0");

	return timer;
}
