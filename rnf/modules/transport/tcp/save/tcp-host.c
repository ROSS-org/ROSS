#include <tcp.h>

#define TCP_DEBUG 0

/*********************************************************************
                Processes a host's incoming acks
*********************************************************************/

void
tcp_host_process_ack(tcp_state * state, tw_bf * bf, tcp_message * msg, tw_lp * lp)
{
	int             ack;
	tw_stime        ts = 0.0;

#if TCP_DEBUG
	printf(" should work %d %d %d\n",msg->ack , msg->seq_num, lp->id);
#endif

	if ((bf->c2 = (state->unack <= msg->ack)))
	{
		// printf(" should work %d %d %d\n",msg->ack , msg->seq_num, lp->id);
		if ((bf->c9 = (msg->ack >= state->seq_num)))
		{						// CHANGE
			msg->RC.seq_num = state->seq_num;
			// printf("new seq %d \n",state->seq_num);
			state->seq_num = msg->ack + state->mss;
		}

		msg->RC.dup_count = state->dup_count;
		state->dup_count = 0;

		ack = state->unack;
		state->unack = msg->ack + state->mss;
		tcp_update_cwnd(state, bf, msg, lp);

		/*
		 * Prints the time it took to transfer, the rate and congestion
		 * window size     
		 */

		if (state->unack >= state->len)
		{
			printf
				("\n\t\tTransfer Time = %f at %f kbps \n\t\tCWND = %f lp = %ld\n\n",
				 (tw_now(lp) - state->start_time),
				 ((state->len) / (tw_now(lp) - state->start_time)) / 1000,
				 state->cwnd, lp->id);
		}

		tcp_update_rtt(state, bf, msg, lp);

		msg->seq_num = 0;
		msg->RC.lastsent = state->lastsent;

		if (state->lastsent < tw_now(lp))
			state->lastsent = tw_now(lp);
		else
			ts = state->lastsent - tw_now(lp);

		ts += state->host->link->delay;
		while (state->seq_num < state->len &&
			   (state->seq_num + state->mss) <=
			   (state->unack + min(state->cwnd, state->recv_wnd) * state->mss))
		{
			msg->seq_num++;
			state->lastsent += TCP_TRANSFER_SIZE / state->host->link->bandwidth;
			ts += TCP_TRANSFER_SIZE / state->host->link->bandwidth;
			tcp_event_send(lp, FORWARD, lp->id, msg->src, 
					TCP_TRANSFER_SIZE, state->seq_num, 0, ts);

			state->sent_packets++;
			state->seq_num += state->mss;
			// if(state->seq_num < 0 )
			// tw_error(TW_LOC,"OverFlow\n");
		}
		msg->ack = ack;
	} else
	{
		// printf("Invalid Ack %d\n", msg->ack);
		if ((bf->c3 = ((state->unack - state->mss) == msg->ack)))
		{
			state->dup_count += 1;
			if ((bf->c4 = (state->dup_count == 4)))
			{

				msg->dst = state->ssthresh;
				state->ssthresh =
					(min(((int)state->cwnd + 1), state->recv_wnd) / 2) *
					state->mss;
				// CHANGED
				msg->RC.cwnd = state->cwnd;
				state->cwnd = 1;

				msg->RC.lastsent = state->lastsent;
				if (state->lastsent < tw_now(lp))
					state->lastsent = tw_now(lp);
				else
					ts = state->lastsent - tw_now(lp);

				state->lastsent +=
					TCP_TRANSFER_SIZE / state->host->link->bandwidth;

				ts += (TCP_TRANSFER_SIZE /
					   state->host->link->bandwidth + state->host->link->delay);
				tcp_event_send(lp, FORWARD, lp->id, msg->src, 
						TCP_TRANSFER_SIZE, state->unack, 
						0, ts);

				msg->ack = state->seq_num;
				state->seq_num = state->unack + state->mss;

				// CHANGE Maybe.
				state->rto_seq++;

				if ((bf->c13 = (state->timer != NULL)))
				{
					if(NULL != state->timer->memory)
						b = state->timer->memory;
					else
					{
						b = tw_memory_alloc(lp, g_tcp_fd);
						tw_event_memory_set(state->timer, b, g_tcp_fd);
					}

					m = tw_memory_data(b);

					msg->RC.timer_ts = state->timer->recv_ts;
					msg->RC.timer_seq = m->seq_num;

					rn_timer_reset(lp, &(state->timer),
							   tw_now(lp) + state->rto);

					if (state->timer)
					{
						m->seq_num = state->unack;
						tw_event_memory_set(state->timer,
									b, g_tcp_fd);
					}
				}

				// state-save variables into event
				msg->RC.seq_num = state->rtt_seq;
				state->rtt_seq = state->unack;

				msg->RC.rtt_time = state->rtt_time;
				state->rtt_time = 0;
			}
		}
	}
}

