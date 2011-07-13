/*
 *  Blue Gene/P model
 *  DDN
 *  by Ning Liu 
 */
#include "bgp.h"

void bgp_ddn_init( DDN_state* s,  tw_lp* lp )
{
  //printf("This is ION init\n");
  //printf("ION LP %d talking\n",lp->gid);

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
  for (i=0; i<NumControllerPerDDN; i++)
    printf("DDN LP %d speaking, my CON is %d\n", lp->gid,s->previous_CON_id[i]);
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
      //packet_generate(s,bf,msg,lp); 
      break;
    case ARRIVAL:
      //printf("Disk arrival\n");

      s->next_available_time = max(s->next_available_time, tw_now(lp));
      ts = s->next_available_time - tw_now(lp);

      s->next_available_time += FS_packet_service_time;
      //ts = 10000;
      e = tw_event_new( lp->gid, ts, lp );
      m = tw_event_data(e);
      m->type = PROCESS;

      m->MsgSrc = ComputeNode;
      tw_event_send(e);

      //packet_arrive(s,bf,msg,lp);  

      printf("Travel time is %lf\n",tw_now(lp) - msg->travel_start_time );

      break;
    case SEND:
      s->nextLinkAvailableTime = max(s->nextLinkAvailableTime, tw_now(lp));         
      ts = s->nextLinkAvailableTime - tw_now(lp);                                   
      s->nextLinkAvailableTime += link_transmission_time;

      for (i=0; i<NumControllerPerDDN; i++)
	{
	  e = tw_event_new( s->previous_CON_id[i], ts, lp );
	  m = tw_event_data(e);
	  m->type = ARRIVAL;
      
	  m->message_type = ACK;

	  tw_event_send(e);
	  //packet_send(s,bf,msg,lp);
	  //printf("File Server Sent \n");
	}
      break;

    case PROCESS:
      //ts = 10000;
      ts = FS_packet_service_time;

      e = tw_event_new( lp->gid, ts, lp );
      m = tw_event_data(e);
      m->type = SEND;

      m->travel_start_time = msg->travel_start_time;

      m->MsgSrc = ComputeNode;
      tw_event_send(e);
      //packet_process(s,bf,msg,lp);
      break;
    }


}

void bgp_ddn_eventHandler_rc( DDN_state* s, tw_bf* bf, MsgData* m, tw_lp* lp )
{}

void bgp_ddn_finish( DDN_state* s, tw_lp* lp )
{
  free(s->previous_CON_id);
}
