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

int number_of_applications = 1024;

int packet_size = 500;
double burst_time = 500.0; /* milliseconds */
double idle_time = 100.0;  /* milliseconds */

double packet_rate = 200000.0;  /* per millisecond */
double pareto_shape = 1.5;
double pareto_interval = 0;
double pareto_p1 = 0.0;     /* scale for num pkts in burst */
double pareto_p2 = 0.0;     /* scale for idle time         */

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
  int packets_remaining_in_burst;
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

void
Application_Init(struct State *SV, tw_lp * lp)
{
  SV->u.application.packets_sent = 0;
  SV->u.application.packets_remaining_in_burst = 0;
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
     tw_error( TW_LOC, "");
   }
}

void 
Application_Next_Burst( struct State *SV, tw_bf * CV, struct Packet_Data *M, tw_lp * lp)
{
  int dest=0;
  struct Packet_Data * packet;
  tw_event *e;
  double ts;

  if( SV->u.application.packets_remaining_in_burst > 0 )
    {
      SV->u.application.packets_sent++;
      SV->u.application.packets_remaining_in_burst--;

      if( lp->id < (number_of_applications / 2) )
	dest = tw_rand_integer( lp->id, 
				(number_of_applications / 2), 
				number_of_applications - 1);
      else
	  dest = tw_rand_integer( lp->id, 
				  0,
				  (number_of_applications / 2) - 1); 

      e = tw_event_new(SV->u.application.router, pareto_interval, lp);
      packet = (struct Packet_Data *) tw_event_data( e );
      packet->method_name = ROUTER_ARRIVAL;
      packet->sequence_number = SV->u.application.sequence_number++;
      packet->destination = tw_getlp( dest );
      tw_event_send( e );

#ifdef TRACE
      printf("Application %d: Sending New Pkt at TS = %f, SN = %d, Dest = %d, PRIB = %d \n",
              lp->id, tw_now(lp) + pareto_interval , packet->sequence_number, 
              packet->destination->id, SV->u.application.packets_remaining_in_burst );
#endif TRACE

      e = tw_event_new(lp, pareto_interval, lp);
      packet = (struct Packet_Data *) tw_event_data( e );
      packet->method_name = APPLICATION_NEXT_BURST;
      packet->sequence_number = -1;
      tw_event_send( e );
    }
  else
    {
#ifdef TRACE
      printf("Application %d: Burst Completed At Time %f \n",
              lp->id, tw_now( lp ) );
#endif /* TRACE */

      SV->u.application.packets_remaining_in_burst = 
	rint(tw_rand_pareto( lp->id, pareto_shape, pareto_p1 ) + 0.5);

      if( SV->u.application.packets_remaining_in_burst == 0 )
	SV->u.application.packets_remaining_in_burst = 1;

      ts = tw_rand_pareto( lp->id, pareto_shape, pareto_p2);
      e = tw_event_new(lp, ts, lp);
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

  printf("Application(%d): Packets Sent:   %d \n", lp->id, SV->u.application.packets_sent );
  printf("Application(%d): Packets Recved: %d \n", lp->id, SV->u.application.packets_recved );
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
      e = tw_event_new( lp, link_delay, lp );
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
    e = tw_event_new( tw_getlp( router_zero_lp_id ), 0.0, lp );
  else
    e = tw_event_new( tw_getlp( router_one_lp_id ), 0.0, lp );

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
  printf("Link(%d): Dropped Packets: %d \n", lp->id, SV->u.link.dropped_packets );
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

  e = tw_event_new( tw_getlp(link_lp_id), 0.0, lp );
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

  e = tw_event_new( M->destination, 0.0, lp );
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
	{TW_APPLICATION, sizeof(struct Application_State),
	 (init_f) Application_Init,
	 (event_f) Application_Event_Handler,
	 (revent_f) RC_Not_Supported_Handler,
	 (final_f) Application_CollectStats,
	 (statecp_f) NULL},
	{TW_LINK, sizeof(struct Link_State),
	 (init_f) Link_Init,
	 (event_f) Link_Event_Handler,
	 (revent_f) RC_Not_Supported_Handler,
	 (final_f) Link_CollectStats,
	 (statecp_f) NULL},
	{TW_ROUTER, sizeof(struct Router_State),
	 (init_f) Router_Init,
	 (event_f) Router_Event_Handler,
	 (revent_f) RC_Not_Supported_Handler,
	 (final_f) Router_CollectStats,
	 (statecp_f) NULL},
	{0}
};

/****************************************************************/
/* Main *********************************************************/
/****************************************************************/


int
main(int argc, char **argv)
{
  int             i=0;
  int             TWnlp=0;
  int             TWnkp=0;
  int             TWnpe=0;
  tw_lp          *lp=NULL;
  tw_kp          *kp=NULL;
  tw_pe          *pe=NULL;


  TWnlp = number_of_applications + 3;
  router_zero_lp_id = number_of_applications;
  router_one_lp_id = number_of_applications + 1;
  link_lp_id = number_of_applications + 2;

  printf("Enter Simulation End Time: \n" );
  scanf("%lf", &g_tw_ts_end );

  TWnpe = 1;
  TWnkp = 1;
  g_tw_gvt_interval = 16;
  g_tw_mblock = 16;

  if( TWnpe != 1 )
    {
      printf("Only 1 Processor Supported for NOW, resetting TWnpe to 1 \n");
      printf("Setting TWnkp to 1 as well!! \n");
      TWnpe = 1;
      TWnkp = 1;
    }

  /* This controls memory allocation -- if you need more event memory, increase it */
  g_tw_events_per_pe = TWnlp * 6; 

  printf("Running simulation with following configuration: \n" );
  printf("    Processors Used = %d\n", TWnpe);
  printf("    End Time = %f \n", g_tw_ts_end);
  printf("    Buffers Allocated Per PE = %d\n", g_tw_events_per_pe);
  printf("    Gvt Interval = %d\n", g_tw_gvt_interval);
  printf("    Message Block Size (i.e., Batch) = %d\n", g_tw_mblock);
  printf("\n\n");

  pareto_interval = (double) (packet_size << 3) / (double) (packet_rate);
  pareto_p1 = (burst_time / pareto_interval) * ((pareto_shape - 1.0) / pareto_shape);
  pareto_p2 = idle_time * ((pareto_shape - 1.0) / pareto_shape);

  printf("Network Simulation Configuration: \n" );
  printf("   Number of Applications......... %d \n", number_of_applications );
  printf("   Packet Size.................... %d bytes \n", packet_size );
  printf("   Burst Time..................... %f ms \n", burst_time );
  printf("   Idle Time...................... %f ms \n", idle_time );
  printf("   Packet Rate.................... %f per ms \n", packet_rate );
  printf("   Pareto Shape................... %f \n", pareto_shape );
  printf("   Pareto Interval................ %f \n", pareto_interval );
  printf("   Pareto P1...................... %f \n", pareto_p1 );
  printf("   Pareto P2...................... %f \n", pareto_p2 );
  printf("   Link Bandwidth................. %f bytes per ms\n", link_bandwidth );
  printf("   Link Delay..................... %f ms\n", link_delay );

  if (tw_init(mylps, TWnpe, TWnkp, TWnlp, sizeof(struct Packet_Data)))
    {
      kp = tw_getkp( 0 ); /* ONLY 1 KP */
      pe = tw_getpe( 0 ); /* ONLY 1 PE */

      tw_kp_onpe( kp, pe ); /* Set mapping from KP to PE */

      for (i = 0; i <TWnlp; i++)
	{
	  lp = tw_getlp(i);

	  if( i < number_of_applications )
	    {
	      tw_lp_settype(lp, TW_APPLICATION);
	    }
	  else if( i == TWnlp -1 )
	    {
	      tw_lp_settype(lp, TW_LINK);
	    }
	  else
	    {
	      tw_lp_settype(lp, TW_ROUTER);
	    }
	  tw_lp_onkp(lp, kp );
	  tw_lp_onpe(lp, pe );
	}
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







