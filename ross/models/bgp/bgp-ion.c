/*
 *  Blue Gene/P model
 *  IO node
 *  by Ning Liu 
 */
#include "bgp.h"

void bgp_ion_init( ION_state* s,  tw_lp* lp )
{

  // figure out which file server I hooked to
  // base1 is the number of base lp-groups-fs
  int N_PE = tw_nnodes();                 
  nlp_DDN = NumDDN / N_PE;                                                  
  nlp_Controller = nlp_DDN * NumControllerPerDDN; 
  nlp_FS = nlp_Controller * NumFSPerController; 
  nlp_ION = nlp_FS * N_ION_per_FS;      
  nlp_CN = nlp_ION * N_CN_per_ION;

  // get the file server gid based on the RR mapping
  // base is the total number of CN lp in a PE

  int PEid = lp->gid / nlp_per_pe;
  int localID = lp->gid % nlp_per_pe;
  localID -= nlp_CN;

  localID /= N_ION_per_FS;
  s->file_server_id = PEid * nlp_per_pe + nlp_CN + nlp_ION + localID;

  printf("IO node LP %d speaking, my server is %d \n", lp->gid, s->file_server_id);  

  /*
  int base1 = (int)(lp->gid/N_lp_per_FS);

  s->file_server_id = base1*N_lp_per_FS + N_CN_per_FS + N_ION_per_FS;
  s->collective_round_counter = 0;
  */
}

void bgp_ion_eventHandler( ION_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  tw_event * e;
  tw_stime ts;
  MsgData * m;

  switch(msg->type)
    {
    case GENERATE:
      //packet_generate(s,bf,msg,lp);
      break;
    case ARRIVAL:

	  s->next_available_time = max(s->next_available_time, tw_now(lp));
	  ts = s->next_available_time - tw_now(lp);
	  
	  s->next_available_time += ION_packet_service_time;
	  
	  m->message_type = msg->message_type; 
	  e = tw_event_new( lp->gid, ts, lp );
	  m = tw_event_data(e);
	  m->type = PROCESS;
	  
	  m->travel_start_time = msg->travel_start_time;
	  m->collective_msg_tag = msg->collective_msg_tag;
	  m->MsgSrc = ComputeNode;
	  tw_event_send(e);
	  
	  //packet_arrive(s,bf,msg,lp);

      break;
    case SEND:
      if (msg->message_type==ACK)
	{
	  printf("ION recieved ACK message\n");
	}
      else
	{
	  //ts = 10000;
	  s->nextLinkAvailableTime = max(s->nextLinkAvailableTime, tw_now(lp));
	  ts = s->nextLinkAvailableTime - tw_now(lp);
	  
	  s->nextLinkAvailableTime += link_transmission_time;
	  
	  e = tw_event_new( s->file_server_id, ts, lp );
	  m = tw_event_data(e);
	  m->type = ARRIVAL;
	  
	  m->message_type = msg->message_type;
	  m->travel_start_time = msg->travel_start_time;
	  m->collective_msg_tag = msg->collective_msg_tag;
	  m->MsgSrc = ComputeNode;
	  tw_event_send(e);
	  //printf("ION ION Sent \n");
	  //packet_send(s,bf,msg,lp);
	}
      break;
    case PROCESS:
      //ts = 10000;
      if (msg->message_type==ACK)
	{
	  //printf("ION recieved ACK message\n");
	  ts = ION_packet_service_time;
	  e = tw_event_new( lp->gid, ts, lp );
	  m = tw_event_data(e);
	  m->type = SEND;
	  
	  m->message_type = msg->message_type;
	  m->travel_start_time = msg->travel_start_time;
	  m->collective_msg_tag = msg->collective_msg_tag;
	  tw_event_send(e);

	}
      else
	{
	  s->collective_round_counter++;
	  if ( s->collective_round_counter == N_CN_per_ION )
	    {
	      s->collective_round_counter = 0;
	      
	      ts = ION_packet_service_time;
	      e = tw_event_new( lp->gid, ts, lp );
	      m = tw_event_data(e);
	      m->type = SEND;
	      
	      m->message_type = msg->message_type;
	      m->travel_start_time = msg->travel_start_time;
	      m->collective_msg_tag = msg->collective_msg_tag;
	      m->MsgSrc = ComputeNode;
	      tw_event_send(e);
	      //packet_process(s,bf,msg,lp);
	    }
	}
      break;
    }

}

void bgp_ion_eventHandler_rc( ION_state* s, tw_bf* bf, MsgData* m, tw_lp* lp )
{}

void bgp_ion_finish( ION_state* s, tw_lp* lp )
{}
