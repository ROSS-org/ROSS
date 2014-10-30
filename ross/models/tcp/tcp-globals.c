/*********************************************************************
		              tcp-global.c
*********************************************************************/

#include "tcp.h"

int g_hosts;
int g_routers;
int g_npe;
int g_nkp;
int g_iss;
int g_recv_wnd;
int g_mss;
int g_frequency;
int g_type;
int g_num_links;

char          **g_new_routing_table;
struct Routing_Table **g_routing_table;
Router_Link   **g_routers_links;
int            *g_routers_info;
Host_Link      *g_hosts_links;
Host_Info      *g_hosts_info;

tcpStatistics TWAppStats;

#ifdef LOGGING

FILE           **host_received_log;
FILE           **host_sent_log;
FILE           **router_log;
FILE           **serv_tcpdump;
FILE           **serv_cwnd;

#endif
