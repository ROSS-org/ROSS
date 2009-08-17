#include <bgp.h>

/*
 * Reset the MRAI timer
 */
void
bgp_mrai_tmr(bgp_state * state, tw_lp * lp)
{
	if(state->tmr_mrai && state->tmr_mrai->recv_ts >= tw_now(lp))
		return;

	// keep the keepalive timer going
	state->tmr_mrai = bgp_timer_start(lp,
					  state->tmr_mrai,
					  state->as->mrai_interval,
					  MRAITIMER);
}

void
bgp_mrai_timer(bgp_state * state, tw_bf * bf, bgp_message * msg, tw_lp * lp)
{
	tw_memory	*b;
	bgp_route	*r;

	int		 i;

	state->tmr_mrai = NULL;

	msg->b.update = 1;
	msg->b.cause_bgp = 1;
	msg->b.cause_ospf = 0;

	// Foreach route in the RIB with dirty bit set, send updates to nbrs
	for(i = 0; i < g_rn_nas; i++)
	{
		b = state->rib[i];
		r = tw_memory_data(b);

		// Route has not been updated since previous MRAI timer fired
		if(!b || r->bit == TW_FALSE)
			continue;

		r->bit = TW_FALSE;
		bgp_update_all(state, msg, lp->id, r, lp);
	}
}
