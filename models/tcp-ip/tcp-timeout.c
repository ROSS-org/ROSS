#include <tcp.h>

void
tcp_timeout(tcp_state * state, tw_bf * bf, tcp_message * in, tw_lp * lp)
{
	if(in->seq_num < state->unack || state->unack >= state->len)
		return;

	bf->c1 = 1;

#if TCP_DEBUG
	printf("\t%ld: timeout, last sent %lf\n", lp->id, state->lastsent);
#endif

	in->dst = state->ssthresh;
	state->ssthresh = (min(((int)state->cwnd + 1), state->recv_wnd) / 2) * state->mss;

	// CHANGED
	in->RC.cwnd = state->cwnd;
	state->cwnd = 1;

	in->RC.lastsent = state->lastsent;
	tcp_event_send(state, lp->id, 0.0, in->src, TCP_MTU, state->unack, 0, lp);

	in->ack = state->seq_num;
	state->seq_num = state->unack + state->mss;
	state->stats->tout++;

	// PUT IN MAX TIME CHANGE
	in->RC.dup_count = state->rto;
	state->rto = state->rto * 2;	
	state->rto_seq++;

	state->timer = tcp_timer_reset(state->timer, bf, in, 
					tw_now(lp) + state->rto, state->unack, lp);

	// state-save into event
	in->RC.rtt_seq = state->rtt_seq;
	state->rtt_seq = state->unack;

	in->RC.rtt_time = state->rtt_time;
	state->rtt_time = 0;
}
