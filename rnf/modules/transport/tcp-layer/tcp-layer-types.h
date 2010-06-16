#ifndef INC_tcp_layer_types_h
#define INC_tcp_layer_types_h

#define FORWARD       1
#define RTO           2
#define STARTUP       3

#define TCP_LAYER_SND_WND  65536
#define TCP_LAYER_HEADER_SIZE 20.0
#define TCP_LAYER_TRANSFER_SIZE ((g_tcp_layer_mss + TCP_LAYER_HEADER_SIZE))

typedef struct tcp_layer_state tcp_layer_state;
typedef struct RC RC;
typedef struct tcp_layer_message tcp_layer_message;
typedef struct tcp_layer_statistics tcp_layer_statistics;

struct tcp_layer_state
{
	rn_machine	*host;

	int		bad_msgs;

	int             unack;
	int             seq_num;
	int             dup_count;
	int             len;
	int             timeout;
	int             rtt_seq;
	short           out_of_order[33];
	int             rto_seq;

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

	/*
	 * stats 
	 */
	int             received_packets;
	int             timedout_packets;
	int             sent_packets;
	double          start_time;
};

struct RC
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

struct tcp_layer_message
{
	int             MethodName;
	int             ack;
	int             seq_num;
	int             source;
	double          dest;
	tw_bf           CV;
	RC              RC;
};

struct tcp_layer_statistics
{
	int             sent_packets;
	int             timedout_packets;
	int             received_packets;
	int             dropped_packets;
	double          throughput;
};


#endif
