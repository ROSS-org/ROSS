#include <bgp.h>

/**
 * Send a BGP CONNECT message to complete the connection established process.
 *
 * BGP needs CONNECT message from transport layer.. upon
 * receiving ANY msg from another BGP router, the OPEN msg is created
 * and sent to that neighbor. Do not use these for anything other than
 * during the init phase.  I have changed the process somewhat so that each
 * peer on either side of a connection do not need to send the CONNECT.
 */
void
bgp_connect(bgp_state * state, bgp_nbr * n, tw_lp * lp)
{
	tw_event	*e;
	tw_memory	*hdr;
	tw_stime	 ts = 0.0;

	bgp_message	*m;

#if 1
	if(lp->id < n->id)
		ts = 0.0;
	else
		return;
#endif

#if VERIFY_BGP
	printf("%ld: BGP router coming online, connecting to %d: %lf \n", 
		lp->id, n->id, ts);
#endif

	state->stats->s_nconnects++;
	e = rn_event_new(n->id, ts, lp, DOWNSTREAM, BGP_CONNECT);

	// Create the BGP message header
	hdr = tw_memory_alloc(lp, g_bgp_fd);
	m = tw_memory_data(hdr);

	m->b.type = CONNECT;
	m->b.cause_bgp = 1;
	m->b.cause_ospf = 0;

	tw_event_memory_set(e, hdr, g_bgp_fd);
	rn_event_send(e);

	if(e == lp->pe->abort_event)
	{
		state->stats->s_nconnects_dropped++;
	} else
	{
		if(NULL == rn_getlink(state->m, n->id))
			if(-1 == rn_route(state->m, n->id))
				state->stats->s_nconnects_dropped++;
	}
}
