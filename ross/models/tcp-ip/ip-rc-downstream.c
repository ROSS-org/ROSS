#include <ip.h>

ip_link	*
rc_getroute(ip_state * state, rn_message * msg, tw_lp * lp)
{
	ip_link		*link = NULL;

	int		 next_hop = -1;

	/*
	 * Get the next_hop destination from the forwarding table 
	 * for this IP packet.
	 */
	if(msg->src == lp->id && state->nlinks == 1)
	{
		link = state->links;
	} else if(NULL == (link = ip_getlink(state, msg->dst)))
	{
		next_hop = ip_route(state, msg->dst);

		if(next_hop >= 0 && next_hop < state->nlinks)
			link = &state->links[next_hop];
		else
			ip_rc_packet_drop(state, msg, lp);
	}

	return link;
}

void
ip_rc_downstream(ip_state * state, rn_message * msg, tw_lp * lp)
{
	tw_memory	*b;
	//tw_event	*e;
	tw_stime	 ts;

	ip_link		*l;
	ip_message	*ip;

	ts = 0.0;
	state->stats->s_nforward--;

	if(NULL == (l = rc_getroute(state, msg, lp)))
	{
		ip_rc_packet_drop(state, msg, lp);
		return;
	}

	b = tw_event_memory_get(lp);

	ip = tw_memory_data(b);
	ip->rc_link->last_sent = ip->rc_lastsent;

	tw_memory_alloc_rc(lp, b, g_ip_fd);

#if VERIFY_IP
	printf("%ld IP: RC sent DOWN src %ld, link %d, dst %ld, ts %lf \n", 
		lp->id, msg->src, l->link->addr, msg->dst, e->recv_ts);
#endif
}
