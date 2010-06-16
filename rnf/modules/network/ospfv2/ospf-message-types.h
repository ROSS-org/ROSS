#ifndef INC_ospf_message_types_h
#define INC_ospf_message_types_h

enum ospf_message_type
{
	OSPF_IP = 1,
	OSPF_HELLO_MSG,
	OSPF_HELLO_SEND,
	OSPF_HELLO_TIMEOUT,
	OSPF_LINK_DOWN,
	OSPF_DD_MSG,
	OSPF_RETRANS_TIMEOUT,
	OSPF_LSA_TIMER,
	OSPF_LS_REQUEST,
	OSPF_LS_UPDATE, 	// == 10
	OSPF_LS_ACK,		// == 11
	OSPF_FLOOD_TIMEOUT,
	OSPF_ACK_TIMEOUT,
	OSPF_AGING_TIMER,	// = 14
	OSPF_RT_TIMER,
	OSPF_FORWARD,
	OSPF_RTO,
	OSPF_WEIGHT_CHANGE
};

/*
 * ROSS.Net does not yet provide me with a bitfield for reverse computation, so I must
 * provide my own!
 */
struct ospf_message
{
	ospf_message_type	type;

	int 	 area;
	void	*data;
};

struct ospf_hello
{
	//unsigned short netmask;
	tw_stime poll_interval;
	tw_stime hello_interval;
	unsigned int priority;

	unsigned int designated_r;
	unsigned int b_designated_r;

	unsigned int offset;
	unsigned int end;

	// keep this at bottom of struct for alloc
	unsigned int	*neighbors;
};

/*
 * These are all protocol messages
 *
 * IF THE SIZE OF A DD PKT GOES BEYOND SIZEOF LSA_LINK THEN PROBLEM!
 */
struct ospf_dd_pkt
{
	struct
	{
		unsigned char init:1;
		unsigned char more:1;	
		unsigned char master:1;
		unsigned char _pad:1;
	} b;

	signed int	 seqnum;
	unsigned char	 nlsas;
};

#endif
