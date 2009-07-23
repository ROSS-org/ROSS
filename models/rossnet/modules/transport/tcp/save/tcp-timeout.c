#include <tcp.h>

void
tcp_timeout(tcp_state * state, tw_bf * bf, tcp_message * in, tw_lp * lp)
{
	if(in->seq_num < state->unack || state->unack >= state->len)
		return;

	bf->c1 = 1;

#if TCP_DEBUG
//if(lp->id == 9377)
	printf("\n\t%lld: timeout %lf\n", lp->id, tw_now(lp));
#endif

	//in->dst = state->ssthresh;
	state->ssthresh = (min(((int)state->cwnd + 1), state->recv_wnd) / 2) * state->mss;

	//in->RC.cwnd = state->cwnd;
	state->cwnd = 1;

	//in->RC.lastsent = state->lastsent;
	tcp_event_send(state, lp->id, 0.0, state->connection, TCP_MTU, state->unack, 0, lp);

	in->ack = state->seq_num;
	//state->seq_num = state->unack + state->mss;
	state->stats->tout++;
	state->stats->sent++;

	//in->RC.dup_count = state->rto;
	state->rto = state->rto * 2;
	state->rto_seq++;

	if(state->rto < 60.0)
	{
#if TCP_DEBUG
//if(lp->id == 9377)
	printf("\t%lld: timeout reset to %lf (was %lf) (unack %d)\n\n", lp->id, tw_now(lp) + state->rto, state->timer->recv_ts, state->unack);
#endif
		state->timer = tcp_timer_reset(state->timer, bf, in, 
					tw_now(lp) + state->rto, state->unack, lp);
	} else
	{
#if TCP_DEBUG
//if(lp->id == 9377)
	printf("\t%lld: timeout cancelled (was %lf) (unack %d)\n\n", lp->id, state->timer->recv_ts, state->unack);
		state->timer = NULL;
#endif
		rn_event_send(rn_event_new(lp->gid, 0.0, lp, UPSTREAM, 0));
	}

	// state-save into event
	//in->RC.rtt_seq = state->rtt_seq;
	state->rtt_seq = state->unack;

	//in->RC.rtt_time = state->rtt_time;
	state->rtt_time = 0;
}
