#include <bgp.h>

void
bgp_notify_send(bgp_state * state, bgp_nbr * n, tw_lp * lp)
{
	tw_event	*e;
	tw_memory	*hdr;

	bgp_message	*m;

	state->stats->s_nnotify_sent++;
	e = rn_event_new(n->id, 0.0, lp, DOWNSTREAM, BGP_NOTIFY);

	hdr = tw_memory_alloc(lp, g_bgp_fd);
	m = tw_memory_data(hdr);

	m->b.type = NOTIFICATION;

	tw_event_memory_set(e, hdr, g_bgp_fd);
	rn_event_send(e);

#if VERIFY_BGP
	fprintf(state->log, "\tsent notify to nbr %d at %f\n", n->id, tw_now(lp));
#endif
}

// notification (error) message
void
bgp_notify(bgp_state * state, tw_bf * bf, int src, tw_lp * lp)
{
	bgp_nbr		*n;

	n = bgp_getnbr(state, src);

#if VERIFY_BGP
	printf("%ld BGP: lost contact with %d \n", lp->id, n->id);
#endif

	//msg->old_last_update = n->last_update;
	n->last_update = floor(tw_now(lp)) - state->as->hold_interval; // - state->mrai;

	bf->c1 = 1;
}
