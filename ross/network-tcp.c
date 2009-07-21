#include <ross.h>
#include "socket-tcp.h"

unsigned short int *pe_map;

int		 pid;
tw_port		 g_tw_port = 9000;
unsigned int	*g_tw_node_ids = NULL;
//unsigned int	g_tw_hash_size = 67;

//char		**g_tw_net_node_names = NULL;
char		**g_tw_hosts = 0;
char		**g_tw_hostname = 0;
char		  g_tw_dist_logdir[1024] = "logs/";
char		  g_tw_net_config[1024] = ".ross_network";

tw_net_node		*g_tw_gvt_node = 0;
tw_net_node		*g_tw_barrier_node = 0;
tw_net_node		**g_tw_net_node = 0;

tw_net_stats	 overall;

void network_spawn(char * next_node);
char            my_hostname[256];

static char ross[4096];
static uintptr_t read_buffer;

int nnodes = 0;
int npe = 0;
int nnet_nodes = 0;

static unsigned int	g_tw_net_barrier_flag = 0;

static const tw_optdef tcp_opts[] = {
	TWOPT_GROUP("ROSS TCP Kernel"),
	TWOPT_UINT("port", g_tw_port, "Starting port offset"),
	TWOPT_UINT(
		"read-buffer",
		read_buffer,
		"network read buffer size in BYTES"),
	TWOPT_END()
};

/*
 * tw_net_init: parse ROSS Network file to determine our 'rank'
 */
const tw_optdef *
tw_net_init(int *argc, char ***argv)
{
	int i;
	char *next_arg;

	g_tw_net_device_size = read_buffer;

#if ROSS_GDB
	// if user wants to spawn ROSS kernels in GDB manually, let them.
	sprintf(ross, "cd %s && gdb %c", get_current_dir_name(), argv[0]);
#else
	sprintf(ross, "cd %s && ", get_current_dir_name());

	for(i = 0; i < *argc; i++)
	{
		next_arg = (*argv)[i];
		sprintf(ross, "%s %s", ross, next_arg);
	}

	//printf("Command line: %s \n", ross);
#endif

	return tcp_opts;
}

void
init_socket(int i, int node_index, char ** temp_hostname, int node_id)
{
	tw_pe * pe;
	tw_net_node * node;

	/*
	 * Init all local PE socket structures
	 */
	pe = g_tw_pe[i];
	pe->node = node_id;
	node = g_tw_net_node[node_index] = tw_calloc(TW_LOC, "net_node", sizeof(tw_net_node), 1);

	node->id = node_id;
	node->clients =
		tw_calloc(TW_LOC, "node clients", sizeof(int) * (nnet_nodes - g_tw_npe), 1);
	node->servers = tw_calloc(TW_LOC, "network_tcp", sizeof(int) * (nnet_nodes), 1);
	node->socket_sz = sizeof(node->socket);
	node->socket_fd = -1;
	node->port = g_tw_port + (pe->id * MAX_NODES);

	node->socket_fd = tw_server_socket_create(&node->socket, node->port);
	strcpy(node->hostname, *temp_hostname);

#if VERIFY_SOCKET_TCP || 1
	printf("Created server socket for PE %d (fd %d, port %d)\n", 
		pe->id, node->socket_fd, node->port);
#endif
 
#if VERIFY_SOCKET_TCP
	printf("%s: pe %d is on server port %d \n", node->hostname, pe->id, node->port);
#endif
}

