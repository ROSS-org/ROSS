#include <epi.h>

void
epi_location_timer(epi_state * state, tw_lp * lp)
{
	tw_memory	*b;

	epi_agent	 *a;

	// state->tmr_remove fired, so reset time to next lowest agent
	// Note that queue may be empty because the only agent in this location died.
	if(NULL != (b = pq_top(g_epi_pq[lp->id])))
	{
		a = tw_memory_data(b);

		if(state->tmr_remove && state->tmr_remove->memory)
			tw_error(TW_LOC, "Membuf on state->tmr_remove!");

		if(a->ts_next == 0.0)
			tw_error(TW_LOC, "Agent state change at ts 0.0?");

		if(!state->tmr_remove || state->tmr_remove == lp->pe->cur_event)
			state->tmr_remove = tw_timer_init(lp, a->ts_next);
		else
			tw_timer_reset(lp, &state->tmr_remove, a->ts_next);
	} else
		state->tmr_remove = NULL;
}
