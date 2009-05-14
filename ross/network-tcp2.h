#ifndef INC_network_tcp2_h
#define INC_network_tcp2_h

#include <ross.h>

INLINE(int)
tw_node_eq(tw_node *a, tw_node *b)
{
	return *a == *b;
}

INLINE(tw_lp *)
tw_net_destlp(tw_event * e)
{
	tw_error(TW_LOC, "cannot call tw_net_destlp!");
}

/*
 * This function returns the network node number for a given PE
 */
INLINE(tw_node *)
tw_net_onnode(tw_peid gid)
{
	return &g_tw_net_node[gid]->id;
}

INLINE(tw_peid)
tw_net_pemap(tw_peid gid)
{
	return pe_map[gid];
}

#endif
