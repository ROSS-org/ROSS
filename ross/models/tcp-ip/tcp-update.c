#include <tcp.h>

/*
 * Update the Control Window
 */
void
tcp_update_cwnd(tcp_state * state, tw_bf * bf, tcp_message * in, tw_lp * lp)
{
	if((bf->c3 = ((state->cwnd * state->mss) < state->ssthresh
		  && state->cwnd * state->mss < TCP_SND_WND)))
	{
		in->RC.cwnd = state->cwnd;
		state->cwnd += 1;
	} else if((bf->c4 = (state->cwnd * state->mss < TCP_SND_WND)))
	{
		in->RC.cwnd = state->cwnd;
		state->cwnd += 1 / state->cwnd;
	}
}

/*
 * Update the Round-Trip Timer
 */
void
tcp_update_rtt(tcp_state * state, tw_bf * bf, tcp_message * in, tw_lp * lp)
{
	if ((bf->c5 = (in->ack == state->rtt_seq && state->unack < state->len)))
	{
		if ((bf->c6 = (state->rtt_time != 0)))
		{
			in->dst = state->rto;
			state->rto = .875 * state->rto +
				(10 * (tw_now(lp) - state->rtt_time)) * .125;
		}

		in->RC.rtt_seq = state->rtt_seq;
		in->RC.rtt_time = state->rtt_time;

		state->rtt_time = tw_now(lp);
		state->rtt_seq = state->seq_num;
		state->rto_seq++;
	} else if((bf->c7 = (in->ack > state->rtt_seq && state->unack < state->len)))
	{
		in->RC.rtt_seq = state->rtt_seq;
		in->RC.rtt_time = state->rtt_time;

		state->rtt_time = tw_now(lp);
		state->rtt_seq = state->seq_num;
		state->rto_seq++;
	} else if(state->unack >= state->len)
	{
		if(!state->timer)
			return;

		bf->c15 = 1;
		state->stats->final_time = tw_now(lp);
		tcp_timer_cancel(state->timer, bf, in, lp);
		state->timer = NULL;

		return;
	}

	state->timer = tcp_timer_reset(state->timer, bf, in, 
					tw_now(lp) + state->rto, state->unack, lp);
}
