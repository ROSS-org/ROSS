#include <ip.h>

void
ip_packet_drop(ip_state * state, rn_message * msg, tw_lp * lp)
{
	state->stats->s_ndropped++;

	if(msg->src == lp->id)
		state->stats->s_ndropped_source++;

#if VERIFY_IP
	printf("%ld: dropped src %ld, dst %ld on port %ld \n", 
		lp->id, msg->src, msg->dst, msg->port);
#endif
}

void
ip_rc_packet_drop(ip_state * state, rn_message * msg, tw_lp * lp)
{
	state->stats->s_ndropped--;

	if(msg->src == lp->id)
		state->stats->s_ndropped_source--;
}
