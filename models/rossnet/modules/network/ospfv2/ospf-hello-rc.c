#include <ospf.h>

void
ospf_hello_packet_rc(ospf_state * state, ospf_nbr * nbr, tw_bf * bf, 
				ospf_message * msg, tw_lp * lp)
{
#if 0
	ospf_hello	*r;

	if(bf->c1)
	{
		state->stats->s_drop_hello_int--;
		return;
	}

	if(bf->c2)
	{
		state->stats->s_drop_poll_int--;
		return;
	}

	r = &msg->data;

	if(!r)
		tw_error(TW_LOC, "Unable to read HELLO message (rc)!");

	rc_swap_uint(&nbr->priority, &r->priority);
	rc_swap_uint(&nbr->designated_r, &r->designated_r);
	rc_swap_uint(&nbr->b_designated_r, &r->b_designated_r);
#endif
}
