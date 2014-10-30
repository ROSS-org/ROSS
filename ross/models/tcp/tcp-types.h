/*********************************************************************
		              tcp-types.h
*********************************************************************/

#ifndef TCP_TYPES_H
#define TCP_TYPES_H

#include "tcp.h"

typedef int MethodName_t;

#define	TW_TCP_HOST   1
#define TW_TCP_ROUTER 2

#define FORWARD       0
#define RECEIVED      1
#define RTO           2

#define CORE_ROUTERS  3187
#define TCP_SND_WND  65536
#define TCP_HEADER_SIZE 40.0
#define TCP_TRANSFER_SIZE (g_mss + TCP_HEADER_SIZE)

typedef struct Router_Link Router_Link;
typedef struct Host_Link Host_Link;
typedef struct Host_Info Host_Info;
typedef struct Router_State Router_State;
typedef struct Host_State Host_State;
typedef struct RC RC;
typedef struct Msg_Data Msg_Data;
typedef struct Tcp_Statistics tcpStatistics;

struct Router_Link
{
  int    buffer_sz;
  int    connected;
  int    delay;

  double link_speed;
};

struct Host_Link
{
  int connected;
  int delay;

  double link_speed;  
};


struct Host_Info
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

struct Host_State
{
  int   unack;
  int   seq_num;
  int   dup_count;
  int   len;
  int   timeout;
  int   rtt_seq;
  short   *out_of_order;
  tw_event *rto_timer;
  int     rto_seq;            // probably do not need 
  double  smoothed_rtt;       //could be changed to shorts.
  double  dev;
  
  int   connection;

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


struct Router_State
{
  int dropped_packets;

  double *lastsent;
};

struct RC
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

struct Msg_Data
{
  MethodName_t    MethodName;
  int             ack;
  int             seq_num;
  int             source;
  double          dest;
  RC              RC;
};

struct Tcp_Statistics
{
  long sent_packets;
  long timedout_packets;
  long received_packets;
  long dropped_packets;
  double throughput;
};

struct rocket_fuel_link
{
    int node_id;
    int bandwidth;
};

struct rocket_fuel_node
{
    int used;
    int level; int is_bb; int num_in_level;
    int num_links;
    int kp;
    int pe;
    struct rocket_fuel_link link_list[128];
};

struct Routing_Table
{
    int connected;
    int link;
};

#endif
