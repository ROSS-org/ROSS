#include "tcp.h"

/*********************************************************************
                 Receives the processing of a timeout    
*********************************************************************/
void
tcp_rc_timeout(tcp_state * state, tw_bf * bf, tcp_message * in, tw_lp * lp)
{
	if(!bf->c1)
		return;

	state->rtt_time = in->RC.rtt_time;
	state->rtt_seq = in->RC.rtt_seq;	

	state->timer = tcp_rc_timer_reset(state->timer, bf, in, lp);

	state->rto_seq--;
	state->rto = in->RC.dup_count;	
	state->stats->sent--;
	state->stats->tout--;
	state->seq_num = in->RC.seq_num;

	rn_reverse_event_send(state->connection, lp, DOWNSTREAM, TCP_MTU * 8.0);

	state->lastsent = in->RC.lastsent;
	state->cwnd = in->RC.cwnd;
	state->ssthresh = in->RC.ssthresh;

}