void
tw_net_start(void)
{
	FILE		*fp;

	struct hostent	*hp;
	socklen_t	 k;

	tw_net_node	*master;
	tw_net_node	**next_node = NULL;
	tw_socket	 socket_addr;

	int		*clients;
	int		 socket_fd;
	//int		 finished = 0;
	int		 num_tries = 0;
	int		 rv = -1;

	int		 id;
	int		 i;
	int		 j;

	char		**hostname;
	char		**temp_hostname;
	char		  temp[256];

	g_tw_net_barrier_flag = 1;
	g_tw_masternode = 0;
	g_tw_mynode = -1;

	overall.s_nsend = 0;
	overall.s_nrecv = 0;

	g_tw_node_ids = tw_calloc(TW_LOC, "g_tw_node_ids", sizeof(*g_tw_node_ids), 1);
	hostname = tw_calloc(TW_LOC, "hostname ptr", sizeof(int), 1);
	hostname[0] = tw_calloc(TW_LOC, "hostname", sizeof(char), 256);
	temp_hostname = tw_calloc(TW_LOC, "hostname ptr", sizeof(int), 1);
	temp_hostname[0] = tw_calloc(TW_LOC, "hostname", sizeof(char), 256);

	if(0 != (rv = gethostname(my_hostname, 256)))
		tw_error(TW_LOC, "Unable to get my hostname!");

	/*
	 * Compute nnodes, nnet_nodes
	 */
	nnodes = 0;
	nnet_nodes = 0;

	if(!(fp = fopen(g_tw_net_config, "r")))
		tw_error(TW_LOC, "Unable to open: %s\n", g_tw_net_config);

	while(EOF != fscanf(fp, "%s", temp))
	{
		char * colon = strrchr(temp, ':');

		if(colon)
			sscanf(++colon, "%d", &npe);
		else
			npe = 1;

		nnet_nodes += npe;
		nnodes++;
	}
	rewind(fp);

	/*
	 * Now allocate structures
	 */
	pe_map = tw_calloc(TW_LOC, "pe_map", sizeof(*pe_map), nnet_nodes);
	g_tw_net_node = tw_calloc(TW_LOC, "Network Nodes", sizeof(tw_net_node), nnet_nodes);
#if 0
	g_tw_net_node_names = tw_calloc(TW_LOC, "node_names ptr", sizeof(char *), nnodes);

	for(i = 0; i < nnodes; i++)
		g_tw_net_node_names[i] = tw_calloc(TW_LOC, "node_name", 256,1);
#endif

	/*
	 *  Get the Master's node from .ross_network
	 */
	id = 0;
	j = 0;
	while(EOF != fscanf(fp, "%s", temp))
	{
		char		*colon = strrchr(temp, ':');

		if(colon)
		{
			*colon = '\0';
			colon++;
			sscanf(colon, "%d", &npe);
		} else
		{
			npe = 1;
		}

		sscanf(temp, "%as", temp_hostname);

		if(0 == strcmp(*temp_hostname, my_hostname))
		{
			g_tw_mynode = id;

			tw_pe_create(npe);

			for(i = 0; i < npe; i++)
			{
				pe_map[j] = i;
				tw_pe_init(i, j);

				init_socket(i, j++, temp_hostname, id);
				g_tw_pe[i]->hash_t = tw_hash_create();
				g_tw_pe[i]->seq_num = tw_calloc(TW_LOC, "seq_num",
							sizeof(tw_eventid), nnet_nodes);
			}

			if(0 == g_tw_mynode)
			{
				g_tw_master = 1;
#if VERIFY_NETWORK
				printf("Master: %s\n", my_hostname);
#endif
			} else
			{
#if VERIFY_NETWORK
				printf("Slave: %s (node %d)\n", my_hostname, g_tw_mynode);
#endif
			}

			if(j < nnet_nodes)
				next_node = &g_tw_net_node[j];
		} else
		{
			for(i = 0; i < npe; i++)
			{
				g_tw_net_node[j] = tw_calloc(TW_LOC, "", sizeof(tw_net_node), 1);
				strcpy(g_tw_net_node[j]->hostname, *temp_hostname);
				g_tw_net_node[j]->port = g_tw_port + (j * MAX_NODES);
				g_tw_net_node[j++]->id = id;
			}
		}

		id++;
	}

	master = g_tw_net_node[0];

	if (nnodes > MAX_NODES)
		tw_error(TW_LOC, "Can only allocate %d nodes.", MAX_NODES);

	//*hostname = g_tw_net_node_names[0];

	// spawn the next node here
	if(g_tw_mynode < nnodes-1)
		network_spawn((*next_node)->hostname);

	tw_socket_create_mesh();

	/*
	 *  Determine if we are the Master NODE, Master PE
	 */
	if(g_tw_master && 0 == strcmp(master->hostname, my_hostname))
	{
		g_tw_barrier_node = tw_calloc(TW_LOC, "network_tcp", sizeof(tw_net_node), 1);
		g_tw_barrier_node->clients = tw_calloc(TW_LOC, "", sizeof(int) * (nnodes - 1), 1);
		g_tw_barrier_node->socket_sz = sizeof(g_tw_barrier_node->socket);
		g_tw_barrier_node->socket_fd = -1;
		g_tw_barrier_node->socket_fd = tw_server_socket_create(&g_tw_barrier_node->socket, g_tw_port - 2);

		g_tw_gvt_node = tw_calloc(TW_LOC, "network_tcp", sizeof(tw_net_node), 1);
		g_tw_gvt_node->clients = tw_calloc(TW_LOC, "", sizeof(int) * (nnodes - 1), 1);
		g_tw_gvt_node->socket_sz = sizeof(g_tw_gvt_node->socket);
		g_tw_gvt_node->socket_fd = tw_server_socket_create(&g_tw_gvt_node->socket, g_tw_port - 1);

		socket_fd = tw_server_socket_create(&socket_addr, g_tw_port - 3);

		/*
		 *  While there are Nodes that we haven't heard from contiune to accept
		 *  Connections.
		 */
		clients = tw_calloc(TW_LOC, "clients", nnodes, sizeof(int));

		i = 1;
		while (i < nnodes)
		{
			k = sizeof(socket_addr);

			if((rv = accept(socket_fd, (struct sockaddr *)&socket_addr, &k)) < 0)
				tw_error(TW_LOC, "Error calling accept, node: %d, %s (%d)\n", i,
						 strerror(errno), rv);

			/*
			 * Store connected file descriptor into array 
			 */
			clients[i] = rv;

			/*
			 * Retrieve the client hostname from the connection 
			 */
			hp = gethostbyaddr((char *)&socket_addr.sin_addr,
							  sizeof(socket_addr.sin_addr),
							  socket_addr.sin_family);

			if(!hp)
				tw_error(TW_LOC, "Unable to get IP addr");

#if 0
			*hostname = hp->h_name;
			g_tw_net_node_names[i] = tw_calloc(TW_LOC, "", 1 + strlen(*hostname), 1);
			strcpy(g_tw_net_node_names[i], *hostname);
#endif
#if VERIFY_NETWORK || 0
			printf("Got connection from %s\n", hp->h_name);
#endif
			i++;
		}

#if 0
		for(i = 1; i < nnodes i++)
		{
			//printf("Writing to: %s\n", g_tw_net_node_names[i]);

			for (j = 1; j < nnodes; j++)
			{
				bzero(temp, 256);
				strcpy(temp, g_tw_net_node_names[j]);
				temp[strlen(g_tw_net_node_names[j])] = '\n';
				k = write(clients[i], temp, strlen(temp));
			}

			/*
			 * Send single NewLine 
			 */
			bzero(temp, 256);
			temp[0] = '\n';
			write(clients[i], temp, 1);
			close(clients[i]);
		}
		close(socket_fd);
#endif
	} else
	{
		socket_fd = tw_client_socket_create(master->hostname,
						&socket_addr, g_tw_port - 3);

		num_tries = 0;
		printf("%s trying to connect to %s \n", my_hostname, master->hostname);

		while((rv = connect(socket_fd, (struct sockaddr *)&socket_addr,
					  sizeof(socket_addr))) < 0)
			;

		/*
		 * exit since we can't connect 
		 */
		if (rv < 0)
			tw_error(TW_LOC, "I'm not a Node the Network\n");

#if 0
		i = 1;
		fp = fdopen(socket_fd, "r");
		while (!finished)
		{
			bzero(temp, 256);
			fgets(temp, 256, fp);

			/* Check for a newline by itself */
			if (0 == strcmp(temp, "\n"))
			{
				finished = 1;
				continue;
			}

			/* Remove new line */
			temp[strlen(temp) - 1] = '\0';
			g_tw_net_node_names[i] = (char *) tw_calloc(TW_LOC, "network_tcp", 1 + strlen(temp), 1);
			strcpy(g_tw_net_node_names[i], temp);

			//printf("Checking if I am Node %d is %s\n", i, g_tw_net_node_names[i]);

			i++;
		}

		fclose(fp);
#endif
	}

	close(socket_fd);

	if (g_tw_mynode == -1)
		tw_error(TW_LOC, "I am not a node on the network! (%s)", my_hostname);

	if(g_tw_master && 0 == strcmp(master->hostname, my_hostname))
	{
		// BARRIER ACCEPT
                socket_accept(g_tw_barrier_node, (nnodes - 1));

		// GVT ACCEPT
                socket_accept(g_tw_gvt_node, (nnodes - 1));

                for (i = 0; i < (nnodes - 1); i++)
                	fcntl(g_tw_gvt_node->clients[i], F_SETFL, O_NONBLOCK);

		//tw_socket_create_mesh();

		printf("Node %d has %d PEs. \n", g_tw_mynode, g_tw_npe);
		printf("Network init complete \n");
	
		return;
	}

	/*
	 * Setup the nodes for GVT and Barrier pessages
	 */
	printf("%d: Attempting to create BARRIER network node...", g_tw_mynode);
	g_tw_barrier_node = tw_socket_create_onport(g_tw_mynode, 0, g_tw_port - 2, 1);
	printf("done!\n");

	printf("%d: Attempting to create GVT network node...", g_tw_mynode);

	g_tw_gvt_node = tw_socket_create_onport(g_tw_mynode, 0, g_tw_port - 1, 0);
	printf("done!\n");

	//tw_socket_create_mesh();

	printf("Node %d has %d PEs. \n", g_tw_mynode, g_tw_npe);
	printf("Network init complete \n");

	return;
}

