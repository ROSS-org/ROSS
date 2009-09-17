#include <tcp.h>

void
stop_transfer(tcp_state * state, tw_bf * bf, rn_message * m, tw_lp * lp)
{
#if TCP_DEBUG
	printf("%lld: STOP XFER: %d of %d bytes at %lf\n", 
		lp->gid, state->unack, state->len, tw_now(lp));
#endif

	//state->unack = 0;
	state->len = 0;

	if(state->timer)
	{
		tw_memory_free(lp, state->timer->memory, g_tcp_fd);
		state->timer->memory = NULL;

		rn_timer_cancel(lp, &state->timer);
		state->timer = NULL;
	}
}

void
start_transfer(tcp_state * state, tw_bf * bf, rn_message * m, tw_lp * lp)
{
	if (state->timer)
		tw_error(TW_LOC, "tcp start with timer set.");

	// file size
	state->len = m->size;
	state->unack = 0;

	// from init
	state->cwnd = 1;
	state->ssthresh = TCP_SND_WND;
	state->rto = 3.0;

#if TCP_DEBUG
	printf("%lld: START XFER: %d bytes at %lf to %lld\n", 
		lp->gid, state->len, tw_now(lp), state->connection);
#endif

	state->stats->sent++;
	state->seq_num = state->mss;
	state->rtt_seq = 0;
	state->rtt_time = tw_now(lp);

	m->dst = state->connection;
	tcp_event_send(state, lp->gid, 0.0, state->connection, TCP_MTU, 0, 0, lp);

	state->timer = rn_timer_init(lp, tw_now(lp) + state->rto);

	if(state->timer)
	{
		tw_memory	*b;
		tcp_message	*msg;

		b = tw_memory_alloc(lp, g_tcp_fd);
		msg = tw_memory_data(b);

		msg->src = m->src;
		msg->seq_num = 0;

		tw_event_memory_set(state->timer, b, g_tcp_fd);

#if TCP_DEBUG
		printf("%lld: RTO timer at %lf \n", lp->gid, state->timer->recv_ts);
#endif
	}
}

/*
 * tcp_init: per LP intiialization function
 */
void
tcp_init(tcp_state * state, tw_lp * lp)
{
	state->cwnd = 1;
	state->ssthresh = TCP_SND_WND;
	state->rto = 3.0;
	//state->host = rn_getmachine(lp->gid);
	state->connection = rn_getmachine(lp->gid)->conn;

	state->stats = tw_calloc(TW_LOC, "", sizeof(tcp_statistics), 1);
	state->mss = g_tcp_mss;
	state->recv_wnd = g_tcp_rwd;

	// faking out that this is not in transfer
	state->len = 1;
	state->unack = 0;

	g_state = state;

#if TCP_LOG
	state->log = stdout;
#endif

	if(-1 != state->connection)
	{
		rn_message m;
		m.size = 40960;
		start_transfer(state, NULL, &m, lp);
	}
}

/*
 * tcp_event_handler: per LP event handler
 */
void
tcp_event_handler(tcp_state * state, tw_bf * bf, rn_message * m, tw_lp * lp)
{
	tw_memory	*b;
	tcp_message	*in = NULL;

	*(int *) bf = 0;

	// if no membuf, then must be application layer
	if(NULL == (b = tw_event_memory_get(lp)))
		tw_error(TW_LOC, "%lld: no membuf on TCP event!", lp->gid);

	in = tw_memory_data(b);
	memset(&in->RC, 0, sizeof(RC));

	if(m->type == DOWNSTREAM)
	{
		if(m->size)
			start_transfer(state, bf, m, lp);
		else
			stop_transfer(state, bf, m, lp);
	} else if(m->type == TIMER)
	{
#if TCP_DEBUG
		printf("%lld: RTO at %.8lf (%ld)\n",
			lp->gid, tw_now(lp), (long int) lp->pe->cur_event);
#endif

		tcp_timeout(state, bf, in, lp);
	} else if(state->connection != -1)
	{
#if TCP_DEBUG
		printf("%lld: ACK from %lld at %.8lf \n", lp->gid, in->src, tw_now(lp));
#endif
		tcp_process_ack(state, bf, in, lp);
	} else
	{
#if TCP_DEBUG
		printf("%lld: DATA from %lld at %.8lf \n", lp->gid, in->src, tw_now(lp));
#endif
		tcp_process_data(state, bf, in, lp);
	}

	tw_memory_free(lp, b, g_tcp_fd);
}

/*
 * tcp_rc_event_handler: per LP reverse computation event handler
 */
void
tcp_rc_event_handler(tcp_state * state, tw_bf * bf, rn_message * m, tw_lp * lp)
{
	tw_memory	*b;
	tcp_message	*in;

	if(NULL == (b = tw_memory_free_rc(lp, g_tcp_fd)))
		tw_error(TW_LOC, "No membuf on event!");

	in = tw_memory_data(b);

	if(m->type == TIMER)
	{
#if TCP_DEBUG
		printf("%lld: RC RTO at %.8lf \n",
			lp->gid, tw_now(lp));
#endif
		tcp_rc_timeout(state, bf, in, lp);
	} else if(state->connection != -1)
	{
#if TCP_DEBUG
		printf("%lld: RC ACK from %lld at %.8lf \n",
			lp->gid, in->src, tw_now(lp));
#endif
		tcp_rc_process_ack(state, bf, in, lp);
	} else
	{
#if TCP_DEBUG
		printf("%lld: RC DATA from %lld at %.8lf \n",
			lp->gid, in->src, tw_now(lp));
#endif
		tcp_rc_process_data(state, bf, in, lp);
	}

	tw_event_memory_get_rc(lp, b, g_tcp_fd);
}

/*
 * tcp_final: per LP finalization function
 */
void
tcp_final(tcp_state * state, tw_lp * lp)
{
#if TCP_LOG
	char	log[255];

	if(state->log && state->log != stdout)
		fclose(state->log);

	if(state->connection > -1)
		sprintf(log, "%s/tcp-host-%ld.log", g_rn_logs_dir, lp->gid);
	else
		sprintf(log, "%s/tcp-server-%ld.log", g_rn_logs_dir, lp->gid);

	state->log = fopen(log, "w");

	if(!state->log)
		return;

	if(state->connection > -1)
	{
		fprintf(state->log, "TCP Svr %ld: Conn Duration: %lf-%lf \n\n", 
			lp->gid, state->stats->start_time, state->stats->final_time);

 		g_tcp_stats->throughput +=
			(((state->len * 8.0)/ 
				(tw_now(lp) - state->stats->start_time)) / 1024);

		fprintf(state->log, "\t%-50s %11d\n", "Server", rn_getmachine(lp)->link[0].addr);
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
			lp->gid, state->stats->start_time, state->stats->final_time);
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

	if(state->connection != -1)
	{
 		g_tcp_stats->throughput +=
			(((state->len * 8.0)/ 
				(tw_now(lp) - state->stats->start_time)) / 1024);
	}

	g_tcp_stats->sent += state->stats->sent;
	g_tcp_stats->recv += state->stats->recv;
	g_tcp_stats->tout += state->stats->tout;
	g_tcp_stats->ack_sent += state->stats->ack_sent;
	g_tcp_stats->ack_recv += state->stats->ack_recv;
	g_tcp_stats->ack_invalid += state->stats->ack_invalid;
	g_tcp_stats->bad_msgs += state->stats->bad_msgs;
	g_tcp_stats->dropped_packets += state->stats->dropped_packets;
}
