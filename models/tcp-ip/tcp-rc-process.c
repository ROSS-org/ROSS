#include "tcp.h"

/*********************************************************************
                Receives the processing of an ack   
*********************************************************************/
void
tcp_rc_process_ack(tcp_state * state, tw_bf * bf, tcp_message * in, tw_lp * lp)
{
	int             i;
	int             ack;

	if (bf->c2)
	{
		state->stats->ack_recv--;
		for (i = 0; i < in->seq_num; i++)
		{
			// tw_rand_reverse_unif(lp->id);
			state->seq_num -= state->mss;
			state->stats->sent--;
		}

		state->lastsent = in->RC.lastsent;

		// reverse tcp_update_rtt 
		if (bf->c5 || bf->c7)
		{
			state->rtt_seq = in->RC.rtt_seq;
			// in->RC.rtt_seq = state->rtt_seq;
			// state->rtt_seq = in->ack; // wrong CHANGE
			state->rtt_time = in->RC.rtt_time;

			if (bf->c6 && bf->c5)
				state->rto = in->dst;

			state->rto_seq--;

			state->timer = tcp_rc_timer_reset(state->timer, bf, in, lp);
		}

		// Reverse tcp_update_cwnd
		if (bf->c3 || bf->c4)
			state->cwnd = in->RC.cwnd;

		if(bf->c15)
			state->timer = tcp_rc_timer_cancel(in, lp);

		ack = state->unack;
		state->unack = in->ack;
		in->ack = ack - state->mss;
		state->dup_count = in->RC.dup_count;

		if (bf->c9)
			state->seq_num = in->RC.seq_num;
	} else
	{
#if TCP_DEBUG
		printf("\t%ld: INVALID ACK: %d \n", lp->id, in->ack);
#endif

		state->stats->ack_invalid--;

		if (bf->c3)
		{
			state->dup_count -= 1;
			if (bf->c4)
			{

				state->rtt_time = in->RC.rtt_time;
				state->rtt_seq = in->RC.seq_num;	// state->unack; //big CHANGE 
				// state->tout--;
				// state->sent--;

				state->rto_seq--;

				state->seq_num = in->ack;
				in->ack = state->unack - state->mss;

				state->lastsent = in->RC.lastsent;
				state->cwnd = in->RC.cwnd;
				state->ssthresh = in->dst;

				state->timer = tcp_rc_timer_reset(state->timer, bf, in, lp);
			}
		}
	}
}


/*********************************************************************
               Receives the processing of data packet
*********************************************************************/

void
tcp_rc_process_data(tcp_state * state, tw_bf * bf, tcp_message * in, tw_lp * lp)
{
	// printf("%f \n", in->RC.lastsent);
	state->stats->ack_sent--;
	state->stats->recv--;
	state->lastsent = in->RC.lastsent;

	if (bf->c2)
	{
		// tw_rand_reverse_unif(lp->id);
		state->seq_num -= state->mss;

		while (in->RC.dup_count)
		{
			state->out_of_order[(state->seq_num / (int)state->mss) %
								state->recv_wnd] = 1;
			state->seq_num -= state->mss;
			in->RC.dup_count--;
		}
	} else
	{
		if (bf->c3)
		{
			if (bf->c4)
			{
				// need to think about this one WILL NOT WORK
				// UNTIL YOU FIX!!!!!
				tw_error(TW_LOC, "broken here");
			} else
				state->out_of_order[(in->seq_num / state->mss) %
							state->recv_wnd] = 0;
		}
	}
}
