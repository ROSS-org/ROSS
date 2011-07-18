/*
 *  Blue Gene/P model
 *  IO node
 *  by Ning Liu 
 */
#include "bgp.h"

void bgp_ion_init( ION_state* s,  tw_lp* lp )
{
  //default mapping of ION to FS

  // figure out which file server I hooked to
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

  // calculate packet round
  s->N_packet_round = collective_block_size/payload_size/N_CN_per_ION;

#ifdef PRINTid
  printf("ION %d speaking, my server is %d \n", lp->gid, s->file_server_id);  
#endif

  s->collective_round_counter = 0;
  s->total_size = 0;
}

void bgp_ion_eventHandler( ION_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  tw_event * e;
  tw_stime ts;
  MsgData * m;

  switch(msg->type)
    {
    case CONFIG:
      printf("ION %d received CONFIG CONT message\n", lp->gid);
      s->root_CN_id = msg->msg_src_lp_id;
      break;
    case IOrequest:
      s->collective_round_counter++;
      if ( s->collective_round_counter == N_CN_per_ION)
	{
	  s->collective_round_counter = 0;
	  printf("ION received IO request from CN\n");
	  e = tw_event_new( s->root_CN_id, ts, lp );
	  m = tw_event_data(e);
	  m->type = IOrequest;
	  
	  m->travel_start_time = msg->travel_start_time;
	  m->collective_msg_tag = msg->collective_msg_tag;
	  m->message_type = ACK;
	  
	  tw_event_send(e);
	}
      break;
    case GENERATE:
      // Take a rest here. Maybe used for debugging.
      break;
    case ARRIVAL:
      switch( msg->message_type )
	{
	case DATA:
	  //printf("collective round counter is %d\n",s->collective_round_counter);
	  s->collective_round_counter++;
	  if ( s->collective_round_counter == N_CN_per_ION * 
	       collective_block_size/payload_size )
	    {
	      printf("ION recieved data payload and travel time is %lf\n",
		     tw_now(lp) - msg->travel_start_time );
	    }
	    
	  /*
	  s->next_available_time = max(s->next_available_time, tw_now(lp));
	  ts = s->next_available_time - tw_now(lp);
	  
	  s->next_available_time += ION_packet_service_time;
	  
	  e = tw_event_new( lp->gid, ts, lp );
	  m = tw_event_data(e);
	  m->type = PROCESS;
	  
	  m->travel_start_time = msg->travel_start_time;
	  m->collective_msg_tag = msg->collective_msg_tag;
	  m->message_type = msg->message_type; 
	  m->message_size = msg->message_size; 
	  
	  tw_event_send(e);
	  */
	  break;
	case ACK:
	  ts = 10;
	  e = tw_event_new( lp->gid, ts, lp );
	  m = tw_event_data(e);
	  m->type = PROCESS;
	  
	  m->travel_start_time = msg->travel_start_time;
	  m->collective_msg_tag = msg->collective_msg_tag;
	  m->message_type = msg->message_type; 
	  m->message_size = msg->message_size; 
	  
	  tw_event_send(e);
	  break;
	}
      break;
    case SEND:
      switch( msg->message_type )
	{      
	case ACK:
	  {
	    printf("ION recieved ACK message, tag is %d, travel time is %lf\n", 
		   msg->collective_msg_tag,
		   tw_now(lp) - msg->travel_start_time);
	    ts =10;

	    e = tw_event_new( s->root_CN_id, ts, lp);
	    m = tw_event_data(e);
	    m->type = ARRIVAL;
	    
	    // pass msg info
	    m->message_type = msg->message_type;
	    m->travel_start_time = msg->travel_start_time;
	    m->collective_msg_tag = msg->collective_msg_tag;
	    m->message_size = msg->message_size;
	  
	    tw_event_send(e);
	    
	  }
	  break;
	case DATA:
	  {
	    s->nextLinkAvailableTime = max(s->nextLinkAvailableTime, tw_now(lp));
	    ts = s->nextLinkAvailableTime - tw_now(lp);
	    
	    s->nextLinkAvailableTime += msg->message_size/ION_FS_bw;
	    
	    e = tw_event_new( s->file_server_id, ts + msg->message_size/ION_FS_bw, lp);
	    m = tw_event_data(e);
	    m->type = ARRIVAL;
	    
	    // pass msg info
	    m->message_type = msg->message_type;
	    m->travel_start_time = msg->travel_start_time;
	    m->collective_msg_tag = msg->collective_msg_tag;
	    m->message_size = msg->message_size;
	  
	    tw_event_send(e);
	  }
	  break;
	}
      break;
    case PROCESS:
      switch( msg->message_type )
	{
	case ACK:
	  {
	    // need work here
	    ts = ION_packet_service_time;
	    e = tw_event_new( lp->gid, ts, lp );
	    m = tw_event_data(e);
	    m->type = SEND;
	    
	    m->message_type = msg->message_type;
	    m->travel_start_time = msg->travel_start_time;
	    m->collective_msg_tag = msg->collective_msg_tag;

	    tw_event_send(e);
	    
	  }
	  break;
	case DATA:
	  {
	    s->collective_round_counter++;
	    // accumulate payload
	    s->total_size += m->message_size;

	    printf("ION received DATA\n");
	    
	    // IO aggregation
	    if ( s->collective_round_counter == N_CN_per_ION*s->N_packet_round )
	      {
		s->collective_round_counter = 0;

		ts = ION_packet_service_time;
		e = tw_event_new( lp->gid, ts, lp );
		m = tw_event_data(e);
		m->type = SEND;
		
		m->message_type = msg->message_type;
	        
		m->travel_start_time = msg->travel_start_time;
		m->collective_msg_tag = msg->collective_msg_tag;

		m->message_size = s->total_size;
		s->total_size = 0;

		tw_event_send(e);

		printf("ION aggregated DATA message, tag is %d, travel time is %lf\n", 
		       msg->collective_msg_tag,
		       tw_now(lp) - msg->travel_start_time);
	      }
	  }
	  break;
	}
      break;
    }

}

void bgp_ion_eventHandler_rc( ION_state* s, tw_bf* bf, MsgData* m, tw_lp* lp )
{}

void bgp_ion_finish( ION_state* s, tw_lp* lp )
{}
