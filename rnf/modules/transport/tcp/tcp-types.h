#ifndef INC_tcp_types_h
#define INC_tcp_types_h

#define	TW_TCP_HOST   1

#define TCP_SND_WND  65536
//#define TCP_SND_WND  66048
#define TCP_HEADER_SIZE 40.0
//#define TCP_HEADER_SIZE 36.0
#define TCP_MTU (state->mss + TCP_HEADER_SIZE)

struct tcp_state_t;
typedef tcp_state_t tcp_state;
FWD(struct, RC);
struct tcp_message_t;
typedef tcp_message_t tcp_message;
enum tcp_event_t_t;
typedef tcp_event_t_t tcp_event_t;
struct tcp_statistics_t;
typedef tcp_statistics_t tcp_statistics;

struct tcp_state_t
{
	//rn_machine	*host;

	int             unack;
	int             seq_num;
	int             dup_count;
	int             len;
	int             timeout;
	int             rtt_seq;
	char 		out_of_order[33];
	int             rto_seq;
	int		mss;
	int		recv_wnd;

	tw_lpid		connection;

	tw_event        *timer;

#ifdef CLIENT_DROP
	int             count;
#endif

	tw_stime	lastsent;
	tw_stime	cwnd;
	tw_stime	ssthresh;
	tw_stime	rtt_time;
	tw_stime	rto;

	tcp_statistics	*stats;

#if TCP_LOG
	FILE		*log;
#endif
};

DEF(struct, RC)
{
	tw_stime	dup_count;
	tw_stime	cwnd;
	tw_stime	rtt_time;
	tw_stime	lastsent;
	tw_stime	rto;
	tw_stime	ssthresh;

	char		out_of_order[33];

	tw_event        *timer;
	tw_stime        timer_ts;
	int             timer_seq;

	int             rtt_seq;
	int             seq_num;
	int		unack;
	int		sent;
};

enum tcp_event_t_t
{
	TCP_CLIENT = 1,
	TCP_SERVER,
	TCP_CONNECT
};

struct tcp_message_t
{
	//tcp_event_t	type;
	int             ack;
	int             seq_num;
	tw_lpid		src;

	RC              RC;
};

struct tcp_statistics_t
{
	tw_stat		bad_msgs;
	tw_stat		sent;
	tw_stat		tout;
	tw_stat		recv;
	tw_stat		dropped_packets;

	tw_stat		ack_invalid;
	tw_stat		ack_sent;
	tw_stat		ack_recv;

	tw_stime	throughput;
	tw_stime	start_time;
	tw_stime	final_time;
};

#endif
