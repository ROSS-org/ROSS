#include <bgp.h>

void
bgp_open_send(bgp_state * state, bgp_nbr * n, tw_lp * lp)
{
	tw_event	*e;
	tw_memory	*b;

	bgp_message	*m;

	e = rn_event_new(n->id, 0.0, lp, DOWNSTREAM, BGP_OPEN);

	// Create the BGP message header
	b = tw_memory_alloc(lp, g_bgp_fd);
	m = tw_memory_data(b);
	m->b.type = OPEN;

	// Must be caused by OSPF route newly available to iBGP neighbor
	// Also, to avoid counting on initial OPEN messages, tw_now > 0.0
	if(tw_now(lp) > 0.0 && state->as == rn_getas(rn_getmachine(n->id)))
	{
		m->b.cause_ospf = 1;
		m->b.cause_bgp = 0;
	} else if(tw_now(lp) > 0.0)
	{
		m->b.cause_ospf = 0;
		m->b.cause_bgp = 1;
	} else
	{
		m->b.cause_ospf = 0;
		m->b.cause_bgp = 0;
	}

	tw_event_memory_set(e, b, g_bgp_fd);

	rn_event_send(e);

#if VERIFY_BGP
	fprintf(state->log, "%ld BGP: Send OPEN to %d\n", lp->id, n->id);
#endif
}

/*
 * When we get a message from a neighbor for the first time, we need
 * to open the session.
 */
void
bgp_open(bgp_state * state, tw_bf * bf, bgp_message * msg, int src, tw_lp * lp)
{
	tw_memory      *b;

	bgp_route	*r;
	bgp_nbr		*n;

	int		 i;

	state->stats->d_opens = 1;
	state->stats->s_nopens++;

	// set the nbr to up
	n = bgp_getnbr(state, src);
	//n->up = TW_TRUE;

	// send a keepalive in response to the open
	//bgp_keepalive_send(n, lp);

	// now we will send this neighbor our full routing table
	// reflection: only send down in the BGP hierarchy

#if VERIFY_BGP
	fprintf(state->log, "\tsending routing table to %d\n", n->id);
#endif

	/*
	 * If route source is down (timed out) and iBGP node,
	 * then some internal OSPF route must have changed.
	 *
	 * If an eBGP node has timed out, it is not due to OSPF.
	 *
	 * Send an update for each route in our routing table.
	 */
	for(i = 0; i < g_rn_nas; i++)
	{
		if(NULL == (b = state->rib[i]))
			continue;

		r = tw_memory_data(b);
		msg->b.update = 1;

		// If OPEN from iBGP node, do not append to AS_PATH
		if(rn_getas(rn_getmachine(n->id)) == state->as)
		{
			if(r->dst == state->as->id)
				continue;

			bgp_update_send(state, n, r, msg, 1, lp);
		} else
		{
			if(r->dst == rn_getas(rn_getmachine(n->id))->id)
				continue;

			bgp_update_send(state, n, r, msg, 0, lp);
		}
	}
}
