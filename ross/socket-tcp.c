#include <ross.h>

tw_net_node *
tw_socket_create_onport(tw_node src, tw_node dst, tw_port port, int blocking)
{
	tw_net_node		*master = g_tw_net_node[0];
	tw_net_node		*node;

	unsigned int             i;

	node = (tw_net_node *) tw_calloc(TW_LOC, "socket-tcp", sizeof(tw_net_node), 1);

	if (NULL == node)
		tw_error(TW_LOC, "Could not allocate mem for node.");

	node->clients = tw_calloc(TW_LOC, "socket-tcp", sizeof(int) * (nnodes - 1), 1);
	node->socket_sz = sizeof(node->socket);
	node->socket_fd = -1;

	/*
	 * If mynode == dest, then create server socket, other create client
	 */
	if (dst == g_tw_mynode)
	{
		//node->socket_fd = tw_server_socket_create(node->socket, port);
		socket_accept(node, (nnodes - 1));

		if (!blocking)
		{
			for (i = 0; i < (nnodes - 1); i++)
			{
				fcntl(node->clients[i], F_SETFL, O_NONBLOCK);
			}
		}

#if VERIFY_SOCKET_TCP
		printf("Node %d (master) recv's events on port %d \n", src, port);
#endif
	} else
	{
		node->socket_fd =
			tw_client_socket_create(master->hostname, &node->socket, port);

		printf("%d: client socket fd created %d \n", g_tw_mynode, node->socket_fd);
		socket_connect(node, node->socket_fd);

		if (!blocking)
		{
			fcntl(node->socket_fd, F_SETFL, O_NONBLOCK);
		}

#if VERIFY_SOCKET_TCP
		printf("Node %d (remote node) sends events on port %d \n", src, port);
#endif
	}

	return node;
}

void
tw_socket_create_mesh()
{
	tw_net_node		*node;
	tw_net_node		*jnode;

	unsigned int             i;
	unsigned int             j;

	/*
	 * IF mynode == dest, then create server socket, other create client
	 */
	for (i = 0; i < nnet_nodes; i++)
	{
		node = g_tw_net_node[i];

		/*
		 * Create server socket for pe and listen for client pe connections 
		 */
		if(g_tw_mynode == node->id)
		{
			//node->socket_fd = tw_server_socket_create(&node->socket, node->port);

			//printf("going to accept %d %d\n",i,g_tw_npe - g_tw_nlocal_pe);
			socket_accept(node, (nnet_nodes - g_tw_npe));

			for (j = 0; j < (nnet_nodes - g_tw_npe); j++)
				fcntl(node->clients[j], F_SETFL, O_NONBLOCK);

#if VERIFY_SOCKET_TCP
			printf("Creating local server on port %d for pe %d \n",
				node->port, i);
#endif
		} else
		{
			for (j = 0; j < nnet_nodes; j++)
			{
				/*
				 * Create my client socket connections
				 */
				if (g_tw_mynode != g_tw_net_node[j]->id ||
				    g_tw_net_node[i]->id == g_tw_net_node[j]->id)
					continue;

				//printf("going to connect %d %d\n", i, j );

				jnode = g_tw_net_node[j];
				jnode->servers[i] = 
					tw_client_socket_create(node->hostname,
								&node->socket,
								node->port);
				socket_connect(node, jnode->servers[i]);

#if VERIFY_SOCKET_TCP
				printf("Creating client connection to server %d (port %d) from pe %d \n",
					   i, node->port, j);
#endif
			}
		}

	}
}

int
socket_connect(tw_net_node * node, int fd)
{
	int             rv;

	while(1)
	{
		if ((rv = connect(fd, (struct sockaddr *) &node->socket,
					  sizeof(node->socket))) < 0)
			continue;

		break;
	}

	return 0;
}

int
socket_accept(tw_net_node * node, int num_of_clients)
{
	int             rv;
	unsigned int             i;

	i = 0;
	while (i < num_of_clients)
	{
		if ((rv = accept(node->socket_fd, (struct sockaddr *)&node->socket,
						 &node->socket_sz)) < 0)
			tw_error(TW_LOC, "Error calling accept, node: %d \n", node->id);
		else
			node->clients[i] = rv;

		i++;
	}

	return 0;
}

