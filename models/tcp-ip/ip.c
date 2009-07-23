#include <ip.h>

#define VERIFY_IP 1

/*
 * Initializes IP layer LP
 */
void
ip_lp_init(ip_state * state, tw_lp * lp)
{
	state->stats = tw_vector_create(sizeof(ip_stats), 1);
	state->links = tw_vector_create(sizeof(ip_link), state->nlinks);

	if(g_ip_routing_simple)
		ip_routing_init_ft(state, lp);
}

void
ip_event_handler(ip_state * state, tw_bf * bf, rn_message * msg, tw_lp * lp)
{
	tw_memory	*b = NULL;

	if(msg->src != lp->id && NULL == (b = tw_event_memory_get(lp)))
		tw_error(TW_LOC, "No IP header!");

	if(msg->type == DOWNSTREAM)
	{
		if(msg->src == lp->id)
			ip_downstream_source(state, bf, msg, lp);
		else
			tw_error(TW_LOC, "Downstream pkt not from self?");
	} else
	{
		if(msg->dst == lp->id)
			ip_upstream(state, msg, lp);
		else
			ip_downstream_forward(state, bf, msg, lp);
	}

	if(b)
		tw_memory_free(lp, b, g_ip_fd);
}

void
ip_rc_event_handler(ip_state * state, tw_bf * bf, rn_message * msg, tw_lp * lp)
{
	tw_memory	*b;

	ip_message	*ip;

	// put IP membuf back onto event
	if(msg->src != lp->id && NULL == (b = tw_memory_free_rc(lp, g_ip_fd)))
		tw_error(TW_LOC, "No IP header!");

	if(b)
	{
		ip = tw_memory_data(b);
		ip->rc_link->last_sent = ip->rc_lastsent;

		tw_event_memory_get_rc(lp, b, g_ip_fd);
	}

	if(msg->type == DOWNSTREAM)
	{
		if(msg->src == lp->id)
		{
			ip_rc_downstream(state, msg, lp);
			msg->size -= g_ip_header;
		} else
		{
			tw_error(TW_LOC, "RC donwstream pkt not from self?");
		}
	} else
	{
		if(msg->dst == lp->id)
		{
			ip_rc_upstream(state, msg, lp);
		} else
		{
			if(++msg->ttl == 1)
			{
				state->stats->s_ndropped_ttl--;
				ip_rc_packet_drop(state, msg, lp);
			} else
			{
				ip_rc_downstream(state, msg, lp);
			}
		}
	}

	if(b)
		tw_event_memory_get_rc(lp, b, g_ip_fd);
}

void
ip_lp_final(ip_state * state, tw_lp * lp)
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
