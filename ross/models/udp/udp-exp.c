#include <ross.h>

/****************************************************************/
/* Defines  *****************************************************/
/****************************************************************/
#define APPLICATION_NEXT_BURST 100
#define APPLICATION_RECV       110

#define LINK_ENQUEUE           1000
#define LINK_DEQUEUE           1100

#define ROUTER_ARRIVAL         10000
#define ROUTER_RECV            11000

#define	TW_APPLICATION         1
#define	TW_LINK                2
#define	TW_ROUTER              3

/****************************************************************/
/* Global Variables *********************************************/
/****************************************************************/

int number_of_applications = 100000;

int packet_size = 500;
double burst_time = 500.0; /* milliseconds */
double idle_time = 100.0;  /* milliseconds */

double packet_rate = 2.0; /* per millisecond */
double pareto_shape = 1.5;

double link_bandwidth = 10000000; /* bytes per millisecond */
double link_delay = 10.0;

int router_zero_lp_id;
int router_one_lp_id;
int link_lp_id;

/****************************************************************/
/* Structures ***************************************************/
/****************************************************************/

struct Application_State
{
  double burst_time;
  int packets_sent_in_burst;
  int packets_sent;
  int packets_recved;
  int sequence_number;
  tw_lp *router;
};

struct Link_State
{
  double data_in_transit; 
  double max_data_in_transit; /* will be Link Bandwith div Link Delay */
  int dropped_packets;
};

struct Router_State
{
  int packets_sent;
  int packets_recved;
};

struct State
{
  union 
  {
    struct Application_State application;
    struct Link_State link;
    struct Router_State router;
  } u;
};

struct Packet_Data
{
  int method_name;
  int sequence_number;
  tw_lp *destination;
};

struct Network_Statistics
{
  int total_packets_sent;
  int total_packets_recved;
} NetStats;

/****************************************************************/
/* Prototypes ***************************************************/
/****************************************************************/

tw_peid Application_Map(tw_lpid gid);
void Application_Init(struct State *SV, tw_lp * lp);
void Application_Event_Handler( struct State *SV, tw_bf * CV, struct Packet_Data *M, tw_lp * lp);
void Application_Next_Burst( struct State *SV, tw_bf * CV, struct Packet_Data *M, tw_lp * lp);
void Application_Recv( struct State *SV, tw_bf * CV, struct Packet_Data *M, tw_lp * lp);
void RC_Not_Supported_Handler(struct State *SV, tw_bf * CV, struct Packet_Data *M, tw_lp * lp);
void Application_CollectStats(struct State *SV, tw_lp * lp);
void Link_Init(struct State *SV, tw_lp * lp);
void Link_Event_Handler(struct State *SV, tw_bf * CV, struct Packet_Data *M, tw_lp * lp);
void Link_EnQueue(struct State *SV, tw_bf * CV, struct Packet_Data *M, tw_lp * lp);
void Link_DeQueue(struct State *SV, tw_bf * CV, struct Packet_Data *M, tw_lp * lp);
void Link_CollectStats(struct State *SV, tw_lp * lp);
void Router_Init(struct State *SV, tw_lp * lp);
void Router_Event_Handler(struct State *SV, tw_bf * CV, struct Packet_Data *M, tw_lp * lp);
void Router_Arrival(struct State *SV, tw_bf * CV, struct Packet_Data *M, tw_lp * lp);
void Router_Recv(struct State *SV, tw_bf * CV, struct Packet_Data *M, tw_lp * lp);
void Router_CollectStats(struct State *SV, tw_lp * lp);

/****************************************************************/
/* Methods ******************************************************/
/****************************************************************/

tw_peid
Application_Map(tw_lpid gid)
{
	return (tw_peid) gid / g_tw_nlp;
}

void
Application_Init(struct State *SV, tw_lp * lp)
{
  SV->u.application.burst_time = 0.0;
  SV->u.application.packets_sent = 0;
  SV->u.application.packets_sent_in_burst = 0;
  SV->u.application.packets_recved = 0;
  SV->u.application.sequence_number = 0;
  if( lp->id < (number_of_applications / 2) )
    SV->u.application.router  = tw_getlp( router_zero_lp_id );
  else
    SV->u.application.router  = tw_getlp( router_one_lp_id );

  Application_Next_Burst( SV, NULL, NULL, lp );
 }

