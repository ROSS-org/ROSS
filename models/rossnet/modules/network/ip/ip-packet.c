#include <ip.h>

void
ip_packet_drop(ip_state * state, rn_message * msg, tw_lp * lp)
{
	tw_event	*e;

	state->stats->s_ndropped++;

	if(msg->src == lp->gid)
		state->stats->s_ndropped_source++;

#if VERIFY_IP
	printf("%lld: dropped src %lld, dst %lld on port %d \n", 
		lp->gid, msg->src, msg->dst, msg->port);
#endif

	// Need to free the event otherwise it will simply be lost!
	e = rn_event_new(msg->dst, 0.0, lp, DOWNSTREAM, msg->size);

	if(e == lp->pe->abort_event)
		tw_event_free(lp->pe, e);
}

void
ip_rc_packet_drop(ip_state * state, rn_message * msg, tw_lp * lp)
{
	state->stats->s_ndropped--;

	if(msg->src == lp->gid)
		state->stats->s_ndropped_source--;

	rn_reverse_event_send(msg->dst, lp, DOWNSTREAM, msg->size);
}