int
tw_socket_send_event(tw_event * event, tw_peid dest_peid)
{
	tw_net_node	*node;
	tw_kp		*kp;
	tw_lp		*src_lp;
	//tw_lp		*dest_lp;

#ifdef ROSS_MEMORY_LIB
	tw_memory	*memory;
	tw_memory	*m;
#endif

	tw_event	*temp_prev;

	int             rv;
	int		send_fd;
	size_t		mem_size;

	mem_size = 0;

	/*
	 * Save the lp pointers into temp vars and set lp pointers to lp->ids 
	 */
	src_lp = event->src_lp;
	//dest_lp = event->dest_lp;
	temp_prev = event->prev;

	kp = src_lp->kp;

	event->src_lp = (tw_lp *) src_lp->gid;

	// dest_lp contains LP global id
	//event->dest_lp = (tw_lp *) dest_lp->id;

	/*
	 * Store the sizeof the next memory buffer and the recv'r must figure
	 * that a memory buffer of that size is following the event.. then
	 * each successive memory buffer will also have the sizeof a 
	 * following mem buffer until all are sent (memory->prev = 0)
	 */
#ifdef ROSS_MEMORY_LIB
	memory = NULL;
	if(event->memory)
	{
		memory = event->memory;

		event->memory = (tw_memory *) 
			tw_memory_getsize(kp, (tw_fd) memory->prev);
		event->prev = (tw_event *) memory->prev;
		mem_size = (size_t) event->memory;
	}
#endif

	node = g_tw_net_node[src_lp->pe->id];
	send_fd = node->servers[dest_peid];
	rv = tw_socket_send(send_fd, (char *) event, sizeof(tw_event) + g_tw_msg_sz, 100);

	if (rv != sizeof(tw_event) + g_tw_msg_sz)
		tw_error(TW_LOC, "Did not send full event! (%d of %d)",
				 rv, sizeof(tw_event) + g_tw_msg_sz);

	/*
	 * Send the event's memory buffers
	 */
#ifdef ROSS_MEMORY_LIB
	m = NULL;
	while(memory)
	{
		m = memory->next;

		if(m)
		{
			memory->next = (tw_memory *) 
				tw_memory_getsize(kp, (tw_fd) m->prev);
			memory->prev = (tw_memory *) m->prev;
		}

		rv = tw_socket_send(send_fd, (char *) memory, mem_size, 100);

#if VERIFY_SOCKET_TCP
printf("%d: sending mem buf of size %d on ev %f \n", src_lp->id, mem_size, event->recv_ts);
#endif

		if(rv != mem_size)
			tw_error(TW_LOC,
				"Did not send full memory buffer! (%d of %d)",
				rv, mem_size);

		/* We will not need these mme bufs any longer on this side */
		tw_memory_free_single(src_lp->kp, memory, (tw_fd) memory->prev);

		memory = m;

		if(memory)
			mem_size = tw_memory_getsize(kp, (tw_fd) memory->prev);
	}
#endif

	/*
	 * Restore the event's lp pointers
	 */
	event->src_lp = src_lp;
	//event->dest_lp = dest_lp;
	event->prev = temp_prev;

#ifdef ROSS_MEMORY_LIB
	event->memory = NULL;
#endif

#if VERIFY_SOCKET_TCP
	if (event->state.cancel_q)
		printf
			("SEND CANCEL: dest %d %d: ts=%f sn=%d src %d %d\n",
			 event->dest_lp->pe->id,
			 event->dest_lp->id, event->recv_ts,
			 event->seq_num,
			 event->src_lp->pe->id,
			 event->src_lp->id);
	else
		printf
			("SEND NORMAL: dest %d %d: ts=%f sn=%d src %d %d\n",
			 event->dest_lp->pe->id,
			 event->dest_lp->id, event->recv_ts,
			 event->seq_num,
			 event->src_lp->pe->id,
			 event->src_lp->id);
#endif

	return rv;
}

