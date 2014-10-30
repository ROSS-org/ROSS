/*********************************************************************
		             tcp-router-rc.c
*********************************************************************/

#include "tcp.h"


/*********************************************************************
                    Reverses the forwarding of packets
*********************************************************************/

void
tcp_router_forward_rc(Router_State *SV,  tw_bf *CV, Msg_Data *M, tw_lp * lp)
{  
  int nexthop_link = tcp_util_nexthop_link(lp,M->dest);
  tw_rand_reverse_unif(lp->rng);

  if(CV->c1 || CV->c2)
    SV->lastsent[nexthop_link] = M->RC.rtt_time;
  else {
    /*     switch(g_routers_links[lp->id][nexthop_link].buffer_sz){
      case 12400000:
	SV->dropped_packets[0]++; 
	break;
      case 3000000:
	SV->dropped_packets[1]++; 
	break;
      case 775000:
	SV->dropped_packets[2]++;
	break;
      case 200000:
	SV->dropped_packets[3]++;
	break;
      case 60000:
	SV->dropped_packets[4]++;
	break;
      case 20000:
	SV->dropped_packets[5]++;
	break;
      case 10000:
	SV->dropped_packets[6]++;
	break;
      case 5000:
	SV->dropped_packets[7]++;
	break;

      default:
	printf("there has been error in buffer_sz %d\n",
	       g_routers_links[lp->id][nexthop_link].buffer_sz);
	exit(0);
	}*/
    SV->dropped_packets--;
  }
}


/*********************************************************************
                    Reverses the routers EventHandler
*********************************************************************/

void 
tcp_router_rc_EventHandler(Router_State *SV, tw_bf * CV,Msg_Data *M, tw_lp * lp)
{
  //printf("router rollback %f lp %d\n",tw_now(lp),lp->id);
  switch (M->MethodName)
    {
    case FORWARD:
      tcp_router_forward_rc(SV, CV, M, lp);
      break;
    default:
      tw_error(TW_LOC, "APP_ERROR(8)(%d): InValid MethodName(%d)\n",
	       lp->id, M->MethodName);
      tw_exit(1);
    }
}


