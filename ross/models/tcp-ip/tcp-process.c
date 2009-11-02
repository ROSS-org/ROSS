#include <tcp.h>

/*
 * Processes an incoming acknowledgement
 */
void
tcp_process_ack(tcp_state * state, tw_bf * bf, tcp_message * in, tw_lp * lp)
{
	int		 ack;

	if ((bf->c2 = (state->unack <= in->ack)))
	{
#if TCP_DEBUG
		printf("\t%ld: ACK: unack %d ack %d sn %d\n", 
			lp->id, state->unack, in->ack , in->seq_num);
#endif

		state->stats->ack_recv++;

		// printf(" should work %d %d %d\n",in->ack , in->seq_num, lp->id);
		if ((bf->c9 = (in->ack >= state->seq_num)))
		{	// CHANGE
			in->RC.seq_num = state->seq_num;
			// printf("new seq %d \n",state->seq_num);
			state->seq_num = in->ack + state->mss;
		}

		in->RC.dup_count = state->dup_count;
		state->dup_count = 0;

		ack = state->unack;
		state->unack = in->ack + state->mss;
		tcp_update_cwnd(state, bf, in, lp);

		tcp_update_rtt(state, bf, in, lp);

		/*
		 * Prints the time it took to transfer, the rate and congestion
		 * window size     
		 */
#if TCP_DEBUG
		if(state->unack >= state->len)
		{
			fprintf(state->log, "\t%-50s %11.4lf KB\n", "Xfer", 
				state->len / 1024.0);
			fprintf(state->log, "\t%-50s %11.4lf secs\n", "Time", 
					tw_now(lp) - state->stats->start_time);
			fprintf(state->log, "\t%-50s %11.4lf Kbps\n", "Rate", 
				 ((state->len) / 
					(tw_now(lp) - state->stats->start_time)) / 
				1024);
			fprintf(state->log, "\t%-50s %11.4lf \n", "Cwnd", state->cwnd);
		}
#endif

		in->seq_num = 0;
		in->RC.lastsent = state->lastsent;

		while (state->seq_num < state->len &&
			   (state->seq_num + state->mss) <=
			   (state->unack + min(state->cwnd, state->recv_wnd) * state->mss))
		{
			in->seq_num++;
			tcp_event_send(state, lp->id, 0.0, in->src, TCP_MTU, 
					state->seq_num, 0, lp);

			state->stats->sent++;
			state->seq_num += state->mss;
		}

		in->ack = ack;
	} else
	{
		state->stats->ack_invalid++;

#if TCP_DEBUG
		printf("\t%ld: Invalid ACK: unack %d ack %d at %lf\n", lp->id, state->unack, in->ack, tw_now(lp));
#endif

		if ((bf->c3 = ((state->unack - state->mss) == in->ack)))
		{
			state->dup_count += 1;
			if ((bf->c4 = (state->dup_count == 4)))
			{

				in->dst = state->ssthresh;
				state->ssthresh =
					(min(((int)state->cwnd + 1), state->recv_wnd) / 2) *
					state->mss;
				// CHANGED
				in->RC.cwnd = state->cwnd;
				state->cwnd = 1;

				in->RC.lastsent = state->lastsent;

				tcp_event_send(state, lp->id, 0.0, in->src, TCP_MTU, 
						state->unack, 0, lp);

				in->ack = state->seq_num;
				state->seq_num = state->unack + state->mss;

				// CHANGE Maybe.
				state->rto_seq++;

				state->timer = tcp_timer_reset(state->timer, bf, in, 
						tw_now(lp) + state->rto, state->unack, lp);

				// state-save variables into event
				in->RC.seq_num = state->rtt_seq;
				state->rtt_seq = state->unack;

				in->RC.rtt_time = state->rtt_time;
				state->rtt_time = 0;
			}
		}
	}
}

/*
 * tcp_process_data: Processes incoming data for a host
 */
void
tcp_process_data(tcp_state * state, tw_bf * bf, tcp_message * in, tw_lp * lp)
{
#if TCP_DEBUG && 0
	printf("\t%ld: DATA: sn %d recv %d\n", 
		lp->id, in->seq_num, state->stats->recv);
#endif

	state->stats->recv++;
	state->stats->ack_sent++;

	if ((bf->c2 = (in->seq_num == state->seq_num)))
	{
		in->RC.dup_count = 0;
		state->seq_num += state->mss;
		while(state->out_of_order[(state->seq_num / (int)state->mss) % 
				state->recv_wnd])
		{
			in->RC.dup_count++;
			state->out_of_order[(state->seq_num / (int)state->mss) %
								state->recv_wnd] = 0;
			state->seq_num += state->mss;
		}
	} else if ((bf->c3 = (in->seq_num > state->seq_num)))
	{
		if ((bf->c4 = (in->seq_num > 
				(state->seq_num + state->recv_wnd * state->mss
					/* TCP_SND_WND */  - state->mss))))
		{
			// look up need rc code CHANGE
			tw_error(TW_LOC, "recv_wnd buffer overflow %d %d\n",
					 in->seq_num, 
				 state->seq_num + TCP_SND_WND - state->mss);
		} else
		{
#if TCP_DEBUG
			printf("\tin %d window %d, loc %d\n", in->seq_num,
				(state->seq_num + TCP_SND_WND - state->mss), 
				(in->seq_num / state->mss) % state->recv_wnd);
#endif

			state->out_of_order[(in->seq_num / state->mss) % 
						state->recv_wnd] = 1;
		}
	}

	in->RC.lastsent = state->lastsent;
	tcp_event_send(state, lp->id, 0.0, in->src, TCP_HEADER_SIZE,
			0, state->seq_num - state->mss, lp);
}
