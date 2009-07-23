#include <ip.h>

void
ip_upstream(ip_state * state, rn_message * msg, tw_lp * lp)
{
	tw_event	*e;

	rn_message	*m;

	state->stats->s_avg_ttl += (g_rn_ttl - msg->ttl);
	state->stats->s_max_ttl = max(state->stats->s_max_ttl, g_rn_ttl - msg->ttl);
	state->stats->s_ncomplete++;

	e = tw_event_new(lp, 0.0, lp);
	m = tw_event_data(e);

	m->type = UPSTREAM;
	m->src = msg->src;
	m->dst = msg->dst;
	m->size = msg->size - g_ip_header;

	tw_event_send(e);

#if VERIFY_IP
		printf("%ld IP: sent UP src %ld, dst %ld, ts %lf \n", 
			lp->id, msg->src, msg->dst, tw_now(lp));
#endif
}

void
ip_rc_upstream(ip_state * state, rn_message * msg, tw_lp * lp)
{
	// not correct, must be state-saved!
	state->stats->s_max_ttl = max(state->stats->s_max_ttl, g_rn_ttl - msg->ttl);
	state->stats->s_avg_ttl -= (g_rn_ttl - msg->ttl);
	state->stats->s_ncomplete--;
}
