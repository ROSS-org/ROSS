/*
 *  Blue Gene/P model
 *  File Server
 *  by Ning Liu 
 */
#include "bgp.h"

void bgp_fs_init( FS_state* s,  tw_lp* lp )
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
  localID = localID - nlp_CN - nlp_ION;                                            
                                                                                   
  localID /= NumFSPerController; 
  s->controller_id = PEid * nlp_per_pe + nlp_CN + nlp_ION + nlp_FS + localID; 

  // get the IDs of the IONs which are hooked to this file server
  int i;                                                       
  s->previous_ION_id = (int *)calloc( N_ION_per_FS, sizeof(int) );
  int base = nlp_CN;                                 
  localID = lp->gid % nlp_per_pe;                              
  localID = localID - base - nlp_ION;                           
                                                               
 for (i=0; i<N_ION_per_FS; i++)                        
    s->previous_ION_id[i] = PEid * nlp_per_pe + base +          
      localID * N_ION_per_FS + i;                        

 s->MsgPrepTime = PVFS_handshake_time;

#ifdef PRINTid
 printf("FS LP %d speaking, my controller is %d \n", lp->gid, s->controller_id); 
 for (i=0; i<N_ION_per_FS; i++)                         
   printf("FS LP %d speaking, my ION is %d\n", lp->gid, s->previous_ION_id[i]);
#endif

}

void bgp_fs_eventHandler( FS_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  tw_event * e;
  tw_stime ts;
  MsgData * m;
  int i;

  switch(msg->type)
    {
    case IOrequest:

      ts = s->MsgPrepTime;

      e = tw_event_new( s->previous_ION_id[0], ts, lp );
      m = tw_event_data(e);
      m->type = IOrequest;
      m->MsgSrc = FileServer;
      
      m->message_type = ACK;
      m->travel_start_time = msg->travel_start_time;
      m->collective_msg_tag = msg->collective_msg_tag;                              

      tw_event_send(e);
      
      break;
    case GENERATE:
      // rest
      break;
    case ARRIVAL:
#ifdef TRACE
      printf("FS %d recieved data\n",lp->gid);
#endif
      printf("FS recieved ION data and traveil time is %lf\n",
	     tw_now(lp) - msg->travel_start_time );
      /*
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
      */
      break;
    case SEND:
      switch( msg->message_type )
	{
	case ACK:

#ifdef TRACE
	  printf("file server %d received ACK %d \n",
		 lp->gid,
		 msg->collective_msg_tag );
#endif
	  
	  for (i=0; i<N_ION_per_FS; i++)                                     
            {                            
	      ts = FS_packet_service_time;
              e = tw_event_new( s->previous_ION_id[i], ts, lp );
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
	  s->nextLinkAvailableTime = max(s->nextLinkAvailableTime, tw_now(lp));
	  ts = s->nextLinkAvailableTime - tw_now(lp);
	  
	  s->nextLinkAvailableTime += msg->message_size/FS_CON_bw;
	  
	  e = tw_event_new( s->controller_id, ts + msg->message_size/FS_CON_bw, lp );
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
      ts = FS_packet_service_time;
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

void bgp_fs_eventHandler_rc( FS_state* s, tw_bf* bf, MsgData* m, tw_lp* lp )
{}

void bgp_fs_finish( FS_state* s, tw_lp* lp )
{
  free(s->previous_ION_id);
}
