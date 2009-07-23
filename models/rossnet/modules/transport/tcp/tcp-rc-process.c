#include "tcp.h"

/*********************************************************************
                Receives the processing of an ack   
*********************************************************************/
void
tcp_rc_process_ack(tcp_state * state, tw_bf * bf, tcp_message * in, tw_lp * lp)
{
	if (bf->c2)
	{
		state->stats->ack_recv--;
		state->seq_num = in->RC.seq_num;

		while(in->RC.sent--)
		{
			state->stats->sent--;

			rn_reverse_event_send(state->connection, lp, 
						DOWNSTREAM, TCP_MTU * 8.0);
		}

		state->lastsent = in->RC.lastsent;

		// reverse tcp_update_rtt 
		if (bf->c5 || bf->c7)
		{
			state->rtt_seq = in->RC.rtt_seq;
			state->rtt_time = in->RC.rtt_time;

			// in->RC.rtt_seq = state->rtt_seq;
			// state->rtt_seq = in->ack; // wrong CHANGE

			if (bf->c6 && bf->c5)
				state->rto = in->RC.rto;

			state->rto_seq--;

		}

		if(bf->c16)
			state->timer = tcp_rc_timer_cancel(in, lp);
		else if(!bf->c17)
			state->timer = tcp_rc_timer_reset(state->timer, bf, in, lp);

		// Reverse tcp_update_cwnd
		if (bf->c3 || bf->c4)
			state->cwnd = in->RC.cwnd;

#if 0
		if(bf->c16)
			state->timer = tcp_rc_timer_cancel(in, lp);
#endif

		state->unack = in->RC.unack;
		state->dup_count = in->RC.dup_count;

#if TCP_DEBUG
		printf("\t%lld: RC ACK: sn %d unack %d ack %d \n", 
			lp->gid, state->seq_num, state->unack, in->ack);
#endif
	} else
	{
		state->stats->ack_invalid--;

		if (bf->c3)
		{
			tw_error(TW_LOC, "how can we ever get here!?");

			state->dup_count -= 1;
			if (bf->c4)
			{
				state->rto_seq--;

				state->rtt_time = in->RC.rtt_time;
				state->rtt_seq = in->RC.seq_num;
				state->seq_num = in->RC.seq_num;
				state->lastsent = in->RC.lastsent;
				state->cwnd = in->RC.cwnd;
				state->ssthresh = in->RC.ssthresh;

				rn_reverse_event_send(state->connection, lp, 
							DOWNSTREAM, TCP_MTU * 8.0);
				state->timer = tcp_rc_timer_reset(state->timer, bf, in, lp);
			}
		}

#if TCP_DEBUG
		printf("\t%lld: RC INVALID ACK: unack %d ack %d at %lf\n", 
			lp->gid, state->unack, in->ack, tw_now(lp));
#endif
	}
}


/*********************************************************************
               Receives the processing of data packet
*********************************************************************/

void
tcp_rc_process_data(tcp_state * state, tw_bf * bf, tcp_message * in, tw_lp * lp)
{
	int	 i;

	state->stats->ack_sent--;
	state->stats->recv--;
	state->lastsent = in->RC.lastsent;

	for(i = 0; i < g_tcp_rwd; i++)
		state->out_of_order[i] = in->RC.out_of_order[i];

	if(in->seq_num == 0)
		state->recv_wnd = in->RC.cwnd;

	state->seq_num = in->RC.seq_num;

#if TCP_DEBUG
	printf("\t%lld: RC c2 in sn %d, st sn %d, window %d, loc %d\n", 
		lp->gid, in->seq_num, state->seq_num,
		(state->seq_num + TCP_SND_WND - state->mss), 
		(in->seq_num / state->mss) % state->recv_wnd);
#endif

	rn_reverse_event_send(in->src, lp, DOWNSTREAM, TCP_HEADER_SIZE * 8.0);
}