tw_event       *
tw_socket_read_event(tw_pe * me)
{
	tw_net_node	*node = g_tw_net_node[me->id];

	tw_event       *recv_event;
	tw_event       *cancel_event;

#ifdef ROSS_MEMORY_LIB
	tw_memory	*last;
	tw_memory	*memory;
#endif

	//tw_message     *temp_message;
	void           *temp_data;

	//tw_pe          *send_pe;
	tw_peid		send_peid;
	tw_pe          *dest_pe;

	int             rv;
	unsigned int             i;

#ifdef ROSS_MEMORY_LIB
	void           *temp_mem_data;

	size_t		mem_size;
	tw_fd		mem_fd;
#endif

	rv = 0;

	/*
	 * Get a free event from our freeq and save the pointers
	 * to the message and the data for later use.
	 */
	if(NULL == (recv_event = tw_event_grab(me)))
		return NULL; //tw_error(TW_LOC, "Event grab failed!");

	//temp_message = recv_event->message;
	//temp_data = recv_event->message->data;
	temp_data = recv_event + 1;

	/*
	 * Attempt to read an event, and return NULL if no more events to recv.
	 */
	for (i = 0; i < nnet_nodes - g_tw_npe; i++)
	{
		rv = tw_socket_read(node->clients[i],
			(char *) recv_event, sizeof(tw_event) + g_tw_msg_sz, 100);

		if (rv > 0)
			break;
	}

	/*
	 * Check to see if we actually read an event
	 */
	if (1 > rv)
	{
		if(recv_event != me->abort_event)
		{
			recv_event->event_id = 0;
			tw_eventq_unshift(&me->free_q, recv_event);
		}

		return NULL;
	}

	if (recv_event == me->abort_event)
		tw_error(TW_LOC, "Out of memory!  Allocate more events!");

	if(recv_event->recv_ts < me->GVT)
		tw_error(TW_LOC, "Received straggler event!");

	/*
	 * Restore recv'ed event's pointers
	 *
	 * on recv'rs side: have dest_lp ptr, not src_lp ptr
	 */
	//recv_event->dest_lp = tw_getlp((tw_lpid)recv_event->dest_lp);
	//recv_event->src_lp = tw_getlp((tw_lpid)recv_event->src_lp);
	//recv_event->message = temp_message;
	//recv_event->message->data = temp_data;
	recv_event->dest_lp = tw_getlocal_lp((tw_lpid) recv_event->dest_lp);

	//send_pe = recv_event->src_lp->pe;
	send_peid = (recv_event->dest_lp->type.map)
				((tw_lpid) recv_event->src_lp);

	if(send_peid == me->id)
		tw_error(TW_LOC, "Sent event over network to self?");

	if (recv_event->recv_ts > g_tw_ts_end)
		tw_error(TW_LOC, "%d: Received remote event at %d, end=%d!", 
				recv_event->dest_lp->id,
				recv_event->recv_ts, g_tw_ts_end);

	if(recv_event->dest_lp->pe != me)
		tw_error(TW_LOC, "Not destination PE!");

	/*
	 * If a CANCEL message, just get the event out of hash table * and call 
	 * tw_event_cancel() on it, which rolls it back if nec 
	 */
	if(recv_event->state.owner == TW_net_acancel)
	{
#if VERIFY_SOCKET_TCP
		printf
			("\t\t\t\t\t\t\t\tREAD CANCEL: dest p%d l%d: ts=%f sn=%d\n",
			 recv_event->dest_lp->pe->id,
			 recv_event->dest_lp->id,
			 recv_event->recv_ts, recv_event->event_id);
#endif

		cancel_event = NULL;

		cancel_event = tw_hash_remove(me->hash_t, recv_event, send_peid);
		dest_pe = cancel_event->dest_lp->pe;
		cancel_event->state.cancel_q = 1;
		cancel_event->state.remote = 0;

		if(cancel_event == recv_event)
			tw_error(TW_LOC, "cancel_event == recv_event!");

		if(cancel_event->state.owner == 0 ||
			cancel_event->state.owner == TW_pe_free_q)
			tw_error(TW_LOC, "cancel_event no owner!");

		tw_mutex_lock(&dest_pe->cancel_q_lck);
		cancel_event->cancel_next = dest_pe->cancel_q;
		dest_pe->cancel_q = cancel_event;
		tw_mutex_unlock(&dest_pe->cancel_q_lck);

		recv_event->event_id = recv_event->state.cancel_q = 0;
		recv_event->state.remote = 0;

		tw_event_free(me, recv_event);

		return cancel_event;
	}

	recv_event->next = NULL;
	//recv_event->lp_state = NULL;
	recv_event->cancel_next = NULL;
	recv_event->caused_by_me = NULL;
	recv_event->cause_next = NULL;

	// signals for on-the-fly fossil collection
	recv_event->state.remote = 1;

	tw_hash_insert(me->hash_t, recv_event, send_peid);

#if VERIFY_SOCKET_TCP
	printf
		("\t\t\t\t\t\t\t\tREAD NORMAL: dest p%d l%d: ts=%f sn=%d src p%d l%d \n",
		 recv_event->dest_lp->pe->id,
		 recv_event->dest_lp->id,
		 recv_event->recv_ts, recv_event->seq_num,
		 recv_event->src_lp->pe->id,
		 recv_event->src_lp->id);
#endif

#ifdef ROSS_MEMORY_LIB
	mem_size = (size_t) recv_event->memory;
	mem_fd = (tw_fd) recv_event->prev;
	last = NULL;
	while(mem_size)
	{
		memory = tw_memory_alloc(recv_event->src_lp, mem_fd);
		temp_mem_data = memory->data;

		if(last)
			last->next = memory;
		else
			recv_event->memory = memory;

		rv = 0;
		while(rv != mem_size)
		{
			rv = tw_socket_read(node->clients[i],
					(char *) memory, mem_size, 100);
		}

		memory->data = temp_mem_data;
		memory->prev = (tw_memory *) mem_fd;

#if VERIFY_SOCKET_TCP
		printf("recv\'d mem buf of size %d on event %f\n", rv, recv_event->recv_ts);
#endif

		mem_size = (size_t) memory->next;
		mem_fd = (tw_fd) memory->prev;
		last = memory;
	}
#endif

	recv_event->prev = NULL;

	return recv_event;
}

