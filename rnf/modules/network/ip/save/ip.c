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

#if 0
{
	tw_stime	 now = tw_now(lp);
	tw_stime	 avg = 0.0;

	rn_machine	*me = rn_getmachine(lp->gid);
	rn_link		*l;

	int		 i;

	// if next minor reporting interval.. report after adding in delay
	if(g_ip_log_on && now >= state->minor &&
		(me->type > c_router && me->type < c_ha_router))
	{
		for(i = 0; i < me->nlinks; i++)
		{
			l = &me->link[i];

			avg += l->avg_delay;
			l->avg_delay = 0.0;
		}

		state->max_delay = max(state->max_delay, 
					avg / (tw_stime) g_ip_minor_interval);

		state->minor = now + g_ip_minor_interval;
	}

	if(g_ip_log_on && now >= state->major &&
		(me->type > c_router && me->type < c_ha_router))
	{
		// divide router capacity by max_delay and output float in range: 0..1)
		fprintf(g_ip_log, "%d %d %d %2.8lf\n", 
			me->type, (int) now / 86400, me->uid, 
			(float) state->max_delay / (float) state->capacity);

		fflush(NULL);
		fsync(fileno(g_ip_log));

		state->major = now + g_ip_major_interval;
	}
}
#endif
}

void
ip_rc_event_handler(ip_state * state, tw_bf * bf, rn_message * msg, tw_lp * lp)
{
	tw_memory	*b;

	//ip_message	*ip;

	// put IP membuf back onto event
	if(msg->src != lp->gid && NULL == (b = tw_memory_free_rc(lp, g_ip_fd)))
		tw_error(TW_LOC, "No IP header!");

	if(b)
	{
		//ip = tw_memory_data(b);
		//ip->rc_link->last_sent = ip->rc_lastsent;

		tw_event_memory_get_rc(lp, b, g_ip_fd);
	}

	if(msg->type == DOWNSTREAM)
	{
		if(msg->src == lp->gid)
		{
			ip_rc_downstream(state, msg, lp);
			msg->size -= g_ip_header;
		} else
		{
			tw_error(TW_LOC, "RC donwstream pkt not from self?");
		}
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
			{
				ip_rc_downstream(state, msg, lp);
			}
		}
	}

	if(b)
		tw_event_memory_get_rc(lp, b, g_ip_fd);
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