void 
Application_Event_Handler( struct State *SV, tw_bf * CV, struct Packet_Data *M, tw_lp * lp)
{
  if( M->method_name == APPLICATION_NEXT_BURST )
    Application_Next_Burst( SV, CV, M, lp);
  else if( M->method_name == APPLICATION_RECV )
    Application_Recv( SV, CV, M, lp);
 else
   {
     printf("Application_Event_Handler: Invalid method_name %d \n", M->method_name );
     exit(-1);
   }
}

void 
Application_Next_Burst( struct State *SV, tw_bf * CV, struct Packet_Data *M, tw_lp * lp)
{
  int dest=0;
  struct Packet_Data * packet;
  tw_event *e;
  double ts;

  ts = tw_rand_exponential( lp->rng, (1.0/packet_rate) ); 
  SV->u.application.burst_time += ts;

  if( SV->u.application.burst_time < burst_time )
    {
      SV->u.application.packets_sent++;
      SV->u.application.packets_sent_in_burst++;

      if( lp->id < (number_of_applications / 2) )
	dest = tw_rand_integer( lp->rng, 
				(number_of_applications / 2), 
				number_of_applications - 1);
      else
	  dest = tw_rand_integer( lp->rng, 
				  0,
				  (number_of_applications / 2) - 1); 

      e = tw_event_new(SV->u.application.router->gid, ts, lp);
      packet = (struct Packet_Data *) tw_event_data( e );
      packet->method_name = ROUTER_ARRIVAL;
      packet->sequence_number = SV->u.application.sequence_number++;
      packet->destination = tw_getlp( dest );
      tw_event_send( e );

#ifdef TRACE
      printf("Application %d: Sending New Pkt at TS = %f, SN = %d, Dest = %d, PSIB = %d \n",
              lp->id, tw_now(lp) + SV->u.application.burst_time  , packet->sequence_number, 
              packet->destination->id, SV->u.application.packets_sent_in_burst );
#endif /* TRACE */

      e = tw_event_new(lp->gid, ts, lp);
      packet = (struct Packet_Data *) tw_event_data( e );
      packet->method_name = APPLICATION_NEXT_BURST;
      packet->sequence_number = -1;
      tw_event_send( e );
    }
  else
    {
      printf("Application: Burst Completed At Time %f, Burst Size = %d \n",
              tw_now( lp ), SV->u.application.packets_sent_in_burst );
      SV->u.application.packets_sent_in_burst = 0;
      SV->u.application.burst_time = 0.0;
      e = tw_event_new(lp->gid, idle_time, lp);
      packet = (struct Packet_Data *) tw_event_data( e );
      packet->method_name = APPLICATION_NEXT_BURST;
      packet->sequence_number = -1;
      tw_event_send( e );
    }
}

void 
Application_Recv( struct State *SV, tw_bf * CV, struct Packet_Data *M, tw_lp * lp)
{
  SV->u.application.packets_recved++;

#ifdef TRACE
  printf("Application %d: Recv'd Pkt TS = %f, SN = %d, Dest = %d \n",
	 lp->id, tw_now( lp ), M->sequence_number, M->destination->id );
#endif /* TRACE */
}

void 
RC_Not_Supported_Handler(struct State *SV, tw_bf * CV, struct Packet_Data *M, tw_lp * lp)
{
  printf("RC Not Support for this application!...terminate \n");
  exit(-1);
}

void 
Application_CollectStats(struct State *SV, tw_lp * lp)
{
  NetStats.total_packets_sent += SV->u.application.packets_sent;
  NetStats.total_packets_recved += SV->u.application.packets_recved;

  // printf("Application(%d): Packets Sent:   %d \n", lp->id, SV->u.application.packets_sent );
  // printf("Application(%d): Packets Recved: %d \n", lp->id, SV->u.application.packets_recved );
}

void
Link_Init(struct State *SV, tw_lp * lp)
{
  SV->u.link.data_in_transit = 0.0;
  SV->u.link.max_data_in_transit = link_bandwidth / link_delay;
  SV->u.link.dropped_packets = 0;
}

