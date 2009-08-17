#include <ip.h>

#if 0
ip_link	*
rc_getlink(ip_state * state, rn_machine * me, rn_link * link)
{
	int	i;

	for(i = 0; i < me->nlinks; i++)
		if(state->links[i].link == link)
			return &state->links[i];

	tw_error(TW_LOC, "Unable to find link!");

	return NULL;
}
#endif

rn_link	*
rc_getroute(ip_state * state, rn_message * msg, tw_lp * lp)
{
	rn_machine	*me = rn_getmachine(lp->gid);
	rn_link		*link = NULL;

	/*
	 * Get the next_hop destination from the forwarding table 
	 * for this IP packet.
	 */
	if(msg->src == lp->gid && me->nlinks == 1)
		link = me->link;
	else if(NULL == (link = ip_route(me, msg)))
		link = rn_getlink(me, msg->dst);
#if 0
	else if(NULL == (link = rn_getlink(me, msg->dst)))
		link = ip_route(me, msg);
#endif

	return link;
}

void
ip_rc_downstream(ip_state * state, tw_bf * bf, rn_message * msg, tw_lp * lp)
{
	tw_memory	*b;

	rn_link		*l;

	ip_message	*ip;

	state->stats->s_nforward--;
	msg->size += g_ip_header;

	if(NULL == (l = rc_getroute(state, msg, lp)))
	{
		ip_rc_packet_drop(state, msg, lp);
		return;
	}

	if(NULL == (b = tw_memoryq_pop(state->link_q)))
		tw_error(TW_LOC, "no membuf!");

	if(b->ts != tw_now(lp))
		tw_error(TW_LOC, "Bad membuf dequeued!");

	ip = tw_memory_data(b);
	ip->link->last_sent = ip->last_sent;
	tw_memory_alloc_rc(lp, b, g_ip_fd);

	rn_reverse_event_send(l->addr, lp, DOWNSTREAM, msg->size);

	if(bf->c31)
	{
		state->stats->s_nnet_failures--;
		msg->ttl--;
	}
}
