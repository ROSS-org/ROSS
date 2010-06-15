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
	short           out_of_order[33];
	int             rto_seq;
	int		mss;
	int		recv_wnd;

	int             connection;

	tw_event        *timer;

#ifdef CLIENT_DROP
	int             count;
#endif

	double          lastsent;
	double          cwnd;
	double          ssthresh;
	double          rtt_time;
	double          rto;

	// just pass this back up stack when connection completes
	//void		*data;

	tcp_statistics	*stats;

#if TCP_LOG
	FILE		*log;
#endif
};

DEF(struct, RC)
{
	double          dup_count;
	double          cwnd;
	double          rtt_time;
	double          lastsent;

	tw_event        *timer;
	tw_stime        timer_ts;
	int             timer_seq;

	int             rtt_seq;

	int             seq_num;
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
	//tw_bf           bf;

	double          dst;
	RC              RC;
};

struct tcp_statistics_t
{
	int		bad_msgs;
	int             sent;
	int             tout;
	int             recv;
	int             dropped_packets;

	//double          throughput;
	//double          start_time;
	//double          final_time;

	int		ack_invalid;
	int		ack_sent;
	int		ack_recv;
};

#endif
