#ifndef INC_network_mpi_h
#define INC_network_mpi_h

static long id_tmp;
typedef unsigned int tw_eventid;
typedef long tw_node;

tw_node * tw_net_onnode(tw_peid gid) {
	id_tmp = gid;
	return &id_tmp;
}

int tw_node_eq(tw_node *a, tw_node *b) {
	return *a == *b;
}

#endif
