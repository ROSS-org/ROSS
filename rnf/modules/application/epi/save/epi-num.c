#include <epi.h>

void
epi_num_add(epi_state * state, tw_bf * bf, epi_agent * a, tw_lp * lp)
{
	tw_event	*e;
	rn_message	*m;

	long int	 i_time;
	long int	 next_move_time;

	// only send if home
	if(a->loc[0] != lp->id)
		return;

	// if the time is before 9:00 AM and the next move is at or before
	// 9:00 AM, then send type 2 event so num will not set a timer
	// for nine oclock that will be cancelled by next move.
	// Cannot just ignore event, because num needs midnight event
	// to calculate daily statistics.
	i_time = (int) tw_now(lp) % TWENTY_FOUR_HOURS;
	if (i_time < NINE_OCLOCK)
	{
		if (a->nloc > a->curr + 1)
		{
			next_move_time = (i_time + (int) a->dur[a->curr+1]) % TWENTY_FOUR_HOURS;
			if (next_move_time <= NINE_OCLOCK)
			{
				if (i_time > 0)
					return;

				e = rn_event_direct(a->num, 0.0, lp);
				m = tw_event_data(e);
				m->ttl = 2;
				rn_event_send(e);
				return;
			}
		}
	}

	e = rn_event_direct(a->num, 0.0, lp);

	m = tw_event_data(e);
	m->ttl = 1;
	
	rn_event_send(e);
}

void
epi_num_remove(epi_state * state, tw_bf * bf, epi_agent * a, tw_lp * lp)
{
	tw_event	*e;
	rn_message	*m;

	// only send if home
	if(a->loc[0] != lp->id)
		return;

	e = rn_event_direct(a->num, 0.0, lp);

	m = tw_event_data(e);
	m->ttl = 0;

	rn_event_send(e);
}
