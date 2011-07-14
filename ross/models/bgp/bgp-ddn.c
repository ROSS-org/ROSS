/*
 *  Blue Gene/P model
 *  DDN
 *  by Ning Liu 
 */
#include "bgp.h"

void bgp_ddn_init( DDN_state* s,  tw_lp* lp )
{
  int i;
  int N_PE = tw_nnodes();                                                           
  nlp_DDN = NumDDN / N_PE;                                                          
  nlp_Controller = nlp_DDN * NumControllerPerDDN;                                   
  nlp_FS = nlp_Controller * NumFSPerController;                                     
  nlp_ION = nlp_FS * N_ION_per_FS;
  nlp_CN = nlp_ION * N_CN_per_ION;

  // calculate the IDs of CON which are hooked to this DDN
  s->previous_CON_id = (int *)calloc( NumControllerPerDDN, sizeof(int) );
  int PEid = lp->gid / nlp_per_pe;
  int localID = lp->gid % nlp_per_pe;
  int base = nlp_CN + nlp_ION + nlp_FS;
  localID = localID - base - nlp_Controller;
  
  for (i=0; i<NumControllerPerDDN; i++)
    s->previous_CON_id[i] = PEid * nlp_per_pe + base + 
      localID * NumControllerPerDDN + i;

#ifdef PRINTid
  for (i=0; i<NumControllerPerDDN; i++)
    printf("DDN LP %d speaking, my CON is %d\n", lp->gid,s->previous_CON_id[i]);
#endif

}

void bgp_ddn_eventHandler( DDN_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
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
      printf("Packet start at %lf and now is %lf",msg->travel_start_time,tw_now(lp));
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

      printf("Msg reach DDN travel time is %lf\n",
	     tw_now(lp) - msg->travel_start_time );
      
      break;
    case SEND:
      /*
      s->nextLinkAvailableTime = max(s->nextLinkAvailableTime, tw_now(lp));         
      ts = s->nextLinkAvailableTime - tw_now(lp);                                   
      s->nextLinkAvailableTime += link_transmission_time;
      */

      for (i=0; i<NumControllerPerDDN; i++)
	{
	  e = tw_event_new( s->previous_CON_id[i], ts, lp );
	  m = tw_event_data(e);
	  m->type = ARRIVAL;
      
	  m->message_type = ACK;
	  m->collective_msg_tag = msg->collective_msg_tag;
	  m->message_size = ACK_message_size;
	  m->travel_start_time = msg->travel_start_time;

	  tw_event_send(e);
	}
      break;

    case PROCESS:
      ts = DDN_packet_service_time;
      e = tw_event_new( lp->gid, ts, lp );
      m = tw_event_data(e);
      m->type = SEND;

      m->travel_start_time = msg->travel_start_time;
      m->collective_msg_tag = msg->collective_msg_tag;
      m->message_size = msg->message_size;

      tw_event_send(e);
      break;
    }


}

void bgp_ddn_eventHandler_rc( DDN_state* s, tw_bf* bf, MsgData* m, tw_lp* lp )
{}

void bgp_ddn_finish( DDN_state* s, tw_lp* lp )
{
  free(s->previous_CON_id);
}
