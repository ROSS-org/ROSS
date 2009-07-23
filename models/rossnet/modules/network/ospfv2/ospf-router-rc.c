#include <ospf.h>

void
ospf_rc_event_handler(ospf_state * state, tw_bf *bf, rn_message *rn_msg, tw_lp *lp)
{
#if 0
	ospf_message	*msg;
	ospf_nbr	*nbr;

	msg = rn_message_data(rn_msg);
	nbr = ospf_int_getinterface(state, rn_msg->src);

	if(!nbr)
		nbr = ospf_util_data(msg);

	if(msg->type == OSPF_IP)
		tw_error(TW_LOC, "IP packet forwarding not reversible yet!");

	switch(msg->type)
	{
		case OSPF_HELLO_MSG:
			ospf_hello_packet_rc(state, nbr, bf, msg, lp);
			state->stats->s_e_hello_in--;
			break;
		case OSPF_HELLO_SEND:
			// need to restart timer
			nbr->hello_timer =
				ospf_timer_start(nbr, nbr->hello_timer,
					 nbr->hello_interval,
					 OSPF_HELLO_SEND, lp);
			break;
		case OSPF_HELLO_TIMEOUT:
			state->stats->s_e_hello_timeouts--;
			ospf_nbr_event_handler(state, nbr,
					rc_ospf_nbr_inactivity_timer_ev, lp);
			break;
		default:
			tw_error(TW_LOC, "Unhandled event type (%d) in "
					 "rc_event_handler!", msg->type);
	}
#endif
}
