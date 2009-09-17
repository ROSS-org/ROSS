#include <tcp.h>

void
tcp_event_send(tcp_state * state, tw_lpid src, tw_stime ts, tw_lpid dst, int size,
		int seq_num, int ack, tw_lp * lp)
{
	tw_event	*e;
	tw_memory	*b;

	tcp_message	*m;

	b = tw_memory_alloc(lp, g_tcp_fd);

#if TCP_DEBUG
if(state->connection != -1)
{
	printf("\t\t%lld %lld: SEND DATA at %lf (%d) to %lld \n", 
		lp->gid, src, tw_now(lp) + ts, seq_num, dst);
} else
{
	printf("\t\t%lld: SEND ACK at %lf (%d) to %lld \n", 
		lp->gid, tw_now(lp) + ts, ack, dst);
}
#endif

	e = rn_event_new(dst, ts, lp, DOWNSTREAM, 0);
	m = tw_memory_data(b);

	m->src = src;
	m->ack = ack;
	m->seq_num = seq_num;

	tw_event_memory_set(e, b, g_tcp_fd);
	rn_message_setsize(tw_event_data(e), 8.0 * size);

	rn_event_send(e);
}