void
tw_net_cancel(tw_event * event)
{
#if VERIFY_NETWORK
	printf("%d: sent CANCEL event seq_num %d to lp %d \n",
		event->src_lp->id, event->seq_num,
		(int) event->dest_lp);
#endif

	if(event->state.owner != TW_net_asend)
		tw_error(TW_LOC, "No previous send!");

	event->state.owner = TW_net_acancel;
	event->state.cancel_q = 1;

	tw_net_send(event);

	tw_eventq_debug(&event->src_lp->pe->sevent_q);
	tw_eventq_delete_any(&event->src_lp->pe->sevent_q, event);
	tw_eventq_debug(&event->src_lp->pe->sevent_q);

	event->event_id = event->state.cancel_q = 0;
	tw_event_free(event->src_lp->pe, event);
}

unsigned int
tw_nnodes(void)
{
	return nnodes;
}

/*
 * Send LVT to master node on network 
 */
void
tw_net_send_lvt(tw_pe * pe, tw_stime LVT)
{
	tw_net_stats	 stats;// = g_tw_net_node[pe->id]->stats;

	int		 i;

	/*
	 * It isn't really necessary to obtain a lock for these
	 * values since a pe could send an event right after we
	 * do this summation -- however, we would not need to do
	 * this if we have correctly solved the transient message problem... 
	 * so to be absolutely certain, do this
	 */
	stats.s_nsend = 0;
	stats.s_nrecv = 0;
	stats.lgvt = LVT;

	for(i = 0; i < g_tw_npe; i++)
	{
		stats.s_nsend += g_tw_pe[i]->s_nsend_network;
		stats.s_nrecv += g_tw_pe[i]->s_nrecv_network;
	}

	tw_socket_send(g_tw_gvt_node->socket_fd,
			(char *) &stats, sizeof(tw_net_stats), 0);

#if VERIFY_GVT || 0
	printf("%d %d sent LVT %lf to Master, send %d, recv %d, LVT %lf \n",
		pe->id, (int) *tw_net_onnode(pe->id), 
		LVT,
		(int) stats.s_nsend, (int) stats.s_nrecv,
		stats.lgvt);
#endif
}

