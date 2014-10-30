/*********************************************************************
		               tcp-router.c
*********************************************************************/

#include "tcp.h"


/*********************************************************************
		  Initializes the router
*********************************************************************/

void 
tcp_router_StartUp(Router_State *SV, tw_lp * lp)
 {
   SV->dropped_packets = 0;
   SV->lastsent = (double *) calloc(g_routers_info[lp->id] ,sizeof(double));
 }


/*********************************************************************
	        Forwards and queues incoming packets
*********************************************************************/

void
tcp_router_forward(Router_State *SV,  tw_bf * CV,Msg_Data *M, tw_lp * lp)
{
  int         nexthop = tcp_util_nexthop(lp,(int) M->dest)->id;
  int         nexthop_link = tcp_util_nexthop_link(lp,M->dest);
  int         dest = (int) M->dest;
  int         packet_sz;
  double      sendtime;
  double      offset = tw_rand_unif(lp->rng) / 1000.0;

  if(offset > .002)
    printf("wrong random\n");

  sendtime = tw_now(lp);
  switch (g_hosts_info[dest - g_routers].type) 
    {
    case 0:
      packet_sz = (int) TCP_HEADER_SIZE;
      break;
    case 1:
      packet_sz = (int) TCP_HEADER_SIZE;
      break;
    case 2:
      packet_sz = (int) TCP_TRANSFER_SIZE; 
      break;
#ifdef CLIENT_DROP
    case 3:
      packet_sz = (int) TCP_TRANSFER_SIZE; 
      break;
#endif
    }

#ifdef LOGGING
 if(sendtime < SV->lastsent[nexthop_link]){
   fprintf(router_log[lp->id],"%f Router %d %d %f %d %d\n\tbyte queued %f %f %f packet size %d\n", 
	 tw_now(lp), M->ack,M->seq_num, SV->lastsent[nexthop_link], dest, nexthop_link, 
	 (SV->lastsent[nexthop_link] - sendtime),
	  g_routers_links[lp->id][nexthop_link].link_speed,
	 ((SV->lastsent[nexthop_link] - sendtime) * g_routers_links[lp->id][nexthop_link].link_speed),
	 packet_sz);}
 else
   fprintf(router_log[lp->id],"%f time hmmm...\n",tw_now(lp));
#endif   


 if((CV->c1 = (sendtime > SV->lastsent[nexthop_link])))  // make sure packet can fit on link CHANGE
    {
      M->RC.rtt_time = SV->lastsent[nexthop_link];
      //printf("not router %d dest %d source %d nexthop %d nexthop_link %d %d last %f offset %f\n\tcurrent time %f \n\n",
      //lp->id, (int) M->dest, (int) M->source, nexthop, nexthop_link, M->seq_num,  
      //SV->lastsent[nexthop_link],  SV->lastsent[nexthop_link] + g_routers_links[lp->id][nexthop_link].delay + offset,
      //tw_now(lp));

      SV->lastsent[nexthop_link] = packet_sz / g_routers_links[lp->id][nexthop_link].link_speed;
      tcp_util_event(lp, FORWARD, M->source, M->dest, M->seq_num, M->ack, 
		 SV->lastsent[nexthop_link] + g_routers_links[lp->id][nexthop_link].delay + offset);
      SV->lastsent[nexthop_link] += (sendtime + offset);
    
      // printf("ts %f lp %d dest %d source %d hop %d link %d sz %d time %f\n",
      //	     tw_now(lp),lp->id, dest, M->source, nexthop, nexthop_link, 
      //	     packet_sz, SV->lastsent[nexthop_link]);
    }
 else if( (CV->c2 = (((SV->lastsent[nexthop_link] - sendtime) * 
		      g_routers_links[lp->id][nexthop_link].link_speed) 
		     + packet_sz  <= g_routers_links[lp->id][nexthop_link].buffer_sz))) 
   {
     M->RC.rtt_time = SV->lastsent[nexthop_link];
     
     SV->lastsent[nexthop_link] += ((packet_sz / 
				     g_routers_links[lp->id][nexthop_link].link_speed) + offset);
     sendtime = SV->lastsent[nexthop_link] - sendtime;
     tcp_util_event(lp, FORWARD, M->source, M->dest, M->seq_num, M->ack,
		    sendtime + g_routers_links[lp->id][nexthop_link].delay);
     //printf("ts %f lp %d dest %d source %d hop %d link %d sz %d time %f\n",
     //	     tw_now(lp),lp->id, dest, M->source, nexthop, nexthop_link, 
     //	     packet_sz, SV->lastsent[nexthop_link]);
    }
 else 
   {
#ifdef LOGGING
     fprintf(router_log[lp->id],"\tDropped Packed on port[%d], ack = %d, Seq_num = %d\n", 
	     dest, M->ack, M->seq_num);
#endif
     SV->dropped_packets++;
   } 
}

/*********************************************************************
	               Router's EventHandler
*********************************************************************/

void 
tcp_router_EventHandler(Router_State *SV, tw_bf * CV, Msg_Data *M, tw_lp * lp)
{
  *(int *)CV = (int)0;
  //if(lp->id == 0 && tw_now(lp) > 71000 && tw_now(lp) < 73000) 
  //printf("router %d %d\n",lp->id,M->MethodName);

  switch (M->MethodName)
    {
    case FORWARD:
      tcp_router_forward(SV, CV, M, lp);
      break;
    default:
      tw_error(TW_LOC, "APP_ERROR(8)(%d): InValid MethodName(%d)\n",
	       lp->id, M->MethodName);
      tw_exit(1);
    }
}



/*********************************************************************
	            Collects the Statistic for a Router
*********************************************************************/

extern tcpStatistics TWAppStats;

void 
tcp_router_Statistics_CollectStats(Router_State *SV, tw_lp * lp)
{
  TWAppStats.dropped_packets += SV->dropped_packets;
}