void Link_EnQueue(struct State *SV, tw_bf * CV, struct Packet_Data *M, tw_lp * lp)
{
  tw_event *e;
  struct Packet_Data *packet;

  if( SV->u.link.data_in_transit + packet_size < 
      SV->u.link.max_data_in_transit )
    {
#ifdef TRACE
  printf("Link %d: EnQ Pkt at TS = %f, SN = %d, Dest = %d \n",
	 lp->id, tw_now( lp ), M->sequence_number, M->destination->id );
#endif /* TRACE */

      SV->u.link.data_in_transit += packet_size;
      e = tw_event_new( lp->gid, link_delay, lp );
      packet = (struct Packet_Data *) tw_event_data( e );
      packet->method_name = LINK_DEQUEUE;
      packet->sequence_number = M->sequence_number;
      packet->destination = M->destination;
      tw_event_send( e );
    }
  else
    {
    SV->u.link.dropped_packets++;
#ifdef TRACE
    printf("Link %d: Dropped Pkt at TS = %f, SN = %d, Dest = %d\n",
	   lp->id, tw_now( lp ),  M->sequence_number, M->destination->id );
#endif /* TRACE */
    }
}

void Link_DeQueue(struct State *SV, tw_bf * CV, struct Packet_Data *M, tw_lp * lp)
{
  tw_event *e;
  struct Packet_Data *packet;

  SV->u.link.data_in_transit -= packet_size;
  
  if( M->destination->id < (number_of_applications / 2) )
    e = tw_event_new( router_zero_lp_id, 0.0, lp );
  else
    e = tw_event_new( router_one_lp_id, 0.0, lp );

  packet = (struct Packet_Data *) tw_event_data( e );
  packet->method_name = ROUTER_RECV;
  packet->sequence_number = M->sequence_number;
  packet->destination = M->destination;
  tw_event_send( e );

#ifdef TRACE
    printf("Link %d: DeQ Pkt at TS = %f, SN = %d, Dest = %d \n",
	   lp->id, tw_now( lp ), M->sequence_number, M->destination->id );
#endif /* TRACE */
}

void Link_Event_Handler(struct State *SV, tw_bf * CV, struct Packet_Data *M, tw_lp * lp)
{
 if( M->method_name == LINK_ENQUEUE )
    Link_EnQueue( SV, CV, M, lp );
 else if( M->method_name == LINK_DEQUEUE )
    Link_DeQueue( SV, CV, M, lp );
 else
   {
     printf("Link_Event_Handler: Invalid method_name %d \n", M->method_name );
     exit(-1);
   }
} 

void 
Link_CollectStats(struct State *SV, tw_lp * lp)
{
  printf("Link(%lld): Dropped Packets: %d \n", lp->gid, SV->u.link.dropped_packets );
}

void
Router_Init(struct State *SV, tw_lp * lp)
{
  SV->u.router.packets_sent = 0;
  SV->u.router.packets_recved = 0;
}

void 
Router_Event_Handler(struct State *SV, tw_bf * CV, struct Packet_Data *M, tw_lp * lp)
{
  if( M->method_name == ROUTER_ARRIVAL )
    Router_Arrival( SV, CV, M, lp );
  else if( M->method_name == ROUTER_RECV )
    Router_Recv( SV, CV, M, lp );
 else
   {
     printf("Router_Event_Handler: Invalid method_name %d \n", M->method_name );
     exit(-1);
   }
}

void 
Router_Arrival(struct State *SV, tw_bf * CV, struct Packet_Data *M, tw_lp * lp)
{
  tw_event *e;
  struct Packet_Data *packet;

  SV->u.router.packets_sent++;

  e = tw_event_new( link_lp_id, 0.0, lp );
  packet = (struct Packet_Data *) tw_event_data( e );
  packet->method_name = LINK_ENQUEUE;
  packet->sequence_number = M->sequence_number;
  packet->destination = M->destination;
  tw_event_send( e );

#ifdef TRACE
    printf("Router %d: Arrival Pkt at TS = %f, SN = %d, Dest = %d \n",
	   lp->id, tw_now( lp ), M->sequence_number, M->destination->id );
#endif /* TRACE */

}

