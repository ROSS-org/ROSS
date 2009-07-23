#include <ip.h>

ip_link	*
ip_getlink(ip_state * state, tw_lpid dst)
{
	int	i;

	for(i = 0; i < state->nlinks; i++)
		if(state->links[i].addr == dst)
			return &state->links[i];

	return NULL;
}

ip_link	*
getroute(ip_state * state, rn_message * msg, tw_lp * lp)
{
	ip_link		*link = NULL;

	int		 next_hop = -1;

	/*
	 * Get the next_hop destination from the forwarding table 
	 * for this IP packet.  Cases are:
	 *
	 * my node has 1 link: return that link as next_hop
	 * my node is not directly connected to dest: get next_hop link from routing
	 * my node is directly connected, return that link
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
			ip_packet_drop(state, msg, lp);
	}

	return link;
}

tw_event	*
forward(ip_state * state, tw_bf * bf, rn_message * msg, tw_lp * lp)
{
	tw_memory	*b;
	tw_event	*e;
	tw_stime	 ts;

	rn_message	*m;

	ip_link		*l;
	ip_message	*ip;

	ts = 0.0;
	state->stats->s_nforward++;

	if(NULL == (l = getroute(state, msg, lp)))
		return NULL;

	b = tw_memory_alloc(lp, g_ip_fd);

	ip = tw_memory_data(b);
	ip->rc_link = l;
	ip->rc_lastsent = l->last_sent;

	if(tw_now(lp) > l->last_sent)
		l->last_sent = tw_now(lp);
	else
		ts = l->last_sent - tw_now(lp);

	l->last_sent += msg->size / l->bandwidth;
	ts += (msg->size / l->bandwidth) + l->delay;

	e = tw_event_new(tw_getlp(l->addr), ts, lp);
	tw_event_memory_set(e, b, g_ip_fd);

	m = tw_event_data(e);
	m->size = msg->size;
	m->type = DOWNSTREAM;

	if((bf->c31 = (e->state.b.abort && e->recv_ts <= g_tw_ts_end)))
	{
		tw_error(TW_LOC, "Got abort event ...");
		state->stats->s_nnet_failures++;
		msg->ttl++;
	}

#if VERIFY_IP
	printf("%ld IP: sent DOWN src %ld, link %d, dst %ld, ts %lf \n", 
		lp->id, msg->src, l->addr, msg->dst, e->recv_ts);
#endif

	return e;
}

void
ip_downstream_source(ip_state * state, tw_bf * bf, rn_message * msg, tw_lp * lp)
{
	tw_event	*e;

	msg->size += g_ip_header;

	if(NULL != (e = forward(state, bf, msg, lp)))
		tw_event_send(e);
}

void
ip_downstream_forward(ip_state * state, tw_bf * bf, rn_message * msg, tw_lp * lp)
{
	tw_event	*e;

	rn_message	*m;

	if(--msg->ttl == 0)
	{
		state->stats->s_ndropped_ttl++;
		ip_packet_drop(state, msg, lp);

		return;
	}

	if(NULL == (e = forward(state, bf, msg, lp)))
		return;

	// TODO: Move this logic into rn_event_send

	// If I was not the destination, then must be forwarding this pkt
	if(!lp->cur_event->memory && !e->state.b.abort)
		tw_error(TW_LOC, "No membuf on forward event!");

	if(lp->cur_event->memory && e->memory->next && !e->state.b.abort)
		tw_error(TW_LOC, "Unable to forward membufs!");

	e->memory->next = lp->cur_event->memory;
	lp->cur_event->memory = NULL;

	// fixup the message source
	m = tw_event_data(e);

	m->type = msg->type;
	m->src = msg->src;
	m->dst = msg->dst;
	m->port = msg->port;
	m->ttl = msg->ttl;
	m->size = msg->size;

	tw_event_send(e);
}
