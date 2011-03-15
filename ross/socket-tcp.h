#ifndef INC_socket_tcp_h
#define INC_socket_tcp_h

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <fcntl.h>

extern tw_net_node *tw_socket_create_onport(tw_node s, tw_node d, tw_port p, int b);
extern int      tw_server_socket_create(tw_socket * s, tw_port p);
extern int      tw_client_socket_create(char *svr, tw_socket * s, tw_port p);

extern int      socket_accept(tw_net_node * n, int num_of_clients);
extern int      socket_connect(tw_net_node * n, int);

extern int      tw_socket_send_event(tw_event *, tw_peid);
extern tw_event *tw_socket_read_event(tw_pe * me);
extern int      tw_socket_read(int fd, char *buf, int sz, int tries);
extern int      tw_socket_send(int fd, char *buf, int sz, int tries);
extern int      tw_socket_close(tw_net_node * s);
extern void	tw_socket_create_mesh();

#endif
