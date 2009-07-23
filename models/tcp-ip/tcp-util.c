#include <tcp.h>

void
tcp_event_send(tcp_state * state, int src, tw_stime ts, int dst, int size,
		int seq_num, int ack, tw_lp * lp)
{
	tw_event	*e;

	tcp_message	*m;

#if TCP_DEBUG && 0
	printf("\t%ld: FWD at %lf to %d \n", lp->id, tw_now(lp) + ts, dst);
#endif

#if 0
	if(state->lastsent < tw_now(lp))
		state->lastsent = tw_now(lp);
	else
		ts = state->lastsent - tw_now(lp);

	xfer = size / state->host->link->bandwidth;
	state->lastsent += xfer;
	ts += xfer + state->host->link->delay;
#endif

	e = tw_event_new(state->connection, ts, lp);

	m = tw_event_data(e);
	m->src = src;
	m->dst = dst;
	m->size = size;
	m->ack = ack;
	m->seq_num = seq_num;

	tw_event_send(e);
}
