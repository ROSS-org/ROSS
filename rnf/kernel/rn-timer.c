#include <rossnet.h>

tw_event	*
rn_timer_simple(tw_lp * lp, tw_stime ts)
{
	tw_event	*tmr;
	rn_lp_state	*state;
	rn_message	*r;
	rn_stream	*s;

	state = lp->cur_state;
	tmr = tw_event_new(lp->gid, ts, lp);

	r = tw_event_data(tmr);
	s = rn_getstream(lp);

	r->type = TIMER;
	r->dst = lp->gid;
	r->src = lp->gid;
	r->timer = state->cur_lp;
	r->size = s->cur_layer;
	r->port = s->port;

	tw_event_send(tmr);

	return tmr;
}

tw_event	*
rn_timer_init(tw_lp * lp, tw_stime ts)
{
	tw_event	*e;
	rn_lp_state	*state;
	rn_message	*msg;
	rn_stream	*s;

	e = tw_timer_init(lp, ts);

	if(!e)
		return NULL;

	msg = tw_event_data(e);
	state = lp->cur_state;
	s = rn_getstream(lp);

	msg->type = TIMER;
	msg->timer = state->cur_lp;
	msg->src = lp->gid;
	msg->dst = lp->gid;
	msg->size = s->cur_layer;
	msg->port = s->port;
	msg->ttl = 0;

	return e;
}

tw_event	*
rn_timer_reset(tw_lp * lp, tw_event ** e, tw_stime ts)
{
	rn_message	*msg;
	rn_lp_state	*state;

	tw_timer_reset(lp, e, ts);

	if(!*e)
		return *e;

	state = (rn_lp_state *) lp->cur_state;
	msg = (rn_message *) tw_event_data(*e);

	msg->type = TIMER;
	msg->timer = state->cur_lp;
	msg->src = lp->gid;
	msg->dst = lp->gid;

	return *e;
}

void
rn_timer_cancel(tw_lp * lp, tw_event ** e)
{
	tw_timer_cancel(lp, e);
	e = NULL;
}
