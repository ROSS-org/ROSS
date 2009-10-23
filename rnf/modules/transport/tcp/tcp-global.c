#include <tcp.h>

tcp_state *g_state;

int		 g_tcp_mss = 1460;
//int		 g_tcp_mss = 2008;
int		 g_tcp_rwd = 32;

tw_fd		 g_tcp_fd = 0;
tcp_statistics	*g_tcp_stats;
