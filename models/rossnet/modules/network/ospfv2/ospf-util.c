#include <ospf.h>

tw_event	*
ospf_event_new(ospf_state * state, tw_lp * dest, tw_stime ts, tw_lp * src)
{
	tw_event       *event = NULL;

	event = rn_event_new(dest->gid, ts, src, DOWNSTREAM, 0);

	if(src->pe->abort_event)
	{
		state->stats->s_sent_lost++;

#if VERIFY_HELLO
		printf("%ld OSPF: Dropped event to dst %lld \n", 
			   src->id, dest->gid);
#endif
	}

	return event;
}

tw_event       *
ospf_event_send(ospf_state * state, tw_event * event, int type, tw_lp * src,
				int size, tw_memory * mem, int area)
{
	tw_memory	*b;

	ospf_message   *msg;

	// packet data
	if(mem)
		tw_event_memory_set(event, mem, g_ospf_fd);

	// packet header
	b = tw_memory_alloc(src, g_ospf_fd);
	msg = tw_memory_data(b);
	msg->type = type;
	msg->area = 0;

	tw_event_memory_set(event, b, g_ospf_fd);
	rn_message_setsize(tw_event_data(event), size + OSPF_HEADER);

	rn_event_send(event);

#if VERIFY_HELLO
	printf("%ld OSPF: sent type %d, dst %ld, ts = %lf \n", 
		   src->id, type, event->dest_lp->id, event->recv_ts);
#endif

	switch (type)
	{
		case OSPF_HELLO_MSG:
			state->stats->s_sent_hellos++;
			break;
		case OSPF_DD_MSG:
			state->stats->s_sent_dds++;
			break;
		case OSPF_LS_REQUEST:
			state->stats->s_sent_ls_requests++;
			break;
		case OSPF_LS_UPDATE:
			state->stats->s_sent_ls_updates++;
			break;
		case OSPF_LS_ACK:
			state->stats->s_sent_ls_acks++;
			break;
		default:
			state->stats->s_sent_unknown++;
	}

	return event;
}
