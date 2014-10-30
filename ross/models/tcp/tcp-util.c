/*********************************************************************
		              tcp-util.c
*********************************************************************/

#include "tcp.h"


/*********************************************************************
      Calculates the next hop from the current lp to the dest.
*********************************************************************/
  
tw_lp*
tcp_util_nexthop(tw_lp *lp, int dest)
{
  int nexthop_link;
  int nexthop;

  if(lp->id == dest)
    return lp;

  if(lp->id >= g_routers)
    return tw_getlp(g_hosts_links[lp->id - g_routers].connected);
  else if(!g_type)
    return tw_getlp(g_routing_table[lp->id][dest].connected);
  else if(g_type == -1) {
    if(lp->id != ((dest - g_routers)/ g_num_links)  + CORE_ROUTERS){
      nexthop_link = g_new_routing_table[lp->id][((dest - g_routers)/ g_num_links) + CORE_ROUTERS];
      nexthop = g_routers_links[lp->id][nexthop_link].connected;
      return tw_getlp(nexthop);
    }
    else{
      nexthop = dest;
      return tw_getlp(nexthop);
    }

  }
  else if(lp->id < g_type && ((dest - g_routers)/(int)pow(g_type,3)) == lp->id)
    return tw_getlp(((dest - ((g_type + 1) * g_type))/(int)pow(g_type,2)));
  else if(lp->id < g_type)
    return tw_getlp((dest - g_routers)/(int)pow(g_type,3));

  else if(lp->id < pow(g_type,2) + g_type && (dest -  ((g_type + 1) * g_type))/(int)pow(g_type,2) == lp->id)
    return tw_getlp(dest/g_type - 1);
  else if (lp->id < pow(g_type,2) + g_type)
    return tw_getlp(lp->id/g_type - 1);    
  else if(lp->id < g_routers && (dest/(int)g_type) - 1 == lp->id)
    return tw_getlp(dest);
  else
    return tw_getlp(lp->id/g_type - 1);    
}

/*********************************************************************
      Calculates the next hop from the current lp to the dest.
*********************************************************************/
  
int
tcp_util_nexthop_link(tw_lp *lp, int dest)
{
  if(!g_type)
    return g_routing_table[lp->id][dest].link;

  else if(g_type == -1) {
    if(lp->id != ((dest - g_routers)/ g_num_links)  + CORE_ROUTERS)
      return g_new_routing_table[lp->id][((dest - g_routers)/ g_num_links) + CORE_ROUTERS];
    else
      return ((dest - g_routers) % g_num_links) + 1; 
  }

  else if(lp->id < g_type && ((dest-g_routers)/(int)pow(g_type,3)) == lp->id)
    return (((dest - ((g_type + 1) * g_type))/(int)pow(g_type,2)) % g_type) + (g_type - 1);
  else if(lp->id < g_type)
    if( lp->id - ((dest- g_routers)/(int)pow(g_type,3)) <= 0)
      return ((dest-g_routers)/(int)pow(g_type,3)) - 1;
    else
      return ((dest-g_routers)/(int)pow(g_type,3));
  else if(lp->id < pow(g_type,2) + g_type && (dest -  ((g_type + 1) * g_type))/(int)pow(g_type,2) == lp->id)
    return (dest/g_type - 1) % g_type + 1;
  else if(lp->id < pow(g_type,2) + g_type )
    return 0 ;
  else if(lp->id < g_routers && (dest/(int)g_type) - 1 == lp->id)
    return (dest % g_type) + 1;
  else
    return 0;    
}


/*********************************************************************
		       Creates a message 
*********************************************************************/

void
tcp_util_event(tw_lp * lp, int type, int source, int dest, int seq_num, int ack, 
	       double ts)
{
  int i;
  Msg_Data * TWMsg;
  tw_event *CurEvent;
  
  //    printf("tw_now %f %f source %d dest %d ts %f seq %d ack %d type %d %d\n",
  //	   tw_now(lp), tw_now(lp) + ts, source, dest, ts,
  //	   seq_num, ack, type,tcp_util_nexthop(lp,dest)->id );

  CurEvent = tw_event_new(tcp_util_nexthop(lp,dest)->gid, ts, lp);
 
  TWMsg = (Msg_Data *)tw_event_data(CurEvent);
  TWMsg->MethodName = type;
  TWMsg->source = source;
  TWMsg->dest = dest;
  TWMsg->seq_num = seq_num;
  TWMsg->ack = ack;
  tw_event_send(CurEvent);

}










