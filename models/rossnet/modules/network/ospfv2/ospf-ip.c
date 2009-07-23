#include <ospf.h>
#include <tcp.h>

void
ospf_ip_route(ospf_state * state, tw_bf * bf, rn_message * rn_msg, tw_lp * lp)
{
	tw_event	*e;

/*
	rn_message	*m;

	ospf_message	*ospf;
	ospf_message	*msg;

	tcp_message	*tcp_in;
	tcp_message	*tcp_out;
*/
	unsigned int	 dest;
	unsigned int	 d;

	tw_error(TW_LOC, "Should not be getting IP packets.. ");

	//msg = rn_message_data(rn_msg);

#if PLEASE_FIX_ME
	d = (rn_msg->final_dest / 1) - (g_rn_nrouters / 1) + (g_rn_nrouters - g_rn_npops);
#endif
	dest = rn_route(state->m, d);

	if(0 && dest == 0xff || dest == -1)
	{
#if PLEASE_FIX_ME
		dest = rn_msg->final_dest;
#endif

		e = ospf_event_new(state, tw_getlp(dest), lp);

		// I think this size is correct, probably not though
		tw_printf(TW_LOC, "Check OSPF ip pkt size! \n");
		ospf_event_send(state, e, OSPF_IP, lp, 
				rn_message_getsize(tw_event_data(lp->cur_event)), 
				NULL, state->area);

/*
		e = ospf_event_send(state, tw_getlp(dest), 
				OSPF_IP, lp, LINK_TIME, NULL, state->area);
*/

#if 0
		tcp_in = ospf_util_data(msg);

		tcp = rn_event_data(e);

		tcp->MethodName = tcp_in->MethodName;
		tcp->ack = tcp_in->ack;
		tcp->source = tcp_in->source;
		tcp->dest = tcp_in->dest;
		tcp->seq_num = tcp_in->seq_num;
#endif

#if 0
		if(rn_msg->final_dest == 1010)
		{
			printf("%d: tcp_in src = %d, tcp_in dst = %f\n",
				lp->id,
				tcp_in->source,
				tcp_in->dest);
			printf("%d: tcp src = %d, tcp dst = %f\n",
				lp->id,
				tcp->source,
				tcp->dest);
			printf("%d: %d \n", lp->id, rn_msg->final_dest);
		}
#endif

		return;
	} else
	{
		dest = state->nbr[dest].id;
	}

	// transfer all membufs from inbound IP to outbound
	e->memory = lp->cur_event->memory;
	lp->cur_event->memory = NULL;

	e = ospf_event_new(state, tw_getlp(dest), lp);

	tw_printf(TW_LOC, "Check my OSPF ip pkt size 2! \n");
	e = ospf_event_send(state, e, OSPF_IP, lp, 
				rn_message_getsize(tw_event_data(lp->cur_event)), 
				NULL, state->area);

#if 0
	m = tw_event_data(e);

	if(rn_getmachine(rn_msg->src)->type == c_host)
		tcp_in = rn_message_data(rn_msg);
	else
		tcp_in = ospf_util_data(msg);

	tcp = ospf_util_data(rn_message_data(m));

	tcp->MethodName = tcp_in->MethodName;
	tcp->ack = tcp_in->ack;
	tcp->source = tcp_in->source;
	tcp->dest = tcp_in->dest;
	tcp->seq_num = tcp_in->seq_num;
#endif

#if 0
	if(rn_msg->final_dest == 1010)
	{
		printf("%d: tcp_in src = %d, tcp_in dst = %f\n",
				lp->id, tcp_in->source, tcp_in->dest);
		printf("%d: tcp src = %d, tcp dst = %f\n",
				lp->id, tcp->source, tcp->dest);
	}
#endif

	return;
}

