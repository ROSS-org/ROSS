#include <ip.h>

/*
 * Initializes IP layer LP
 */
void
ip_init(ip_state * state, tw_lp * lp)
{
	rn_machine	*me;

	int		 i;

	me = rn_getmachine(lp->gid);
	state->stats = tw_calloc(TW_LOC, "", sizeof(ip_stats), 1);

	// in seconds..
	state->minor = 32400 + g_ip_minor_interval;
	state->major = 32400 + g_ip_major_interval;

	state->link_q = tw_memoryq_init();

	for(i = 0; i < me->nlinks; i++)
		state->capacity += me->link[i].bandwidth;
}

void
ip_event_handler(ip_state * state, tw_bf * bf, rn_message * msg, tw_lp * lp)
{
	if(msg->type == DOWNSTREAM)
	{
		if(msg->src == lp->gid)
			ip_downstream_source(state, bf, msg, lp);
		else
			tw_error(TW_LOC, "Downstream pkt not from self? (%lld %lld, %d, %lld)",
				 msg->src, lp->gid, msg->type, msg->dst);
	} else
	{
		if(msg->dst == lp->gid)
			ip_upstream(state, msg, lp);
		else
			ip_downstream_forward(state, bf, msg, lp);
	}
}

void
ip_rc_event_handler(ip_state * state, tw_bf * bf, rn_message * msg, tw_lp * lp)
{
	if(msg->type == DOWNSTREAM)
	{
		if(msg->src == lp->gid)
		{
			ip_rc_downstream(state, bf, msg, lp);
			msg->size -= g_ip_header;
		} else
			tw_error(TW_LOC, "RC donwstream pkt not from self?");
	} else
	{
		if(msg->dst == lp->gid)
		{
			ip_rc_upstream(state, msg, lp);
		} else
		{
			if(++msg->ttl == 1)
			{
				state->stats->s_ndropped_ttl--;
				ip_rc_packet_drop(state, msg, lp);
			} else
				ip_rc_downstream(state, bf, msg, lp);
		}
	}
}

void
ip_final(ip_state * state, tw_lp * lp)
{
	g_ip_stats->s_ncomplete += state->stats->s_ncomplete;
	g_ip_stats->s_nforward += state->stats->s_nforward;
	g_ip_stats->s_ndropped += state->stats->s_ndropped;
	g_ip_stats->s_ndropped_ttl += state->stats->s_ndropped_ttl;
	g_ip_stats->s_ndropped_source += state->stats->s_ndropped_source;
	g_ip_stats->s_nnet_failures += state->stats->s_nnet_failures;
	g_ip_stats->s_avg_ttl += state->stats->s_avg_ttl;

	g_ip_stats->s_max_ttl = max(g_ip_stats->s_max_ttl, state->stats->s_max_ttl);
}