void
tw_net_gvt_compute(tw_pe * pe, tw_stime * LVT)
{
	tw_net_stats	stats;

	int		bytes = 0;
	int		i;


	if(pe->master)
	{
		overall.lgvt = *LVT;

		for (i = 0; i < nnodes - 1; i++)
		{
			bytes = tw_socket_read(g_tw_gvt_node->clients[i],
						(char *)&stats, sizeof(tw_net_stats), 0);

			if (bytes > 0)
			{
				pe->s_ngvts++;
#if VERIFY_GVT || 0
				printf("READ LVT: %f (%f)\n", stats.lgvt, overall.lgvt);
                                printf("old stats: SEND %ld READ %ld \n", overall.s_nsend, overall.s_nrecv);
                                printf("adding   : SEND %ld READ %ld \n", stats.s_nsend, stats.s_nrecv);

#endif

				overall.lgvt = *LVT = min(overall.lgvt, stats.lgvt);
				overall.s_nsend += stats.s_nsend;
				overall.s_nrecv += stats.s_nrecv;
			}
		}

		/* come back later to check for more GVT messages, SPEC EXEC */
		if (pe->s_ngvts < nnodes - 1)
			return;

		/* Now that we have all of the remote values, add in our own */
		for(i = 0; i < g_tw_npe; i++)
		{
			overall.s_nsend += g_tw_pe[i]->s_nsend_network;
			overall.s_nrecv += g_tw_pe[i]->s_nrecv_network;
		}

		// force GVT to _NOT_ advance if at end of simulation and we
		// have unaccounted sends / recvs
		if(overall.s_nsend != overall.s_nrecv && *LVT == DBL_MAX)
			overall.lgvt = *LVT = pe->GVT;

#if VERIFY_GVT
		printf("SENT: %ld, RECV %ld \n", overall.s_nsend, overall.s_nrecv);
		printf("Master send new GVT: %lf \n",
				overall.lgvt);
#endif

		for (i = 0; i < nnodes - 1; i++)
		{
			tw_socket_send(g_tw_gvt_node->clients[i],
							(char *)&overall, 
							sizeof(tw_net_stats), 0);
		}

		tw_gvt_set(pe, *LVT);

		overall.s_nsend = overall.s_nrecv = 0;
		*LVT = -1.0;
	} else
	{
		if(!pe->local_master)
			return;

		/*
		 * Read in new GVT from Master into overall
		 */
		overall.lgvt = 0;
		if (tw_socket_read(g_tw_gvt_node->socket_fd,
				   (char *)&overall, sizeof(tw_net_stats), 0) < 1)
			return;

		tw_gvt_set(pe, overall.lgvt);

		overall.s_nsend = overall.s_nrecv = 0;
		*LVT = -1.0;
	}
}

