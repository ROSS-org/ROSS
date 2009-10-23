#include <tcp.h>

void
tcp_timeout(tcp_state * state, tw_bf * bf, tcp_message * in, tw_lp * lp)
{
	if(in->seq_num < state->unack || state->unack >= state->len)
		return;

	bf->c1 = 1;

#if TCP_DEBUG
	printf("\n\t%lld: timeout %lf\n", lp->id, tw_now(lp));
#endif

	in->RC.ssthresh = state->ssthresh;
	state->ssthresh = (min(((int)state->cwnd + 1), state->recv_wnd) / 2) * state->mss;

	in->RC.cwnd = state->cwnd;
	state->cwnd = 1;

	in->RC.lastsent = state->lastsent;
	tcp_event_send(state, lp->gid, 0.0, state->connection, TCP_MTU, state->unack, 0, lp);

	in->RC.seq_num = state->seq_num;
	state->seq_num = state->unack + state->mss;
	state->stats->tout++;
	state->stats->sent++;

	in->RC.dup_count = state->rto;
	state->rto *= 2;
	state->rto_seq++;

#if 0
	if(state->rto < g_tw_ts_end)
	{
#if TCP_DEBUG
	printf("\t%lld: timeout reset to %lf (was %lf) (unack %d)\n\n", lp->gid, tw_now(lp) + state->rto, state->timer->recv_ts, state->unack);
#endif
		state->timer = tcp_timer_reset(state->timer, bf, in, 
					tw_now(lp) + state->rto, state->unack, lp);
	} else
	{
#if TCP_DEBUG
	printf("\t%lld: timeout cancelled (was %lf) (unack %d)\n\n", lp->gid, state->timer->recv_ts, state->unack);
		state->timer = NULL;
#endif
		rn_event_send(rn_event_new(lp->gid, 0.0, lp, UPSTREAM, 0));
	}
#endif

	state->timer = tcp_timer_reset(state->timer, bf, in, 
					tw_now(lp) + state->rto, state->unack, lp);

	in->RC.rtt_seq = state->rtt_seq;
	state->rtt_seq = state->unack;

	in->RC.rtt_time = state->rtt_time;
	state->rtt_time = 0;
}