/*
 * tcp_process_data: Processes incoming data for a host
 */
void
tcp_host_process_data(tcp_state * state, tw_bf * bf, tcp_message * msg, tw_lp * lp)
{
	tw_stime        ts = 0.0;

	if ((bf->c2 = (msg->seq_num == state->seq_num)))
	{
		state->received_packets++;
		msg->RC.lastsent = state->lastsent;
		if (state->lastsent < tw_now(lp))
		{
			state->lastsent = tw_now(lp);
			ts = state->host->link->delay +
				TCP_HEADER_SIZE / state->host->link->bandwidth;
		} else
		{
			ts = (state->lastsent - tw_now(lp)) +
				TCP_HEADER_SIZE / state->host->link->bandwidth +
				state->host->link->delay;
			state->lastsent += TCP_TRANSFER_SIZE / state->host->link->bandwidth;
		}

		msg->RC.dup_count = 0;
		state->seq_num += state->mss;
		while(state->out_of_order[(state->seq_num / (int)state->mss) % state->recv_wnd])
		{
			msg->RC.dup_count++;
			state->out_of_order[(state->seq_num / (int)state->mss) %
								state->recv_wnd] = 0;
			state->seq_num += state->mss;
		}

		tcp_event_send(lp, FORWARD, lp->id, msg->src, TCP_TRANSFER_SIZE,
					   0, state->seq_num - state->mss, ts);
	} else
	{
		msg->RC.lastsent = state->lastsent;
		if (state->lastsent < tw_now(lp))
		{
			state->lastsent = tw_now(lp);
			ts = state->host->link->delay +
				TCP_HEADER_SIZE / state->host->link->bandwidth;
		} else
		{
			// printf("why 1 %f %f %d\n", state->lastsent, tw_now(lp),
			// state->seq_num);
			ts = (state->lastsent - tw_now(lp)) +
				TCP_HEADER_SIZE / state->host->link->bandwidth +
				state->host->link->delay;
			state->lastsent += TCP_HEADER_SIZE / state->host->link->bandwidth;
		}

		if ((bf->c3 = (msg->seq_num > state->seq_num)))
		{
			if ((bf->c4 =
				 (msg->seq_num > (state->seq_num + state->recv_wnd * state->mss
								/*
								 * TCP_SND_WND 
								 */  - state->mss))))
			{
				// look up need rc code CHANGE
				tw_error(TW_LOC, "revc_wnd buffer overflow %d %d\n",
						 msg->seq_num, state->seq_num + TCP_SND_WND - state->mss);
			} else
			{
				/*
				 * printf("msg %d window %d, loc %d\n", msg->seq_num,
				 * (state->seq_num + TCP_SND_WND - state->mss), (msg->seq_num /
				 * state->mss) % state->recv_wnd); 
				 */
				state->out_of_order[(msg->seq_num / state->mss) % state->recv_wnd] =
					1;
			}
		}

		tcp_event_send(lp, FORWARD, lp->id, msg->src, TCP_TRANSFER_SIZE,
					   0, state->seq_num - state->mss, ts);

	}

}


/*********************************************************************
                          Updates the cwnd
*********************************************************************/

void
tcp_host_update_cwnd(tcp_state * state, tw_bf * bf, tcp_message * msg, tw_lp * lp)
{
	if ((bf->c3 =
		 ((state->cwnd * state->mss) < state->ssthresh
		  && state->cwnd * state->mss < TCP_SND_WND)))
	{
		msg->RC.cwnd = state->cwnd;
		state->cwnd += 1;
	} else if ((bf->c4 = (state->cwnd * state->mss < TCP_SND_WND)))
	{
		msg->RC.cwnd = state->cwnd;
		state->cwnd += 1 / state->cwnd;
	}

}


/*********************************************************************
                          Updates the rtt
*********************************************************************/

