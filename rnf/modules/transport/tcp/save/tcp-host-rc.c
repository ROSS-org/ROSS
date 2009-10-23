#include "tcp.h"


/*********************************************************************
                Receives the processing of an ack   
*********************************************************************/

void
tcp_process_ack_rc(tcp_state * state, tw_bf * bf, tcp_message * M, tw_lp * lp)
{
	int             i;
	int             ack;

	// printf("%f roll ack %d %d lp %d\n",tw_now(lp), state->unack, M->ack,
	// lp->id);

	if (bf->c2)
	{
		for (i = 0; i < M->seq_num; i++)
		{
			// tw_rand_reverse_unif(lp->id);
			state->seq_num -= state->mss;
			state->sent_packets--;
		}
		state->lastsent = M->RC.lastsent;

		// reverse tcp_update_rtt 
		if (bf->c5 || bf->c7)
		{

			state->rtt_seq = M->RC.rtt_seq;
			// M->RC.rtt_seq = state->rtt_seq;
			// state->rtt_seq = M->ack; // wrong CHANGE
			state->rtt_time = M->RC.rtt_time;

			if (bf->c6 && bf->c5)
				state->rto = M->dst;
			state->rto_seq--;

			if (bf->c13)
			{
				tcp_message    *M1;
				rn_message     *msg1;

				if (state->timer)
					tw_timer_reset(lp, &(state->timer), M->RC.timer_ts);
				else
					state->timer = tw_timer_init(lp, M->RC.timer_ts);

				msg1 = tw_event_data(state->timer);
				M1 = rn_message_data(msg1);
				M1->seq_num = M->RC.timer_seq;
				M1->src = M->src;
				M1->type = RTO;
			}
		}


		// Reverse tcp_update_cwnd
		if (bf->c3 || bf->c4)
			state->cwnd = M->RC.cwnd;

		ack = state->unack;
		state->unack = M->ack;
		M->ack = ack - state->mss;
		state->dup_count = M->RC.dup_count;
		if (bf->c9)
			state->seq_num = M->RC.seq_num;
	} else
	{
		if (bf->c3)
		{
			state->dup_count -= 1;
			if (bf->c4)
			{

				state->rtt_time = M->RC.rtt_time;
				state->rtt_seq = M->RC.seq_num;	// state->unack; //big CHANGE 
				// state->timedout_packets--;
				// state->sent_packets--;

				state->rto_seq--;

				state->seq_num = M->ack;
				M->ack = state->unack - state->mss;

				state->lastsent = M->RC.lastsent;
				state->cwnd = M->RC.cwnd;
				state->ssthresh = M->dst;


				if (bf->c13)
				{
					tcp_message    *M1;

					if (state->timer)
						tw_timer_reset(lp, &(state->timer), M->RC.timer_ts);
					else
						state->timer = tw_timer_init(lp, M->RC.timer_ts);
					M1 = tw_event_data(state->timer);
					M1->seq_num = M->RC.timer_seq;
					M1->src = M->src;
					M1->type = RTO;
				}
			}
		}
	}
}


/*********************************************************************
               Receives the processing of data packet
*********************************************************************/

void
tcp_process_data_rc(tcp_state * state, tw_bf * bf, tcp_message * M, tw_lp * lp)
{
	// printf("%f \n", M->RC.lastsent);
	if (bf->c2)
	{
		// tw_rand_reverse_unif(lp->id);
		state->lastsent = M->RC.lastsent;
		state->received_packets--;
		state->seq_num -= state->mss;
		while (M->RC.dup_count)
		{
			state->out_of_order[(state->seq_num / (int)state->mss) %
								state->recv_wnd] = 1;
			state->seq_num -= state->mss;
			M->RC.dup_count--;
		}
	} else
	{
		state->lastsent = M->RC.lastsent;
		if (bf->c3)
		{
			if (bf->c4)
			{
			}	// need to think about this one WILL NOT WORK
				// UNTIL YOU FIX!!!!!
			else
				state->out_of_order[(M->seq_num / (int)state->mss) %
							state->recv_wnd] = 0;
		}
	}
}


/*********************************************************************
                 Receives the processing of a timeout    
*********************************************************************/

void
tcp_timeout_rc(tcp_state * state, tw_bf * bf, tcp_message * M, tw_lp * lp)
{
	int             ssthresh = M->dst;
	int             seq_num = M->ack;

	if (bf->c1)
	{
		state->lastsent = M->RC.lastsent;
		state->rtt_time = M->RC.rtt_time;
		state->rtt_seq = M->RC.rtt_seq;	// M->seq_num; CHANGE
		state->rto_seq--;
		state->timedout_packets--;
		// state->sent_packets--;


		state->rto = M->RC.dup_count;	
		// state->rto/2; //possible an error.
		state->seq_num = seq_num;

		state->cwnd = M->RC.cwnd;
		state->ssthresh = ssthresh;

		tw_timer_cancel(lp, &(state->timer));
		state->timer = M->RC.timer;
	}
}
