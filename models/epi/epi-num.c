#include <epi.h>

void
epi_num_add(epi_state * state, tw_bf * bf, epi_agent * a, tw_lp * lp)
{
	tw_event	*e;
	rn_message	*m;

	long int	 i_time;
	int		 today;
	//long int	 next_move_time;

	today = (int) tw_now(lp) / 86400;
	i_time = (int) tw_now(lp) % TWENTY_FOUR_HOURS;

	// if the time is before 9:00 AM and the next move is at or before
	// 9:00 AM, then send type 2 event so num will not set a timer
	// for nine oclock that will be cancelled by next move.
	// Cannot just ignore event, because num needs midnight event
	// to calculate daily statistics.
#if 0
	if(i_time == 0)
	{
		e = rn_event_direct(a->num, 0.0, lp);
		m = tw_event_data(e);
		m->ttl = 2;
		rn_event_send(e);

		return;
	}
#endif

	if(i_time < NINE_OCLOCK || i_time >= 6120000)
		return;

	// only send if home
	if(a->loc[0] != lp->id)
		return;

	// only invoke network user model every Nth day
	if(g_epi_mod > 1 && (today % g_epi_mod) + 1 != g_epi_mod)
		return;

	e = rn_event_direct(a->num, 0.0, lp);

	m = tw_event_data(e);
	m->ttl = 1;

	rn_event_send(e);
}

void
epi_num_remove(epi_state * state, tw_bf * bf, epi_agent * a, tw_lp * lp)
{
#if 0
	tw_event	*e;
	rn_message	*m;

	// only send if home
	if(a->loc[0] != lp->id)
		return;

	if(!a->started)
		return;

	a->started = 0;

	e = rn_event_direct(a->num, 0.0, lp);
	if (e->state.b.abort)
		tw_error(TW_LOC, "ABORT event!");

	m = tw_event_data(e);
	m->ttl = 0;

	rn_event_send(e);
#endif
}
