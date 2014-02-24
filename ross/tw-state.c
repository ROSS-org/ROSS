#include <ross.h>

void
tw_state_rollback(tw_lp *lp, tw_event *revent)
{
	lp->pe->cur_event = revent;
	lp->kp->last_time = revent->recv_ts;

	(*lp->type.revent)(
		lp->cur_state,
		&revent->cv,
		tw_event_data(revent),
		lp);
}
