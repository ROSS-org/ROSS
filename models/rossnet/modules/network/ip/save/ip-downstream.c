#include <ip.h>

rn_link	*
getroute(ip_state * state, rn_message * msg, tw_lp * lp)
{
	rn_machine	*me = rn_getmachine(lp->gid);
	rn_link		*link = NULL;

	//int		 next_hop = -1;

	/*
	 * Get the next_hop destination from the forwarding table 
	 * for this IP packet.
	 */
	if(msg->src == lp->gid && me->nlinks == 1)
		link = me->link;
	else if(NULL == (link = ip_route(me, msg)))
	 	link = rn_getlink(me, msg->dst);

	if(NULL == link)
	{
		state->stats->s_nnet_failures++;
		ip_packet_drop(state, msg, lp);

		return NULL;
	}

	return link;
}

tw_event	*
forward(ip_state * state, tw_bf * bf, rn_message * msg, tw_lp * lp)
{
	//tw_memory	*b;
	tw_event	*e;
	tw_stime	 ts;

	rn_link		*l;

	//ip_message	*ip;

	ts = 0.0;
	state->stats->s_nforward++;

	if(NULL == (l = getroute(state, msg, lp)))
		return NULL;

	//b = tw_memory_alloc(lp, g_ip_fd);

	//ip = tw_memory_data(b);
	//ip->rc_link = l;
	//ip->rc_lastsent = l->last_sent;

	if(tw_now(lp) > l->last_sent)
		l->last_sent = tw_now(lp);
	else
		ts = l->last_sent - tw_now(lp);

	l->last_sent += (msg->size) / l->bandwidth;
	ts += ((msg->size) / l->bandwidth) + l->delay;

	//l->avg_delay += ts;
	l->avg_delay += (msg->size);

	e = rn_event_new(l->addr, ts, lp, DOWNSTREAM, msg->size);
	//tw_event_memory_set(e, b, g_ip_fd);

	if((bf->c31 = (e == lp->pe->abort_event && e->recv_ts <= g_tw_ts_end)))
	{
		tw_error(TW_LOC, "Got abort event ...");
		state->stats->s_nnet_failures++;
		msg->ttl++;
	}

#if VERIFY_IP
	printf("\t\t%lld IP FWD: to %lld at ts %lf (bw %lf del %lf)\n", 
		lp->gid, l->addr, e->recv_ts, l->bandwidth, l->delay);
#endif

	return e;
}

void
ip_downstream_source(ip_state * state, tw_bf * bf, rn_message * msg, tw_lp * lp)
{
	tw_event	*e;

	msg->size += g_ip_header;

	if(NULL != (e = forward(state, bf, msg, lp)))
		rn_event_send(e);
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

	if(e == lp->pe->abort_event)
	{
		printf("got abort event in IP\n");
		rn_event_send(e);
		return;
	}

	// If I was not the destination, then must be forwarding this pkt
	if(!lp->pe->cur_event->memory)
		tw_error(TW_LOC, "No membuf on forward event!");

	//if(lp->cur_event->memory && e->memory->next && !e->state.b.abort)
	if(lp->pe->cur_event->memory && e->memory && e != lp->pe->abort_event)
		tw_error(TW_LOC, "Unable to forward membufs!");

	e->memory = lp->pe->cur_event->memory;
	lp->pe->cur_event->memory = NULL;

	// fixup the message source
	m = tw_event_data(e);

	m->src = msg->src;
	m->dst = msg->dst;
	m->port = msg->port;
	m->ttl = msg->ttl;

	rn_event_send(e);
}
