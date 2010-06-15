/*********************************************************************
		              tcp-types.h
*********************************************************************/

#ifndef TCP_TYPES_H
#define TCP_TYPES_H

#include "tcp.h"

typedef int MethodName_t;

//#define LOGGING 
//#define CLIENT_DROP

#define	TW_TCP_HOST   1
#define TW_TCP_ROUTER 2

#define FORWARD       0
#define RECEIVED      1
#define RTO           2

#define CORE_ROUTERS  3187
#define TCP_SND_WND  65536
#define TCP_HEADER_SIZE 40.0
#define TCP_TRANSFER_SIZE (g_mss + TCP_HEADER_SIZE)


struct Routing_Table_tag;
typedef struct Routing_Table_tag Routing_Table;
struct Router_Link_tag;
typedef struct Router_Link_tag Router_Link;
struct Host_Link_tag;
typedef struct Host_Link_tag Host_Link;
struct Host_Info_tag;
typedef struct Host_Info_tag Host_Info;
struct Router_State_tag;
typedef struct Router_State_tag Router_State;
struct Host_State_tag;
typedef struct Host_State_tag Host_State;
struct RC_tag;
typedef struct RC_tag RC;
struct Msg_Data_tag;
typedef struct Msg_Data_tag Msg_Data;
struct tcpStatistics_tag;
typedef struct tcpStatistics_tag tcpStatistics;
struct rocket_fuel_link_tag;
typedef struct rocket_fuel_link_tag rocket_fuel_link;
struct rocket_fuel_node_tag;
typedef struct rocket_fuel_node_tag rocket_fuel_node;

struct rocket_fuel_link_tag
{
  int node_id;
  int bandwidth;
};

struct rocket_fuel_node_tag
{
  int used;
  int level; int is_bb; int num_in_level;
  int num_links;
  int kp;
  int pe;
  rocket_fuel_link link_list[128];
};


struct Routing_Table_tag
{
  int     connected;
  int     link;
};

struct Router_Link_tag
{
  int    buffer_sz;
  int    connected;
  int    delay;

  double link_speed;
};

struct Host_Link_tag
{
  int connected;
  int delay;

  double link_speed;  
};


struct Host_Info_tag
{
  int type;           
  int connected;
  int size;

#ifdef CLIENT_DROP
  int max;
  int drop_index;
  int *packets;
#endif
};

struct Host_State_tag
{
  int   unack;
  int   seq_num;
  int   dup_count;
  int   len;
  int   timeout;
  int   rtt_seq;
  short   *out_of_order;
  tw_event *rto_timer;
  int     rto_seq; // probably do not need 
  double  smoothed_rtt;  //could be changed to shorts.
  double  dev;
  
  int   connection;

#ifdef CLIENT_DROP
  int count;
#endif

  double lastsent;
  double cwnd;
  double ssthresh;
  double rtt_time;
  double rto;

  /* stats */
  int received_packets;
  int timedout_packets;
  int sent_packets;
  double start_time;
};


struct Router_State_tag
{
  int dropped_packets;

  double *lastsent;
};

struct RC_tag
{
  double             dup_count;
  double             cwnd;
  double             rtt_time;
  double             lastsent;
  double             timer_ts;
  int                rtt_seq;
  tw_event           *rto_timer;
  int                timer_seq;
  int                seq_num;
  double             dev;
  double             smoothed_rtt; 
};

struct Msg_Data_tag
{
  MethodName_t    MethodName;
  int             ack;
  int             seq_num;
  int             source;
  double          dest;
  RC              RC;
};

struct tcpStatistics_tag
{
  int sent_packets;
  int timedout_packets;
  int received_packets;
  int dropped_packets;
  double throughput;
};

#endif









