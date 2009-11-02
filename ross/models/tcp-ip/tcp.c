#include <tcp.h>

#define TCP_LOG 0

/*
 * tcp_init: per LP intiialization function
 */
void
tcp_lp_init(tcp_state * state, tw_lp * lp)
{
	tw_stime        ts;

	tcp_message	*m;

	state->cwnd = 1;
	state->ssthresh = TCP_SND_WND;
	state->rto = 300.0;

	// must point to this TCP lps IP layer LP, read from file
	//state->connection = state->host->conn;
	state->stats = tw_vector_create(sizeof(tcp_statistics), 1);

#if TCP_LOG
	state->log = stdout;
#endif

	// create init event in absence of application
	//if (state->connection > -1)
	{
		state->len = 419600;

#if TCP_DEBUG
		printf("%ld: INITIAL SEND: %d bytes\n", lp->id, state->len);
#endif

		ts = tw_rand_exponential(lp->id, 20);
		state->stats->start_time = ts;
		state->stats->sent++;
		state->seq_num = state->mss;
		state->rtt_seq = 0;
		state->rtt_time = ts;
		state->lastsent = -1;

		tcp_event_send(state, lp->id, ts, state->connection, TCP_MTU, 0, 0, lp);

		state->timer = tw_timer_init(lp, ts + state->rto);

		if(state->timer)
		{
			m = tw_event_data(state->timer);

			m->src = lp->id;
			m->seq_num = 0;

#if TCP_DEBUG
			printf("%ld: RTO timer at %lf \n", lp->id, state->timer->recv_ts);
#endif
		}
	}
}

/*
 * tcp_event_handler: per LP event handler
 */
void
tcp_event_handler(tcp_state * state, tw_bf * bf, tcp_message * m, tw_lp * lp)
{
	*(int *) bf = 0;

	if(m->src == lp->id)
	{
#if TCP_DEBUG
		printf("%ld: RTO for %d at %lf (%ld)\n",
			lp->id, m->src, tw_now(lp), (long int) lp->cur_event);
#endif

		tcp_timeout(state, bf, m, lp);
	} else if(state->connection == NULL)
	{
#if TCP_DEBUG
		printf("%ld: DATA from %d at %lf \n", lp->id, m->src, tw_now(lp));
#endif
		tcp_process_data(state, bf, m, lp);
	} else
	{
#if TCP_DEBUG
		printf("%ld: ACK from %d at %lf \n", lp->id, m->src, tw_now(lp));
#endif
		tcp_process_ack(state, bf, m, lp);
	}
}

/*
 * tcp_rc_event_handler: per LP reverse computation event handler
 */
void
tcp_rc_event_handler(tcp_state * state, tw_bf * bf, tcp_message * m, tw_lp * lp)
{
	if(m->src == lp->id)
	{
#if TCP_DEBUG
		printf("%ld: RC RTO for %d at %lf (%ld)\n",
			lp->id, m->src, tw_now(lp), (long int) lp->cur_event);
#endif
		tcp_rc_timeout(state, bf, m, lp);
	} else if(state->connection == NULL)
	{
#if TCP_DEBUG
		printf("%ld: RC DATA from %d at %lf \n",
			lp->id, m->src, tw_now(lp));
#endif
		tcp_rc_process_data(state, bf, m, lp);
	} else
	{
#if TCP_DEBUG
		printf("%ld: RC ACK from %d at %lf \n",
			lp->id, m->src, tw_now(lp));
#endif
		tcp_rc_process_ack(state, bf, m, lp);
	}
}

/*
 * tcp_final: per LP finalization function
 */
void
tcp_lp_final(tcp_state * state, tw_lp * lp)
{
#if TCP_LOG
	char	log[255];
#endif

	g_tcp_stats->sent += state->stats->sent;
	g_tcp_stats->recv += state->stats->recv;
	g_tcp_stats->tout += state->stats->tout;
	g_tcp_stats->ack_sent += state->stats->ack_sent;
	g_tcp_stats->ack_recv += state->stats->ack_recv;
	g_tcp_stats->ack_invalid += state->stats->ack_invalid;
	g_tcp_stats->bad_msgs += state->stats->bad_msgs;
	g_tcp_stats->dropped_packets += state->stats->dropped_packets;

	if(lp->id % 10000 == 0)
		printf("Finalizing TCP LP %ld \n", lp->id);

#if TCP_LOG
	if(state->log && state->log != stdout)
		fclose(state->log);

	if(state->connection > -1)
		sprintf(log, "%s/tcp-server-%ld.log", g_rn_logs_dir, lp->id);
	else
		sprintf(log, "%s/tcp-host-%ld.log", g_rn_logs_dir, lp->id);

	state->log = fopen(log, "w");

	if(!state->log)
		return;

	if(state->connection > -1)
	{
		fprintf(state->log, "TCP Svr %ld: Conn Duration: %lf-%lf \n\n", 
			lp->id, state->stats->start_time, state->stats->final_time);

 		g_tcp_stats->throughput +=
			(((state->len * 8.0)/ 
				(tw_now(lp) - state->stats->start_time)) / 1024);

		fprintf(state->log, "\t%-50s %11d\n", "Server", state->host->link[0].addr);
		fprintf(state->log, "\t%-50s %11d Bytes\n", "Length", state->len);
		fprintf(state->log, "\t%-50s %11.4lf Kbps\n", "Throughput", 
			   (((state->len * 8) /
				 (tw_now(lp) - state->stats->start_time)) / 1024));

		fprintf(state->log, "\t%-50s %11d\n", "Timeouts", state->stats->tout);
		fprintf(state->log, "\t%-50s %11.4lf\n", "Cwnd", state->cwnd);
		fprintf(state->log, "\n");
	} else
	{
		fprintf(state->log, "TCP Clt %ld: Conn Duration: %lf-%lf \n\n", 
			lp->id, state->stats->start_time, state->stats->final_time);
	}

	fprintf(state->log, "\t%-50s %11d\n", "Sent Pkts", state->stats->sent);
	fprintf(state->log, "\t%-50s %11d\n", "Recv Pkts", state->stats->recv);
	fprintf(state->log, "\t%-50s %11d\n", "Drop Pkts", state->stats->dropped_packets);
	fprintf(state->log, "\t%-50s %11d\n", "Bad Pkts", state->stats->bad_msgs);
	fprintf(state->log, "\n");
	fprintf(state->log, "\t%-50s %11d\n", "ACK Sent", state->stats->ack_sent);
	fprintf(state->log, "\t%-50s %11d\n", "ACK Recv", state->stats->ack_recv);
	fprintf(state->log, "\t%-50s %11d\n", "ACK Invalid", state->stats->ack_invalid);
#endif
}