void
tcp_host_update_rtt(tcp_state * state, tw_bf * bf, tcp_message * msg, tw_lp * lp)
{
	tw_memory	*b;

	tcp_message	*m;

	if ((bf->c5 = (msg->ack == state->rtt_seq && state->unack < state->len)))
	{
		if ((bf->c6 = (state->rtt_time != 0)))
		{
			msg->dst = state->rto;
			state->rto =
				.875 * state->rto +
				(10 * (tw_now(lp) - state->rtt_time)) * .125;

		}

		msg->RC.rtt_seq = state->rtt_seq;
		msg->RC.rtt_time = state->rtt_time;

		state->rtt_time = tw_now(lp);
		state->rtt_seq = state->seq_num;
		state->rto_seq++;

		if ((bf->c13 = (state->timer != NULL)))
		{
			if(NULL != state->timer->memory)
				b = state->timer->memory;
			else
			{
				b = tw_memory_alloc(lp, g_tcp_fd);
				tw_event_memory_set(state->timer, b, g_tcp_fd);
			}

			m = tw_memory_data(b);

			msg->RC.timer_ts = state->timer->recv_ts;
			msg->RC.timer_seq = m->seq_num;

			rn_timer_reset(lp, &(state->timer), tw_now(lp) + state->rto);

			if (state->timer)
				m->seq_num = state->unack;
		}

		// tcp_event_send(lp, RTO, msg->src, lp->id, state->seq_num,
		// state->rto_seq, state->rto);
	} else if((bf->c7 = (msg->ack > state->rtt_seq && state->unack < state->len)))
	{
		msg->RC.rtt_seq = state->rtt_seq;
		msg->RC.rtt_time = state->rtt_time;
		state->rtt_time = tw_now(lp);
		state->rtt_seq = state->seq_num;

		state->rto_seq++;

		if ((bf->c13 = (state->timer != NULL)))
		{
			if(NULL != state->timer->memory)
				b = state->timer->memory;
			else
			{
				b = tw_memory_alloc(lp, g_tcp_fd);
				tw_event_memory_set(state->timer, b, g_tcp_fd);
			}

			m = tw_memory_data(b);

			msg->RC.timer_ts = state->timer->recv_ts;
			msg->RC.timer_seq = m->seq_num;

			rn_timer_reset(lp, &(state->timer), tw_now(lp) + state->rto);

			if (state->timer)
				m->seq_num = state->unack;
		}

		// tcp_event_send(lp, RTO, msg->src, lp->id, state->seq_num,
		// state->rto_seq, state->rto);
	}
}


/*********************************************************************
 Check if the timeout is valid, if so it restransmits the lost packet 
*********************************************************************/

void
tcp_host_timeout(tcp_state * state, tw_bf * bf, tcp_message * msg, tw_lp * lp)
{
	tw_memory	*b;

	tcp_message	*m;

#if TCP_DEBUG
	printf("bw %d de %f last %f \n", state->host->link->bandwidth, 
		state->host->link->delay, state->lastsent);
#endif

	if ((bf->c1 = (msg->seq_num >= state->unack && state->unack < state->len)))
	{
#if TCP_DEBUG
		printf("should timeout %f %d %f\n", tw_now(lp), lp->id, 
			state->lastsent );
#endif

		msg->dst = state->ssthresh;
		state->ssthresh = (min(((int)state->cwnd + 1), state->recv_wnd) / 2) *
			state->mss;

		// CHANGED
		msg->RC.cwnd = state->cwnd;
		state->cwnd = 1;

		msg->RC.lastsent = state->lastsent;
		if (state->lastsent < tw_now(lp))
			state->lastsent = tw_now(lp);
		state->lastsent += TCP_TRANSFER_SIZE / state->host->link->bandwidth;

#if TCP_DEBUG
		printf("bw %d de %f last %f \n", state->host->link->bandwidth, 
			state->host->link->delay, state->lastsent);
#endif

		tcp_event_send(lp, FORWARD, lp->id, msg->src, TCP_TRANSFER_SIZE,
				state->unack, 0, 
				state->lastsent - tw_now(lp) + 
					state->host->link->delay);

		msg->ack = state->seq_num;
		state->seq_num = state->unack + state->mss;
		state->timedout_packets++;

		msg->RC.dup_count = state->rto;
		state->rto = state->rto * 2;	// PUT IN msgAX TImsgE CHANGE

		state->rto_seq++;

		msg->RC.timer = state->timer;
		state->timer = rn_timer_init(lp, tw_now(lp) + state->rto);

		if ((bf->c13 = (state->timer != NULL)))
		{
			if(NULL != state->timer->memory)
				b = state->timer->memory;
			else
			{
				b = tw_memory_alloc(lp, g_tcp_fd);
				tw_event_memory_set(state->timer, b, g_tcp_fd);
			}

			m = tw_memory_data(b);

			m->src = msg->src;
			m->seq_num = state->unack;
			m->type = RTO;

			tw_event_memory_set(state->timer, b, g_tcp_fd);
		}

		// state-save into event
		msg->RC.rtt_seq = state->rtt_seq;
		state->rtt_seq = state->unack;

		msg->RC.rtt_time = state->rtt_time;
		state->rtt_time = 0;
	}
}
