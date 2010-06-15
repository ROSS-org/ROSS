#ifndef INC_network_tcp1_h
#define INC_network_tcp1_h

#include <arpa/inet.h>

#define MAX_NODES 128

struct tw_net_stats_tag;
typedef struct tw_net_stats_tag tw_net_stats;
struct tw_net_node_tag;
typedef struct tw_net_node_tag tw_net_node;

// required to be defined by ROSS: index into tw_net_node array
typedef unsigned int tw_eventid;
typedef unsigned int tw_node;

typedef struct sockaddr_in tw_socket;
typedef unsigned int tw_port;

	/*
	 * tw_mynode   -- my local node
	 */
extern tw_node		 tw_mynode;

extern tw_port		 g_tw_port;

extern int		 nnodes;
extern int		 nnet_nodes;

// the pe_map maps the global pe id to the local pe on the dest node
extern unsigned short int *pe_map;
extern unsigned int	*g_tw_node_ids;

extern tw_net_node	*g_tw_gvt_node;
extern tw_net_node	*g_tw_barrier_node;
extern tw_net_node	**g_tw_net_node;

extern tw_net_stats	 overall;

//extern unsigned int	g_tw_net_barrier_flag;

struct tw_net_stats_tag
{
	long		 s_nsend;
	long		 s_nrecv;

	tw_stime	 lgvt;
};

/*
 * tw_net_node: contain the representation of a TCP connection
 *
 * id		-- the node id
 * port		-- the port number listened on
 *
 * socket	-- the socket
 * socket_fd	-- the socket file descriptor
 * socket_sz	-- the size of the socket object
 * 
 * lients	-- channels to PEs we send events 'to'
 * servers	-- channels to PEs we recv events 'from'
 *
 * stats	-- statistics object
 */
struct tw_net_node_tag
{
	tw_node		 id;
	tw_port		 port;

	tw_socket	 socket;
	int		 socket_fd;
	unsigned int	 socket_sz;

	int		*clients;
	int		*servers;

	tw_net_stats	 stats;

	char		 hostname[256];
};

#endif
