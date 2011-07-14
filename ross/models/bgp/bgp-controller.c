/*
 *  Blue Gene/P model
 *  DDN
 *  by Ning Liu 
 */
#include "bgp.h"

void bgp_controller_init( CON_state* s,  tw_lp* lp )
{
  int N_PE = tw_nnodes();

  nlp_DDN = NumDDN / N_PE;
  nlp_Controller = nlp_DDN * NumControllerPerDDN;
  nlp_FS = nlp_Controller * NumFSPerController;
  nlp_ION = nlp_FS * N_ION_per_FS;
  nlp_CN = nlp_ION * N_CN_per_ION; 

  // get the file server gid based on the RR mapping
  // base is the total number of CN + ION lp in a PE
  int PEid = lp->gid / nlp_per_pe;
  int localID = lp->gid % nlp_per_pe;
  localID = localID - nlp_CN - nlp_ION - nlp_FS;
  localID /= NumControllerPerDDN;
  s->ddn_id = PEid * nlp_per_pe + nlp_CN + nlp_ION + nlp_FS+ nlp_Controller + localID;

  // get the IDs of the file servers which are hooked to this DDN
  int i;
  s->previous_FS_id = (int *)calloc( NumFSPerController, sizeof(int) );
  int base = nlp_CN + nlp_ION;
  localID = lp->gid % nlp_per_pe;
  localID = localID - base - nlp_FS;

  for (i=0; i<NumFSPerController; i++)
    s->previous_FS_id[i] = PEid * nlp_per_pe + base +
      localID * NumFSPerController + i;

#ifdef PRINTid
  printf("controller LP %d speaking, my DDN is %d \n", lp->gid, s->ddn_id);
  for (i=0; i<NumFSPerController; i++)
    printf("Controller LP %d speaking, my FS is %d\n", lp->gid,s->previous_FS_id[i]);
#endif

}

void bgp_controller_eventHandler( CON_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  tw_event * e;
  tw_stime ts;
  MsgData * m;
  int i;

  switch(msg->type)
    {
    case GENERATE:
      break;
    case ARRIVAL:

#ifdef TRACE
      printf("Controller %d recieved data\n",lp->gid);
#endif 

      s->next_available_time = max(s->next_available_time, tw_now(lp));
      ts = s->next_available_time - tw_now(lp);

      s->next_available_time += FS_packet_service_time;

      e = tw_event_new( lp->gid, ts, lp );
      m = tw_event_data(e);
      m->type = PROCESS;

      m->message_type = msg->message_type;
      m->travel_start_time = msg->travel_start_time;
      m->collective_msg_tag = msg->collective_msg_tag;
      m->message_size = msg->message_size;

      tw_event_send(e);
      
      break;
    case SEND:
      switch ( msg->message_type )
	{
	case ACK:
	  //printf("Controller %d received ACK\n",lp->gid);
	  
	  for (i=0; i<NumFSPerController; i++)
	    {
	      ts = FS_packet_service_time;
	      e = tw_event_new( s->previous_FS_id[i], ts, lp );
	      m = tw_event_data(e);
	      m->type = ARRIVAL;
	      
	      m->message_type = ACK;
	      m->travel_start_time = msg->travel_start_time; 
	      m->collective_msg_tag = msg->collective_msg_tag;
	      m->message_size = msg->message_size;
	      
	      tw_event_send(e);
	    }

	  break;
	case DATA:
	  s->nextLinkAvailableTime = max(s->nextLinkAvailableTime, tw_now(lp));
	  ts = s->nextLinkAvailableTime - tw_now(lp);                    
	  
	  s->nextLinkAvailableTime += msg->message_size/CON_DDN_bw;      
	  
	  e = tw_event_new( s->ddn_id, ts + msg->message_size/CON_DDN_bw, lp );
	  m = tw_event_data(e);
	  m->type = ARRIVAL;
	  
	  m->message_type = msg->message_type;
	  m->travel_start_time = msg->travel_start_time; 
	  m->collective_msg_tag = msg->collective_msg_tag; 
	  m->message_size = msg->message_size;
	  
	  tw_event_send(e);
	  break;
	case CONT:
	  break;
	}
      break;

    case PROCESS:
      ts = CON_packet_service_time;
      e = tw_event_new( lp->gid, ts, lp );
      m = tw_event_data(e);
      m->type = SEND;

      m->message_type = msg->message_type;
      m->travel_start_time = msg->travel_start_time;
      m->collective_msg_tag = msg->collective_msg_tag;
      m->message_size = msg->message_size;                 
               
      tw_event_send(e);

      break;
    }
}

void bgp_controller_eventHandler_rc( CON_state* s, tw_bf* bf, MsgData* m, tw_lp* lp )
{}

void bgp_controller_finish( CON_state* s, tw_lp* lp )
{
  free( s->previous_FS_id );
}