void 
Router_Recv(struct State *SV, tw_bf * CV, struct Packet_Data *M, tw_lp * lp)
{
  tw_event *e;
  struct Packet_Data *packet;

  SV->u.router.packets_sent++;

  e = tw_event_new( M->destination->gid, 0.0, lp );
  packet = (struct Packet_Data *) tw_event_data( e );
  packet->method_name = APPLICATION_RECV;
  packet->sequence_number = M->sequence_number;
  packet->destination = M->destination;
  tw_event_send( e );

#ifdef TRACE
    printf("Router %d: Recv Pkt at TS = %f, SN = %d, Dest = %d \n",
	   lp->id, tw_now( lp ), M->sequence_number, M->destination->id );
#endif /* TRACE */
}

void 
Router_CollectStats(struct State *SV, tw_lp * lp)
{
  return;
}

/****************************************************************/
/* LPType Structure**********************************************/
/****************************************************************/

tw_lptype       mylps[] =
{
	{ // Application
	 (init_f) Application_Init,
	 (event_f) Application_Event_Handler,
	 (revent_f) RC_Not_Supported_Handler,
	 (final_f) Application_CollectStats,
	 (map_f) Application_Map,
	 sizeof(struct Application_State),
	},
	{ // Link
	 (init_f) Link_Init,
	 (event_f) Link_Event_Handler,
	 (revent_f) RC_Not_Supported_Handler,
	 (final_f) Link_CollectStats,
	 (map_f) Application_Map,
	  sizeof(struct Link_State),
	},
	{ // Router
	 (init_f) Router_Init,
	 (event_f) Router_Event_Handler,
	 (revent_f) RC_Not_Supported_Handler,
	 (final_f) Router_CollectStats,
	 (map_f) Application_Map,
	 sizeof(struct Router_State),
	},
	{0}
};

/****************************************************************/
/* Main *********************************************************/
/****************************************************************/


int
main(int argc, char **argv)
{
  int             i;
  int             TWnlp;
  int             TWnkp;
  int             TWnpe;

  //tw_opt_add(app_opt);
  tw_init(&argc, &argv);

  TWnlp = number_of_applications + 3;
  router_zero_lp_id = number_of_applications;
  router_one_lp_id = number_of_applications + 1;
  link_lp_id = number_of_applications + 2;

  g_tw_events_per_pe = TWnlp * 5;
  TWnkp = TWnpe;

  printf("Running simulation with following configuration: \n" );
  printf("    Processors Used = %d\n", tw_nnodes() * g_tw_npe);
  printf("    KPs Used = %d\n", tw_nnodes() * g_tw_nkp);
  printf("    End Time = %f \n", g_tw_ts_end);
  printf("    Buffers Allocated Per PE = %d\n", g_tw_events_per_pe);
  printf("\n\n");

  tw_define_lps(TWnlp, sizeof(struct Packet_Data), 0);
  //if (tw_init(mylps, TWnpe, TWnkp, TWnlp, sizeof(struct Packet_Data)))


      for (i = 0; i < TWnlp; i++)
	{

	  if( i < number_of_applications )
	    {
	      tw_lp_settype(i, &mylps[0]);
	    }
	  else if( i == TWnlp -1 )
	    {
	      tw_lp_settype(i, &mylps[1]);
	    }
	  else
	    {
	      tw_lp_settype(i, &mylps[2]);
	    }
#if 0
	  tw_lp_onkp(lp, tw_getkp(i % TWnkp) );
          tw_kp_onpe(tw_getkp(i % TWnkp), tw_getpe(i % TWnpe));
	  tw_lp_onpe(lp, tw_getpe(i % TWnpe) );
#endif
	}

  /*
   * Initialize App Stats Structure 
   */
  NetStats.total_packets_sent = 0;
  NetStats.total_packets_recved = 0;

  tw_run();

  printf("NetStats: Total Packets Sent = %d \n",   NetStats.total_packets_sent );
  printf("NetStats: Total Packets Recved = %d \n", NetStats.total_packets_recved );
  
  return 0;
}