void
tw_net_read(tw_pe * pe)
{
	tw_pe		*dest_pe;
	tw_event       *recv_event;

	while (1)
	{
		/*
		 * This can happen if we either run out of events in the
		 * free_q, or if there are no more events to recv.. or the
		 * the event recv'ed had a recv_tc > end_tipe
		 */
		if ((recv_event = tw_socket_read_event(pe)) == NULL)
			break;

		pe->s_nrecv_network++;

		/*
		 * If a CANCEL message occurs, just continue, it has already been 
		 * placed in the CanQ.
		 */
		if (recv_event->state.cancel_q)
			continue;

		/*
		 * Put recv'ed event into local PE recv queues
		 */
		dest_pe = recv_event->dest_lp->pe;

		if(recv_event->dest_lp->kp->last_time <= recv_event->recv_ts)
		{
			tw_pq_enqueue(dest_pe->pq, recv_event);
		} else
		{
			recv_event->state.owner = TW_pe_event_q;

			tw_mutex_lock(&dest_pe->event_q_lck);
			tw_eventq_unshift(&dest_pe->event_q, recv_event);
			tw_mutex_unlock(&dest_pe->event_q_lck);
		}

		/*
		 * It is possible that the node that sent this did not account for the 
		 * time between when I recv'd this event and now, so I have to account 
		 * for the "local sending" of this event
		 */
		if(tw_gvt_inprogress(pe))
                        pe->trans_msg_ts = min(pe->trans_msg_ts, recv_event->recv_ts);
	}
}

