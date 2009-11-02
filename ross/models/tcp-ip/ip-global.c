#include <ip.h>


/*
 * g_ip_stats		-- collection point for IP layer stats
 * g_ip_fd		-- membuf queue descriptor
 * g_ip_header		-- ip pkt header size in bytes
 * g_ip_routing_simple	-- flag to indicate setting up simple IP layer routing
 * g_ip_routing_file	-- the file name to read/write routing table from/to
 */
ip_stats	*g_ip_stats = NULL;
int		 g_ip_header = 20;

tw_fd		 g_ip_fd;
int		 g_ip_routing_simple = 0;
int		 g_ip_write = 0;

FILE		*g_ip_routing_file = NULL;

/*
 * Globals for simple routing computation (Dijkstras Shortest Path)
 */
short int	*g_ip_front;
short int	*g_ip_p;
short int	*g_ip_f;
short int	*g_ip_s;
short int	*g_ip_d;
