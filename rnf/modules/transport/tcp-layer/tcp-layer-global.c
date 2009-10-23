#include <tcp-layer.h>

int g_tcp_layer_iss = 0;
int g_tcp_layer_recv_wnd = 32;
int g_tcp_layer_mss = 512;

tcp_layer_statistics 	g_tcp_layer_stats;

#ifdef LOGGING
FILE           **host_received_log;
FILE           **host_sent_log;
FILE           **router_log;
FILE           **serv_tcpdump;
FILE           **serv_cwnd;
#endif