void
tw_net_send(tw_event * event)
{
	int             rv;
	tw_pe          *src_pe = event->src_lp->pe;
	tw_peid		dest_peid = (*event->src_lp->type.map)
						((tw_lpid) event->dest_lp);

	event->state.hash_t = 0;

	// as long as this is not a cancellation, 
	// enqueue into my PEs remote send queue
	if(event->state.cancel_q == 0)
	{
		// configure event id and store in PE send event queue
		event->event_id = ++src_pe->seq_num[dest_peid];
		event->state.owner = TW_net_asend;
		tw_eventq_unshift(&event->src_lp->pe->sevent_q, event);
	}

	if (0 == (rv = tw_socket_send_event(event, dest_peid)))
		tw_error(TW_LOC, "Network send error.");
	else
		src_pe->s_nsend_network++;

#if VERIFY_NETWORK
	printf("%d: sent event id %d to lp %d \n",
		event->src_lp->id, event->event_id,
		event->dest_lp);
#endif
}

void
tw_net_barrier(tw_pe * pe)
{
	unsigned int             i;
	char            buffer[256];
	int             msg_len;


#if VERIFY_NETWORK
	printf("PE %d Node %d entering barrier. \n", pe->id, g_tw_mynode);
#endif

	/*
	 * 0 0 ONLY is MASTER
	 */
	if (pe->id == 0 && g_tw_mynode == 0)
	{
		/*
		 * Master waits for all to check-in
		 */
		for (i = 0; i < nnodes - 1;)
		{
			if ((msg_len = tw_socket_read(g_tw_barrier_node->clients[i],
								  &buffer[0], 16, 0)) > 0)
			{
				if (0 == strncmp(buffer, "NET_BARRIER_ENT", 16))
				{
					i++;
				} else
					tw_error(TW_LOC, "Recv'd unknown pessage: %d \n", buffer);
			}
		}

		/*
		 * Then sends all remote pes the ok to leave barrier pessage
		 */
		for (i = 0; i < nnodes - 1; i++)
		{
			tw_socket_send(g_tw_barrier_node->clients[i],
						   "NET_BARRIER_COM", 16, 0);
		}

		g_tw_net_barrier_flag = 0;
		tw_barrier_sync(&g_tw_network);
	} else if (pe->local_master && g_tw_mynode != 0)
	{
		tw_socket_send(g_tw_barrier_node->socket_fd, "NET_BARRIER_ENT", 16, 0);
		//printf("sent barr ent msg \n");
		while((msg_len = tw_socket_read(g_tw_barrier_node->socket_fd,
						buffer, 16, 0)) < 0)
			;

		tw_barrier_sync(&g_tw_network);
	} else
		tw_barrier_sync(&g_tw_network);

#if VERIFY_NETWORK
	printf("PE %d Node %d leaving barrier. \n", pe->id, g_tw_mynode);
#endif
}

