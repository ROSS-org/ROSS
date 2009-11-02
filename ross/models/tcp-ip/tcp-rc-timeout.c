#include "tcp.h"

/*********************************************************************
                 Receives the processing of a timeout    
*********************************************************************/
void
tcp_rc_timeout(tcp_state * state, tw_bf * bf, tcp_message * in, tw_lp * lp)
{
	int             ssthresh = in->dst;
	int             seq_num = in->ack;

	if(bf->c1 == 0)
		return;

	state->rtt_time = in->RC.rtt_time;
	state->rtt_seq = in->RC.rtt_seq;	
	state->rto_seq--;
	state->rto = in->RC.dup_count;	
	state->stats->tout--;
	state->seq_num = seq_num;
	state->lastsent = in->RC.lastsent;
	state->cwnd = in->RC.cwnd;
	state->ssthresh = ssthresh;

	state->timer = tcp_rc_timer_reset(state->timer, bf, in, lp);
}
