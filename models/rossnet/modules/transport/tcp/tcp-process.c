#include <tcp.h>

/*
 * Processes an incoming acknowledgement
 */
void
tcp_process_ack(tcp_state * state, tw_bf * bf, tcp_message * in, tw_lp * lp)
{
	// end case?  no file x-fer complete, still recbing bad acks?
	if(state->unack == 0 && in->ack > 0)
		return;

	if(state->seq_num < 0)
		tw_error(TW_LOC, "ere");

	if((bf->c2 = (state->unack <= in->ack)))
	{
#if TCP_DEBUG
		printf("\t%lld: ACK: sn %d unack %d ack %d sn %d (%lf)\n", 
			lp->gid, state->seq_num, state->unack, in->ack , in->seq_num, tw_now(lp));
#endif

		state->stats->ack_recv++;

		// printf(" should work %d %d %d\n",in->ack , in->seq_num, lp->gid);
		in->RC.seq_num = state->seq_num;
		if (in->ack >= state->seq_num)
			state->seq_num = in->ack + state->mss;

		in->RC.dup_count = state->dup_count;
		state->dup_count = 0;

		in->RC.unack = state->unack;
		state->unack = in->ack + state->mss;

		tcp_update_cwnd(state, bf, in, lp);
		tcp_update_rtt(state, bf, in, lp);

		/*
		 * Prints the time it took to transfer, the rate and congestion
		 * window size     
		 */
		if(state->len && state->unack >= state->len)
		{
#if 1
			double	 rate = ((state->len) /
					 (tw_now(lp) - state->stats->start_time)) /
					  1024;

			fprintf(stdout, "\t%-50s %11.4lf KB\n", "Xfer", 
					state->len / 1024.0);
			fprintf(stdout, "\t%-50s %11.4lf secs\n", "Time", 
					tw_now(lp) - state->stats->start_time);
			fprintf(stdout, "\t%-50s %11.4lf Kbps\n", "Rate", rate);
			fprintf(stdout, "\t%-50s %11.4lf \n", "Cwnd", state->cwnd);
			fprintf(stdout, "\t%-50s %11d %11d \n", "ACK", state->unack, state->len);
			fprintf(stdout, "\t%-50s %11.4lf \n", "Now", tw_now(lp));
#endif

			//state->len = 0;
#if TCP_DEBUG
	printf("\t%lld: file complete (unack %d, len %d)\n", lp->gid, state->unack, state->len);
#endif
			// send event up to app with throughput rate
			//rn_event_send(rn_event_new(lp->gid, 0.0, lp, UPSTREAM, state->len));
		}

		in->RC.lastsent = state->lastsent;
		in->RC.sent = 0;
		while (state->seq_num < state->len &&
			   (state->seq_num + state->mss) <=
			   (state->unack + (min(state->cwnd, state->recv_wnd) * state->mss)))
		{
			tcp_event_send(state, lp->gid, 0.0, state->connection, TCP_MTU, 
					state->seq_num, 0, lp);

			in->RC.sent++;
			state->stats->sent++;
			state->seq_num += state->mss;
		}

#if TCP_DEBUG
		printf("\t%lld: final st sn %d, unack %d ack %d\n",
			lp->gid, state->seq_num, state->unack, in->ack);
#endif
	} else
	{
		state->stats->ack_invalid++;

#if TCP_DEBUG
		printf("\t%lld: Invalid ACK: unack %d ack %d at %lf\n", lp->gid, state->unack, in->ack, tw_now(lp));
#endif

		if ((bf->c3 = (state->unack == in->ack)))
		{
			tw_error(TW_LOC, "how can we ever get here!?");

			state->dup_count += 1;
			if ((bf->c4 = (state->dup_count == 4)))
			{

				in->RC.ssthresh = state->ssthresh;
				state->ssthresh =
					(min(((int)state->cwnd + 1), state->recv_wnd) / 2) *
					state->mss;
				// CHANGED
				in->RC.cwnd = state->cwnd;
				state->cwnd = 1;

				in->RC.lastsent = state->lastsent;

				tcp_event_send(state, lp->gid, 0.0, state->connection, TCP_MTU, 
						state->unack, 0, lp);

				in->RC.seq_num = state->seq_num;
				state->seq_num = state->unack + state->mss;

				// CHANGE Maybe.
				state->rto_seq++;

				state->timer = tcp_timer_reset(state->timer, bf, in, 
						tw_now(lp) + state->rto, state->unack, lp);

				in->RC.rtt_seq = state->rtt_seq;
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
	int	i;

#if TCP_DEBUG
	printf("\t%lld: DATA: in sn %d, st sn %d mss %d\n", lp->gid, in->seq_num, state->seq_num, state->mss);
#endif

	state->stats->recv++;
	state->stats->ack_sent++;
	in->RC.seq_num = state->seq_num;

	// server sending new file..
	if(in->seq_num == 0)
	{
		bf->c16 = 1;
		state->seq_num = 0;

		in->RC.cwnd = state->recv_wnd;
		state->recv_wnd = g_tcp_rwd;

		for(i = 0; i < g_tcp_rwd; i++)
		{
			in->RC.out_of_order[i] = state->out_of_order[i];
			state->out_of_order[i] = 0;
		}

#if TCP_DEBUG
		printf("\t%lld: Client starting new file receive!\n", lp->gid);
#endif
	}

	for(i = 0; i < g_tcp_rwd; i++)
		in->RC.out_of_order[i] = state->out_of_order[i];

	if ((bf->c2 = (in->seq_num == state->seq_num)))
	{
#if TCP_DEBUG
		printf("\t%lld: c2 in sn %d, st sn %d, window %d, loc %d\n", 
			lp->gid, in->seq_num, state->seq_num,
			(state->seq_num + TCP_SND_WND - state->mss), 
			(in->seq_num / state->mss) % state->recv_wnd);
#endif
		in->RC.dup_count = 0;

		state->seq_num += state->mss;
		while(state->out_of_order[(state->seq_num / (int)state->mss) % 
				state->recv_wnd])
		{
			state->out_of_order[(state->seq_num / (int)state->mss) %
								state->recv_wnd] = 0;
			state->seq_num += state->mss;
			in->RC.dup_count++;
		}
	} else if ((bf->c3 = (in->seq_num > state->seq_num)))
	{
		if ((bf->c4 = (in->seq_num > 
				(state->seq_num + state->recv_wnd * state->mss
					/* TCP_SND_WND */  - state->mss))))
		{
			// look up need rc code CHANGE
			tw_error(TW_LOC,"recv_wnd buffer overflow %d %d\n",
					 in->seq_num, 
				 state->seq_num + TCP_SND_WND - state->mss);
		} else
		{
#if TCP_DEBUG
			printf("\t%lld: c3 in sn %d, sn %d, window %d, loc %d\n", 
				lp->gid, in->seq_num, state->seq_num,
				(state->seq_num + TCP_SND_WND - state->mss), 
				(in->seq_num / state->mss) % state->recv_wnd);
#endif

			state->out_of_order[(in->seq_num / state->mss) % 
						state->recv_wnd] = 1;
		}
	}

	in->RC.lastsent = state->lastsent;
	tcp_event_send(state, lp->gid, 0.0, in->src, TCP_HEADER_SIZE,
			0, state->seq_num - state->mss, lp);
}