void
tw_net_stop(void)
{
	unsigned int             i;

	int             waiting = 0; //1;

	if(g_tw_net_node) // pe->master || pe->local_master)
	{
		for (i = 0; i < nnet_nodes; i++)
			tw_socket_close(g_tw_net_node[i]);

		tw_socket_close(g_tw_gvt_node);
		tw_socket_close(g_tw_barrier_node);

		waiting = 0;
	} else
	{
		while (waiting)
			;
	}
}

#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif

/*
 * Start ROSS on each of the distributed nodes
 */
void
network_spawn(char * next_node)
{
	char 	spawn[2048];

#if ROSS_DIST_MANUAL
	return;
#endif

	if(0 == strcmp(g_tw_dist_logdir, ""))
	{
		sprintf(spawn, "%s > /dev/null %c", ross, '\0');
	} else
	{
		sprintf(spawn, "%s > %s/%s.log 2>&1 %c", 
			ross, g_tw_dist_logdir, next_node, '\0');
	}

	pid = fork();

	if(0 != pid)
		return;

	else if(-1 == pid)
		tw_error(TW_LOC, "Fork failed.");

#if ROSS_GDB
	execl("/usr/X11R6/bin/xterm", "xterm", 
			"-T", next_node, "-e",
		   	"/usr/bin/ssh", next_node, ross, NULL);
#else
	printf("ssh %s %s\n", next_node, spawn);
	execl("/usr/local/bin/ssh", "ssh", next_node, spawn, NULL);
#endif

	tw_error(TW_LOC, "Exec failed. pid %d", pid);
}

void
send_stats(tw_statistics * s)
{
	tw_socket_send(g_tw_barrier_node->socket_fd,
			(char *) s, sizeof(*s), 0);
}

static int client = 0;
void
recv_stats(tw_statistics * s)
{
	tw_socket_read(g_tw_barrier_node->clients[client++],
		(char *) s, sizeof(tw_statistics), INT_MAX);
}

void
tw_net_abort(void)
{
	exit(1);
}

tw_statistics	*
tw_net_statistics(tw_pe * me, tw_statistics * s)
{
	int	 i;

	// if not master, send stats (but still print locally
	// else, collate stats
	if (!tw_node_eq(&me->node, &g_tw_masternode))
	{
		send_stats(s);
		return s;
	} else
	{
		sleep(1);
		for(i = 0; i < tw_nnodes() - 1; i++)
		{
			tw_statistics * n_stats = tw_calloc(TW_LOC, "n_stats", sizeof(tw_statistics), 1);
	
			recv_stats(n_stats);

			s->max_run_time = max(s->max_run_time, n_stats->max_run_time);

			s->tw_s_net_events += n_stats->tw_s_net_events;
			s->tw_s_nevent_processed += n_stats->tw_s_nevent_processed;
			s->tw_s_nevent_abort += n_stats->tw_s_nevent_abort;
			s->tw_s_e_rbs += n_stats->tw_s_e_rbs;
			s->tw_s_rb_total += n_stats->tw_s_rb_total;
			s->tw_s_rb_primary += n_stats->tw_s_rb_primary;
			s->tw_s_rb_secondary += n_stats->tw_s_rb_secondary;
			s->tw_s_fc_attempts += n_stats->tw_s_fc_attempts;
			s->tw_s_pq_qsize += n_stats->tw_s_pq_qsize;
	
			s->tw_s_nsend_network += n_stats->tw_s_nsend_network;
			s->tw_s_nrecv_network += n_stats->tw_s_nrecv_network;

			s->tw_s_nsend_remote_rb += n_stats->tw_s_nsend_remote_rb;

			s->tw_s_nsend_net_remote += n_stats->tw_s_nsend_net_remote;
			s->tw_s_nsend_loc_remote += n_stats->tw_s_nsend_loc_remote;
		}
	}

	return s;
}
