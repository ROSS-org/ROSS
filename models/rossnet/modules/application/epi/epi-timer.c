#include <epi.h>

void
epi_location_timer(epi_state * state, tw_lp * lp)
{
	epi_agent	 *a;

	// state->tmr_remove fired, so reset time to next lowest agent
	// Note that queue may be empty because the only agent in this location died.
	if(NULL != (a = pq_top(state->pq)))
	{
		if(state->tmr_remove && state->tmr_remove->memory)
			tw_error(TW_LOC, "Membuf on state->tmr_remove!");

		if(!state->tmr_remove || state->tmr_remove == lp->pe->cur_event)
			state->tmr_remove = rn_timer_init(lp, a->ts_next);
		else
			state->tmr_remove =
				rn_timer_reset(lp, &state->tmr_remove, a->ts_next);
	} else
		state->tmr_remove = NULL;
}
