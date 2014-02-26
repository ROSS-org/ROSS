#ifndef INC_network_mpi2_h
#define INC_network_mpi2_h

static long id_tmp;

static inline tw_node * 
tw_net_onnode(tw_peid gid)
{
	id_tmp = gid;
	return &id_tmp;
}

static inline int 
tw_node_eq(tw_node *a, tw_node *b)
{
	return *a == *b;
}

static inline tw_peid 
tw_net_pemap(tw_peid gid)
{
	return gid;
}

#endif
