#ifndef INC_network_none2_h
#define INC_network_none2_h

static inline int 
tw_node_eq(tw_node *a, tw_node *b)
{
	return *a == *b;
}

static inline tw_lp* 
tw_net_destlp(tw_event *e)
{
	return e->dest_lp;
}

static inline tw_peid 
tw_net_pemap(tw_peid gid)
{
	return gid;
}

static inline tw_node * 
tw_net_onnode(tw_peid gid)
{
	return &mynode;
}

#endif