int
tw_socket_close(tw_net_node * n)
{
	unsigned int             i;

	for (i = 0; i < nnodes - 1; i++)
		if (n->clients && n->clients[i] != 0)
			close(n->clients[i]);
	if (n->socket_fd != 0)
		close(n->socket_fd);
	return 0;
}

int
tw_server_socket_create(tw_socket * socket_addr, tw_port port)
{
	int             socket_fd;
	int             one = 1;

	if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		tw_error(TW_LOC, "Error creating socket.");

	bzero(socket_addr, sizeof(struct sockaddr_in));
	setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

	socket_addr->sin_addr.s_addr = INADDR_ANY;
	socket_addr->sin_family = AF_INET;
	socket_addr->sin_port = htons(port);

	if (bind(socket_fd, (struct sockaddr *) socket_addr, sizeof(struct sockaddr)) < 0)
	{
		printf("%s\n", strerror(errno));
		tw_error(TW_LOC, "Error binding server socket.");
	}

	if(listen(socket_fd, MAX_NODES) == -1)
		tw_error(TW_LOC, "Error listenening on server socket.");

	return socket_fd;
}

int
tw_client_socket_create(char *server, tw_socket * socket_addr, tw_port port)
{
	int             socket_fd;
	struct hostent *hp;

	if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		tw_error(TW_LOC, "Error creating socket.");

	bzero(socket_addr, sizeof(struct sockaddr_in));
	hp = gethostbyname(server);

	if (hp == NULL)
		tw_error(TW_LOC, "Unknown host: %s", server);

	bcopy((char *)hp->h_addr, (char *)&socket_addr->sin_addr, hp->h_length);
	socket_addr->sin_family = AF_INET;
	socket_addr->sin_port = htons(port);

	return socket_fd;
}

int
tw_socket_read(int fd, char *buffer, int bytes, int tries)
{
	int             rv;
	int             total = 0;
	int             give_up;

	if(fd == 0)
		tw_error(TW_LOC, "Trying to read from invalid fd %d ! \n", fd);

	give_up = tries ? tries : 100;

	do
	{
		if ((rv = read(fd, &buffer[total], bytes - total)) > 0)
		{
			total += rv;
		} else if (errno == EAGAIN && total == 0)
		{
			return 0;
		} else if (errno != EAGAIN && errno != 0)
		{
			perror("network read error");
			tw_error(TW_LOC, "network receive error occurred.");
		}
	} while (total < bytes || (give_up-- && (total == 0)));
	//} while(--give_up || total < bytes);


	if (give_up == 0 && total != bytes)
		tw_error(TW_LOC, "Giving up in read!");

	if (total != bytes)
	{
		tw_error(TW_LOC, "Only read %d out of %d bytes!", total, bytes);
	}

	return total;
}

int
tw_socket_send(int fd, char *buffer, int bytes, int tries)
{
	int             rv;
	int             give_up;
	int             total = 0;

	give_up = tries ? tries : 100;

	do
	{
		if ((rv = write(fd, &buffer[total], bytes - total)) < 0)
		{
			perror("network write error");
			tw_error(TW_LOC, "Network write error occurred.");
		} else
			total += rv;

		fsync(fd);
	} while(total < bytes);
	//} while (total < bytes || (total == 0 && give_up-- == 0));

	if (give_up == 0 && total != bytes)
		tw_error(TW_LOC, "Giving up in send\n");

	if (total != bytes && total)
		tw_error(TW_LOC, "Only sent %d out of %d bytes!", total, bytes);

	return total;
}
