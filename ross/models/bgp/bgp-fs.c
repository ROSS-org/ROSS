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
                                                                                   
  // get the file server gid 
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

 s->ion_receiver_next_available_time = 0;
 s->ddn_receiver_next_available_time = 0;
 s->processor_next_available_time = 0;
 s->ion_sender_next_available_time = 0;
 s->ddn_sender_next_available_time = 0;

}

void fs_lookup_arrive( FS_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  tw_event * e;
  tw_stime ts;
  MsgData * m;

  double transmission_time = lookup_meta_size/FS_ION_in_bw;

#ifdef TRACE
  printf("Lookup %d arrive at FS 0 travel time is %lf\n",
	 msg->message_CN_source,
         tw_now(lp) - msg->travel_start_time );
#endif
  s->ion_receiver_next_available_time = max(s->ion_receiver_next_available_time, tw_now(lp) - transmission_time);
  ts = s->ion_receiver_next_available_time - tw_now(lp) + transmission_time;                                                                
  s->ion_receiver_next_available_time += transmission_time; 

  e = tw_event_new( lp->gid, ts , lp );
  m = tw_event_data(e);
  m->event_type = LOOKUP_PROCESS;

  m->travel_start_time = msg->travel_start_time;

  m->io_offset = msg->io_offset;
  m->io_payload_size = msg->io_payload_size;
  m->collective_group_size = msg->collective_group_size;
  m->collective_group_rank = msg->collective_group_rank;

  m->collective_master_node_id = msg->collective_master_node_id;
  m->io_type = msg->io_type;

  m->message_ION_source = msg->message_ION_source;
  m->message_CN_source = msg->message_CN_source;
  m->io_tag = msg->io_tag;

  m->MC = msg->MC;
  tw_event_send(e);
}

void fs_create_arrive( FS_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  tw_event * e;
  tw_stime ts;
  MsgData * m;

  double transmission_time = ION_FS_meta_payload/FS_ION_in_bw;

#ifdef TRACE
  printf("create %d arrive at FS travel time is %lf, IO tag is %d\n",
         msg->message_CN_source,
	 tw_now(lp) - msg->travel_start_time,
	 msg->io_tag);
#endif
  s->ion_receiver_next_available_time = max(s->ion_receiver_next_available_time, tw_now(lp) - transmission_time);
  ts = s->ion_receiver_next_available_time - tw_now(lp) + transmission_time;                                                                
  s->ion_receiver_next_available_time += transmission_time; 

  e = tw_event_new( lp->gid, ts , lp );
  m = tw_event_data(e);
  m->event_type = CREATE_PROCESS;

  m->travel_start_time = msg->travel_start_time;

  m->io_offset = msg->io_offset;
  m->io_payload_size = msg->io_payload_size;
  m->collective_group_size = msg->collective_group_size;
  m->collective_group_rank = msg->collective_group_rank;

  m->collective_master_node_id = msg->collective_master_node_id;
  m->io_type = msg->io_type;

  m->message_ION_source = msg->message_ION_source;
  m->message_CN_source = msg->message_CN_source;
  m->io_tag = msg->io_tag;

  m->MC = msg->MC;
  tw_event_send(e);
}

void fs_handshake_arrive( FS_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  tw_event * e;
  tw_stime ts;
  MsgData * m;

  double transmission_time = handshake_payload_size/FS_ION_in_bw*2;

#ifdef TRACE
  printf("handshake %d arrive at FS travel time is %lf, IO tag is %d\n",
         msg->message_CN_source,
	 tw_now(lp) - msg->travel_start_time,
	 msg->io_tag);
#endif
  s->ion_receiver_next_available_time = max(s->ion_receiver_next_available_time, tw_now(lp) - transmission_time);
  ts = s->ion_receiver_next_available_time - tw_now(lp) + transmission_time;                                                                
  s->ion_receiver_next_available_time += transmission_time; 

  e = tw_event_new( lp->gid, ts , lp );
  m = tw_event_data(e);
  m->event_type = HANDSHAKE_PROCESS;

  m->travel_start_time = msg->travel_start_time;

  m->io_offset = msg->io_offset;
  m->io_payload_size = msg->io_payload_size;
  m->collective_group_size = msg->collective_group_size;
  m->collective_group_rank = msg->collective_group_rank;

  m->collective_master_node_id = msg->collective_master_node_id;
  m->io_type = msg->io_type;

  m->message_ION_source = msg->message_ION_source;
  m->message_CN_source = msg->message_CN_source;
  m->io_tag = msg->io_tag;

  m->IsLastPacket = msg->IsLastPacket;

  m->MC = msg->MC;
  tw_event_send(e);
}

void fs_data_arrive( FS_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  tw_event * e;
  tw_stime ts;
  MsgData * m;

  double transmission_time;
  /* if (msg->io_payload_size<4*1024) */
  /*   { */
  /*     transmission_time = msg->io_payload_size/FS_ION_in_bw*100; */
  /*   } */
  /* else if (msg->io_payload_size<8*1024) */
  /*   { */
  /*     transmission_time = msg->io_payload_size/FS_ION_in_bw*50; */
  /*   } */
  /* else if (msg->io_payload_size<16*1024) */
  /*   { */
  /*     transmission_time = msg->io_payload_size/FS_ION_in_bw*30; */
  /*   } */
  /* else if (msg->io_payload_size<32*1024) */
  /*   { */
  /*     transmission_time = msg->io_payload_size/FS_ION_in_bw*15; */
  /*   } */
  /* else if (msg->io_payload_size<64*1024) */
  /*   { */
  /*     transmission_time = msg->io_payload_size/FS_ION_in_bw*10; */
  /*   } */
  /* else if (msg->io_payload_size<128*1024) */
  /*   { */
  /*     transmission_time = msg->io_payload_size/FS_ION_in_bw*6; */
  /*   } */
  /* else if (msg->io_payload_size<256*1024) */
  /*   { */
  /*     transmission_time = msg->io_payload_size/FS_ION_in_bw*4; */
  /*   } */
  /* else if (msg->io_payload_size<1024*1024) */
  /*   { */
  /*     transmission_time = msg->io_payload_size/FS_ION_in_bw*1.5; */
  /*   } */
  /* else */
  /*   { */
  /*     transmission_time = msg->io_payload_size/FS_ION_in_bw*1.1; */
  /*   } */
  transmission_time = msg->io_payload_size/FS_ION_in_bw;

#ifdef TRACE
  printf("data %d arrive at FS travel time is %lf, IO tag is %d\n",
         msg->message_CN_source,
	 tw_now(lp) - msg->travel_start_time,
	 msg->io_tag);
#endif
  s->ion_receiver_next_available_time = max(s->ion_receiver_next_available_time, tw_now(lp) - transmission_time);
  ts = s->ion_receiver_next_available_time - tw_now(lp) + transmission_time;                                                                
  s->ion_receiver_next_available_time += transmission_time; 

  e = tw_event_new( lp->gid, ts , lp );
  m = tw_event_data(e);
  m->event_type = DATA_PROCESS;

  m->travel_start_time = msg->travel_start_time;

  m->io_offset = msg->io_offset;
  m->io_payload_size = msg->io_payload_size;
  m->collective_group_size = msg->collective_group_size;
  m->collective_group_rank = msg->collective_group_rank;

  m->collective_master_node_id = msg->collective_master_node_id;
  m->io_type = msg->io_type;

  m->message_ION_source = msg->message_ION_source;
  m->message_CN_source = msg->message_CN_source;
  m->io_tag = msg->io_tag;

  m->IsLastPacket = msg->IsLastPacket;

  m->MC = msg->MC;
  tw_event_send(e);
}

void fs_close_arrive( FS_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  tw_event * e;
  tw_stime ts;
  MsgData * m;

  double transmission_time = close_meta_size/FS_ION_in_bw;

#ifdef TRACE
  printf("close %d arrive at FS travel time is %lf, IO tag is %d\n",
         msg->message_CN_source,
	 tw_now(lp) - msg->travel_start_time,
	 msg->io_tag);
#endif
  s->ion_receiver_next_available_time = max(s->ion_receiver_next_available_time, tw_now(lp) - transmission_time);
  ts = s->ion_receiver_next_available_time - tw_now(lp) + transmission_time;                                                                
  s->ion_receiver_next_available_time += transmission_time; 

  e = tw_event_new( lp->gid, ts , lp );
  m = tw_event_data(e);
  m->event_type = CLOSE_SEND;

  m->travel_start_time = msg->travel_start_time;

  m->io_offset = msg->io_offset;
  m->io_payload_size = msg->io_payload_size;
  m->collective_group_size = msg->collective_group_size;
  m->collective_group_rank = msg->collective_group_rank;

  m->collective_master_node_id = msg->collective_master_node_id;
  m->io_type = msg->io_type;

  m->message_ION_source = msg->message_ION_source;
  m->message_CN_source = msg->message_CN_source;
  m->message_FS_source = lp->gid;
  m->io_tag = msg->io_tag;

  m->IsLastPacket = msg->IsLastPacket;

  m->MC = msg->MC;
  tw_event_send(e);
}


void fs_lookup_process( FS_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  tw_event * e;
  tw_stime ts;
  MsgData * m;

#ifdef TRACE
  printf("Lookup %d process at FS 0 travel time is %lf\n",
         msg->message_CN_source,
	 tw_now(lp) - msg->travel_start_time );
#endif

  ts = FS_CONT_msg_prep_time;
  e = tw_event_new( lp->gid, ts , lp );
  m = tw_event_data(e);
  m->event_type = LOOKUP_ACK;

  m->travel_start_time = msg->travel_start_time;

  m->io_offset = msg->io_offset;
  m->io_payload_size = msg->io_payload_size;
  m->collective_group_size = msg->collective_group_size;
  m->collective_group_rank = msg->collective_group_rank;

  m->collective_master_node_id = msg->collective_master_node_id;
  m->io_type = msg->io_type;

  m->message_ION_source = msg->message_ION_source;
  m->message_CN_source = msg->message_CN_source;
  m->io_tag = msg->io_tag;

  m->MC = msg->MC;
  tw_event_send(e);

}

void fs_create_process( FS_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  tw_event * e;
  tw_stime ts;
  MsgData * m;

#ifdef TRACE
  printf("create %d process at FS travel time is %lf\n",
         msg->message_CN_source,
	 tw_now(lp) - msg->travel_start_time );
#endif

  ts = FS_CONT_msg_prep_time;
  e = tw_event_new( lp->gid, ts , lp );
  m = tw_event_data(e);
  m->event_type = CREATE_SEND;

  m->travel_start_time = msg->travel_start_time;

  m->io_offset = msg->io_offset;
  m->io_payload_size = msg->io_payload_size;
  m->collective_group_size = msg->collective_group_size;
  m->collective_group_rank = msg->collective_group_rank;

  m->collective_master_node_id = msg->collective_master_node_id;
  m->io_type = msg->io_type;

  m->message_ION_source = msg->message_ION_source;
  m->message_CN_source = msg->message_CN_source;
  m->message_FS_source = msg->message_FS_source;
  m->io_tag = msg->io_tag;

  m->MC = msg->MC;
  tw_event_send(e);

}

void fs_handshake_process( FS_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  tw_event * e;
  tw_stime ts;
  MsgData * m;

#ifdef TRACE
  printf("handshake %d process at FS travel time is %lf\n",
         msg->message_CN_source,
	 tw_now(lp) - msg->travel_start_time );
#endif

  ts = FS_CONT_msg_prep_time;
  e = tw_event_new( lp->gid, ts , lp );
  m = tw_event_data(e);
  m->event_type = HANDSHAKE_ACK;

  m->travel_start_time = msg->travel_start_time;

  m->io_offset = msg->io_offset;
  m->io_payload_size = msg->io_payload_size;
  m->collective_group_size = msg->collective_group_size;
  m->collective_group_rank = msg->collective_group_rank;

  m->collective_master_node_id = msg->collective_master_node_id;
  m->io_type = msg->io_type;

  m->message_ION_source = msg->message_ION_source;
  m->message_CN_source = msg->message_CN_source;
  m->message_FS_source = msg->message_FS_source;
  m->io_tag = msg->io_tag;

  m->IsLastPacket = msg->IsLastPacket;

  m->MC = msg->MC;
  tw_event_send(e);

}

void fs_data_process( FS_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  tw_event * e;
  tw_stime ts;
  MsgData * m;

#ifdef TRACE
  printf("data %d process at FS travel time is %lf\n",
         msg->message_CN_source,
	 tw_now(lp) - msg->travel_start_time );
#endif

  ts = FS_CONT_msg_prep_time;
  e = tw_event_new( lp->gid, ts , lp );
  m = tw_event_data(e);
  m->event_type = HANDSHAKE_SEND;

  m->travel_start_time = msg->travel_start_time;

  m->io_offset = msg->io_offset;
  m->io_payload_size = msg->io_payload_size;
  m->collective_group_size = msg->collective_group_size;
  m->collective_group_rank = msg->collective_group_rank;

  m->collective_master_node_id = msg->collective_master_node_id;
  m->io_type = msg->io_type;

  m->message_ION_source = msg->message_ION_source;
  m->message_CN_source = msg->message_CN_source;
  m->message_FS_source = lp->gid;
  m->io_tag = msg->io_tag;

  m->IsLastPacket = msg->IsLastPacket;

  m->MC = msg->MC;
  tw_event_send(e);

}


void fs_lookup_ack( FS_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  tw_event * e;
  tw_stime ts;
  MsgData * m;

#ifdef TRACE
  printf("Lookup %d ack at FS 0 travel time is %lf\n",
         msg->message_CN_source,
	 tw_now(lp) - msg->travel_start_time );
#endif

  ts = FS_CONT_msg_prep_time;
  e = tw_event_new( msg->message_ION_source, ts , lp );
  m = tw_event_data(e);
  m->event_type = LOOKUP_END;

  m->travel_start_time = msg->travel_start_time;

  m->io_offset = msg->io_offset;
  m->io_payload_size = msg->io_payload_size;
  m->collective_group_size = msg->collective_group_size;
  m->collective_group_rank = msg->collective_group_rank;

  m->collective_master_node_id = msg->collective_master_node_id;
  m->io_type = msg->io_type;
  m->io_tag = msg->io_tag;
  m->message_ION_source = msg->message_ION_source;
  m->message_CN_source = msg->message_CN_source;
  m->message_FS_source = msg->message_FS_source;

  m->MC = msg->MC;
  tw_event_send(e);

}

void fs_create_ack( FS_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  tw_event * e;
  tw_stime ts;
  MsgData * m;

  double transmission_time = DDN_ACK_size/FS_DDN_in_bw;

#ifdef TRACE
  printf("create %d ACKed at FS travel time is %lf, IO tag is %d\n",
         msg->message_CN_source,
	 tw_now(lp) - msg->travel_start_time,
	 msg->io_tag);
#endif
  s->ddn_receiver_next_available_time = max(s->ddn_receiver_next_available_time, tw_now(lp)) - transmission_time; 
  ts = s->ddn_receiver_next_available_time - tw_now(lp) + transmission_time;
  s->ddn_receiver_next_available_time += transmission_time; 

  e = tw_event_new( msg->message_ION_source, ts , lp );
  m = tw_event_data(e);
  m->event_type = CREATE_END;

  m->travel_start_time = msg->travel_start_time;

  m->io_offset = msg->io_offset;
  m->io_payload_size = msg->io_payload_size;
  m->collective_group_size = msg->collective_group_size;
  m->collective_group_rank = msg->collective_group_rank;

  m->collective_master_node_id = msg->collective_master_node_id;
  m->io_type = msg->io_type;
  m->io_tag = msg->io_tag;
  m->message_ION_source = msg->message_ION_source;
  m->message_CN_source = msg->message_CN_source;
  m->message_FS_source = msg->message_FS_source;

  m->MC = msg->MC;
  tw_event_send(e);

}


void fs_handshake_ack( FS_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  tw_event * e;
  tw_stime ts;
  MsgData * m;

#ifdef TRACE
  printf("handshake %d ACKed at FS travel time is %lf, IO tag is %d\n",
         msg->message_CN_source,
	 tw_now(lp) - msg->travel_start_time,
	 msg->io_tag);
#endif
  s->ion_sender_next_available_time = max(s->ion_sender_next_available_time, tw_now(lp)); 
  ts = s->ion_sender_next_available_time - tw_now(lp);
  s->ion_sender_next_available_time += ION_FS_meta_payload/FS_ION_out_bw; 

  e = tw_event_new( msg->message_ION_source, ts + ION_FS_meta_payload/FS_ION_out_bw, lp );
  m = tw_event_data(e);
  m->event_type = HANDSHAKE_END;

  m->travel_start_time = msg->travel_start_time;

  m->io_offset = msg->io_offset;
  m->io_payload_size = msg->io_payload_size;
  m->collective_group_size = msg->collective_group_size;
  m->collective_group_rank = msg->collective_group_rank;

  m->collective_master_node_id = msg->collective_master_node_id;
  m->io_type = msg->io_type;
  m->io_tag = msg->io_tag;
  m->message_ION_source = msg->message_ION_source;
  m->message_CN_source = msg->message_CN_source;
  m->message_FS_source = lp->gid;

  m->IsLastPacket = msg->IsLastPacket;

  m->MC = msg->MC;
  tw_event_send(e);

}

void fs_handshake_end( FS_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  tw_event * e;
  tw_stime ts;
  MsgData * m;

#ifdef TRACE
      printf("data %d ACKed at FS travel time is %lf, IO tag is %d\n",
	     msg->message_CN_source,
	     tw_now(lp) - msg->travel_start_time,
	     msg->io_tag);
#endif

      e = tw_event_new( lp->gid, FS_CONT_msg_prep_time, lp );
      m = tw_event_data(e);
      m->event_type = DATA_SEND;

      m->travel_start_time = msg->travel_start_time;

      m->io_offset = msg->io_offset;
      m->io_payload_size = msg->io_payload_size;
      m->collective_group_size = msg->collective_group_size;
      m->collective_group_rank = msg->collective_group_rank;

      m->collective_master_node_id = msg->collective_master_node_id;
      m->io_type = msg->io_type;
      m->io_tag = msg->io_tag;
      m->message_ION_source = msg->message_ION_source;
      m->message_CN_source = msg->message_CN_source;
      m->message_FS_source = lp->gid;

      m->IsLastPacket = msg->IsLastPacket;

      m->MC = msg->MC;
      tw_event_send(e);

}

void fs_data_ack( FS_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  tw_event * e;
  tw_stime ts;
  MsgData * m;

#ifdef TRACE
      printf("data %d ACKed at FS travel time is %lf, IO tag is %d\n",
	     msg->message_CN_source,
	     tw_now(lp) - msg->travel_start_time,
	     msg->io_tag);
#endif
      s->ion_sender_next_available_time = max(s->ion_sender_next_available_time, tw_now(lp)); 
      ts = s->ion_sender_next_available_time - tw_now(lp);
      s->ion_sender_next_available_time += ION_FS_meta_payload/FS_ION_out_bw; 

      //printf("ION source is %d\n",msg->message_ION_source);

      e = tw_event_new( msg->message_ION_source, ts + ION_FS_meta_payload/FS_ION_out_bw, lp );
      m = tw_event_data(e);
      m->event_type = DATA_ACK;

      m->travel_start_time = msg->travel_start_time;

      m->io_offset = msg->io_offset;
      m->io_payload_size = msg->io_payload_size;
      m->collective_group_size = msg->collective_group_size;
      m->collective_group_rank = msg->collective_group_rank;

      m->collective_master_node_id = msg->collective_master_node_id;
      m->io_type = msg->io_type;
      m->io_tag = msg->io_tag;
      m->message_ION_source = msg->message_ION_source;
      m->message_CN_source = msg->message_CN_source;
      m->message_FS_source = lp->gid;

      m->IsLastPacket = msg->IsLastPacket;

      m->MC = msg->MC;
      tw_event_send(e);

}

void fs_close_ack( FS_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  tw_event * e;
  tw_stime ts;
  MsgData * m;

#ifdef TRACE
      printf("close %d ACKed at FS travel time is %lf, IO tag is %d\n",
	     msg->message_CN_source,
	     tw_now(lp) - msg->travel_start_time,
	     msg->io_tag);
#endif
      s->ion_sender_next_available_time = max(s->ion_sender_next_available_time, tw_now(lp)); 
      ts = s->ion_sender_next_available_time - tw_now(lp);
      s->ion_sender_next_available_time += ION_FS_meta_payload/FS_ION_out_bw; 

      e = tw_event_new( msg->message_ION_source, ts + ION_FS_meta_payload/FS_ION_out_bw, lp );
      m = tw_event_data(e);
      m->event_type = CLOSE_ACK;

      m->travel_start_time = msg->travel_start_time;

      m->io_offset = msg->io_offset;
      m->io_payload_size = msg->io_payload_size;
      m->collective_group_size = msg->collective_group_size;
      m->collective_group_rank = msg->collective_group_rank;

      m->collective_master_node_id = msg->collective_master_node_id;
      m->io_type = msg->io_type;
      m->io_tag = msg->io_tag;
      m->message_ION_source = msg->message_ION_source;
      m->message_CN_source = msg->message_CN_source;
      m->message_FS_source = lp->gid;

      m->IsLastPacket = msg->IsLastPacket;

      m->MC = msg->MC;
      tw_event_send(e);

}

void fs_create_send( FS_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  tw_event * e;
  tw_stime ts;
  MsgData * m;

  double transmission_time;
  /* if (msg->io_payload_size<4*1024) */
  /*   { */
  /*     transmission_time = FS_DDN_meta_payload/FS_DDN_out_bw*100; */
  /*   } */
  /* else if (msg->io_payload_size<8*1024) */
  /*   { */
  /*     transmission_time = FS_DDN_meta_payload/FS_DDN_out_bw*50; */
  /*   } */
  /* else if (msg->io_payload_size<16*1024) */
  /*   { */
  /*     transmission_time = FS_DDN_meta_payload/FS_DDN_out_bw*30; */
  /*   } */
  /* else if (msg->io_payload_size<32*1024) */
  /*   { */
  /*     transmission_time = FS_DDN_meta_payload/FS_DDN_out_bw*15; */
  /*   } */
  /* else if (msg->io_payload_size<64*1024) */
  /*   { */
  /*     transmission_time = FS_DDN_meta_payload/FS_DDN_out_bw*10; */
  /*   } */
  /* else if (msg->io_payload_size<128*1024) */
  /*   { */
  /*     transmission_time = FS_DDN_meta_payload/FS_DDN_out_bw*6; */
  /*   } */
  /* else if (msg->io_payload_size<256*1024) */
  /*   { */
  /*     transmission_time = FS_DDN_meta_payload/FS_DDN_out_bw*4; */
  /*   } */
  /* else if (msg->io_payload_size<1024*1024) */
  /*   { */
  /*     transmission_time = FS_DDN_meta_payload/FS_DDN_out_bw*1.5; */
  /*   } */
  /* else  */
  /*   { */
  /*     transmission_time = FS_DDN_meta_payload/FS_DDN_out_bw*1.1; */
  /*   } */

  transmission_time = FS_DDN_meta_payload/FS_DDN_out_bw;

#ifdef TRACE
  printf("create %d send at FS travel time is %lf, IO tag is %d\n",
	 msg->message_CN_source,
	 tw_now(lp) - msg->travel_start_time,
	 msg->io_tag);
#endif

      s->ddn_sender_next_available_time = max(s->ddn_sender_next_available_time, tw_now(lp));                                                                      
      ts = s->ddn_sender_next_available_time - tw_now(lp);

      s->ddn_sender_next_available_time += transmission_time; 

      /* printf("***** fs ddn sender available time is %lf and ts is %lf trans time is %lf now is %lf\n", */
      /* 	     s->ddn_sender_next_available_time, */
      /* 	     ts, */
      /* 	     transmission_time, */
      /* 	     tw_now(lp)); */

      e = tw_event_new( s->controller_id, ts + transmission_time, lp );
      m = tw_event_data(e);
      m->event_type = CREATE_ARRIVE;

      m->travel_start_time = msg->travel_start_time;

      m->io_offset = msg->io_offset;
      m->io_payload_size = msg->io_payload_size;
      m->collective_group_size = msg->collective_group_size;
      m->collective_group_rank = msg->collective_group_rank;

      m->collective_master_node_id = msg->collective_master_node_id;
      m->io_type = msg->io_type;
      m->io_tag = msg->io_tag;
      m->message_ION_source = msg->message_ION_source;
      m->message_CN_source = msg->message_CN_source;
      m->message_FS_source = lp->gid;

      m->MC = msg->MC;
      tw_event_send(e);
    
/*   // working version from Argonne */
  
/* #ifdef PARA */

/*   if (msg->collective_group_rank == msg->collective_master_node_id) */
/*     { */
/*       //#ifdef TRACE */
/*       printf("create %d send at FS travel time is %lf, IO tag is %d\n", */
/* 	     msg->message_CN_source, */
/* 	     tw_now(lp) - msg->travel_start_time, */
/* 	     msg->io_tag); */
/*       //#endif */
/*       //printf("send once ******\n"); */
/*       s->ddn_sender_next_available_time = max(s->ddn_sender_next_available_time, tw_now(lp));    */
/*       ts = s->ddn_sender_next_available_time - tw_now(lp); */                                        
/*       s->ddn_sender_next_available_time += FS_DDN_meta_payload/FS_DDN_out_bw;  */

/*       e = tw_event_new( s->controller_id, ts + FS_DDN_meta_payload/FS_DDN_out_bw, lp ); */
/*       m = tw_event_data(e); */
/*       m->event_type = CREATE_ARRIVE; */

/*       m->travel_start_time = msg->travel_start_time; */

/*       m->io_offset = msg->io_offset; */
/*       m->io_payload_size = msg->io_payload_size; */
/*       m->collective_group_size = msg->collective_group_size; */
/*       m->collective_group_rank = msg->collective_group_rank; */

/*       m->collective_master_node_id = msg->collective_master_node_id; */
/*       m->io_type = msg->io_type; */
/*       m->io_tag = msg->io_tag; */
/*       m->message_ION_source = msg->message_ION_source; */
/*       m->message_CN_source = msg->message_CN_source; */
/*       m->message_FS_source = lp->gid; */

/*       tw_event_send(e); */
/*     } */
/*   else */
/*     { */
/*       //#ifdef TRACE */
/*       printf("create %d send to create ACK at FS travel time is %lf, IO tag is %d\n", */
/*              msg->message_CN_source, */
/*              tw_now(lp) - msg->travel_start_time, */
/*              msg->io_tag); */
/*       //#endif */
/*       //s->ddn_sender_next_available_time = max(s->ddn_sender_next_available_time, tw_now(lp)); */
/*       //ts = s->ddn_sender_next_available_time - tw_now(lp); */

/*       ts = close_meta_size/FS_DDN_out_bw + CONT_CONT_msg_prep_time; */

/*       e = tw_event_new( lp->gid, ts , lp ); */
/*       m = tw_event_data(e); */
/*       m->event_type = CREATE_ACK; */

/*       m->travel_start_time = msg->travel_start_time; */

/*       m->io_offset = msg->io_offset; */
/*       m->io_payload_size = msg->io_payload_size; */
/*       m->collective_group_size = msg->collective_group_size; */
/*       m->collective_group_rank = msg->collective_group_rank; */

/*       m->collective_master_node_id = msg->collective_master_node_id; */
/*       m->io_type = msg->io_type; */
/*       m->io_tag = msg->io_tag; */
/*       m->message_ION_source = msg->message_ION_source; */
/*       m->message_CN_source = msg->message_CN_source; */
/*       m->message_FS_source = lp->gid; */

/*       tw_event_send(e); */

/*     } */
/* #endif */

}

void fs_handshake_send( FS_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  tw_event * e;
  tw_stime ts = 1111;
  MsgData * m;

  double transmission_time;
  if (msg->io_payload_size<4*1024)
    {
      transmission_time = FS_DDN_meta_payload/FS_DDN_out_bw*100;
    }
  else if (msg->io_payload_size<8*1024)
    {
      transmission_time = FS_DDN_meta_payload/FS_DDN_out_bw*50;
    }
  else if (msg->io_payload_size<16*1024)
    {
      transmission_time = FS_DDN_meta_payload/FS_DDN_out_bw*30;
    }
  else if (msg->io_payload_size<32*1024)
    {
      transmission_time = FS_DDN_meta_payload/FS_DDN_out_bw*15;
    }
  else if (msg->io_payload_size<64*1024)
    {
      transmission_time = FS_DDN_meta_payload/FS_DDN_out_bw*10;
    }
  else if (msg->io_payload_size<128*1024)
    {
      transmission_time = FS_DDN_meta_payload/FS_DDN_out_bw*6;
    }
  else if (msg->io_payload_size<256*1024)
    {
      transmission_time = FS_DDN_meta_payload/FS_DDN_out_bw*4;
    }
  else if (msg->io_payload_size<1024*1024)
    {
      transmission_time = FS_DDN_meta_payload/FS_DDN_out_bw*1.5;
    }
  else 
    {
      transmission_time = FS_DDN_meta_payload/FS_DDN_out_bw*1.1;
    }


#ifdef TRACE
  printf("handshake %d send at FS travel time is %lf, IO tag is %d\n",
         msg->message_CN_source,
	 tw_now(lp) - msg->travel_start_time,
	 msg->io_tag);
#endif
  s->ddn_sender_next_available_time = max(transmission_time, tw_now(lp));                                                                      
  ts = s->ddn_sender_next_available_time - tw_now(lp);                                                                                                                       
  s->ddn_sender_next_available_time += transmission_time; 

  e = tw_event_new( s->controller_id, ts + transmission_time, lp );
  m = tw_event_data(e);
  m->event_type = HANDSHAKE_ARRIVE;

  m->travel_start_time = msg->travel_start_time;

  m->io_offset = msg->io_offset;
  m->io_payload_size = msg->io_payload_size;
  m->collective_group_size = msg->collective_group_size;
  m->collective_group_rank = msg->collective_group_rank;

  m->collective_master_node_id = msg->collective_master_node_id;
  m->io_type = msg->io_type;
  m->io_tag = msg->io_tag;
  m->message_ION_source = msg->message_ION_source;
  m->message_CN_source = msg->message_CN_source;
  m->message_FS_source = lp->gid;

  m->IsLastPacket = msg->IsLastPacket;

  m->MC = msg->MC;
  tw_event_send(e);

}


void fs_data_send( FS_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  tw_event * e;
  tw_stime ts;
  MsgData * m;

  double transmission_time;
  if (msg->io_payload_size<4*1024)
    {
      transmission_time = msg->io_payload_size/FS_DDN_out_bw*100;
    }
  else if (msg->io_payload_size<8*1024)
    {
      transmission_time = msg->io_payload_size/FS_DDN_out_bw*50;
    }
  else if (msg->io_payload_size<16*1024)
    {
      transmission_time = msg->io_payload_size/FS_DDN_out_bw*30;
    }
  else if (msg->io_payload_size<32*1024)
    {
      transmission_time = msg->io_payload_size/FS_DDN_out_bw*15;
    }
  else if (msg->io_payload_size<64*1024)
    {
      transmission_time = msg->io_payload_size/FS_DDN_out_bw*10;
    }
  else if (msg->io_payload_size<128*1024)
    {
      transmission_time = msg->io_payload_size/FS_DDN_out_bw*6;
    }
  else if (msg->io_payload_size<256*1024)
    {
      transmission_time = msg->io_payload_size/FS_DDN_out_bw*4;
    }
  else if (msg->io_payload_size<1024*1024)
    {
      transmission_time = msg->io_payload_size/FS_DDN_out_bw*1.5;
    }
  else 
    {
      transmission_time = msg->io_payload_size/FS_DDN_out_bw*1.1;
    }



#ifdef TRACE
  printf("data %d send at FS travel time is %lf, IO tag is %d\n",
         msg->message_CN_source,
	 tw_now(lp) - msg->travel_start_time,
	 msg->io_tag);
#endif
  s->ddn_sender_next_available_time = max(s->ddn_sender_next_available_time, tw_now(lp));                                                                      
  ts = s->ddn_sender_next_available_time - tw_now(lp);                                                                                                                       
  s->ddn_sender_next_available_time += transmission_time; 

  e = tw_event_new( s->controller_id, ts + transmission_time, lp );
  m = tw_event_data(e);
  m->event_type = DATA_ARRIVE;

  m->travel_start_time = msg->travel_start_time;

  m->io_offset = msg->io_offset;
  m->io_payload_size = msg->io_payload_size;
  m->collective_group_size = msg->collective_group_size;
  m->collective_group_rank = msg->collective_group_rank;

  m->collective_master_node_id = msg->collective_master_node_id;
  m->io_type = msg->io_type;
  m->io_tag = msg->io_tag;
  m->message_ION_source = msg->message_ION_source;
  m->message_CN_source = msg->message_CN_source;
  m->message_FS_source = lp->gid;

  m->IsLastPacket = msg->IsLastPacket;

  m->MC = msg->MC;
  tw_event_send(e);

}

void fs_close_send( FS_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  tw_event * e;
  tw_stime ts;
  MsgData * m;

  if ( msg->collective_group_rank == msg->collective_master_node_id )
    {
#ifdef TRACE
      printf("close %d send at FS travel time is %lf, IO tag is %d\n",
	     msg->message_CN_source,
	     tw_now(lp) - msg->travel_start_time,
	     msg->io_tag);
#endif
      s->ddn_sender_next_available_time = max(s->ddn_sender_next_available_time, tw_now(lp));
      ts = s->ddn_sender_next_available_time - tw_now(lp);

      s->ddn_sender_next_available_time += close_meta_size/FS_DDN_out_bw; 

      e = tw_event_new( s->controller_id, ts + close_meta_size/FS_DDN_out_bw, lp );
      m = tw_event_data(e);
      m->event_type = CLOSE_ARRIVE;

      m->travel_start_time = msg->travel_start_time;

      m->io_offset = msg->io_offset;
      m->io_payload_size = msg->io_payload_size;
      m->collective_group_size = msg->collective_group_size;
      m->collective_group_rank = msg->collective_group_rank;

      m->collective_master_node_id = msg->collective_master_node_id;
      m->io_type = msg->io_type;
      m->io_tag = msg->io_tag;
      m->message_ION_source = msg->message_ION_source;
      m->message_CN_source = msg->message_CN_source;
      m->message_FS_source = lp->gid;

      m->IsLastPacket = msg->IsLastPacket;

      m->MC = msg->MC;
      tw_event_send(e);
    }
  else
    {
#ifdef TRACE
      printf("close %d send to close ACK at FS travel time is %lf, IO tag is %d\n",
	     msg->message_CN_source,
	     tw_now(lp) - msg->travel_start_time,
	     msg->io_tag);
#endif
      //s->ddn_sender_next_available_time = max(s->ddn_sender_next_available_time, tw_now(lp));                                                                      
      //ts = s->ddn_sender_next_available_time - tw_now(lp);

      ts = close_meta_size/FS_DDN_out_bw; 

      e = tw_event_new( s->controller_id, ts , lp );
      m = tw_event_data(e);
      m->event_type = CLOSE_ACK;

      m->travel_start_time = msg->travel_start_time;

      m->io_offset = msg->io_offset;
      m->io_payload_size = msg->io_payload_size;
      m->collective_group_size = msg->collective_group_size;
      m->collective_group_rank = msg->collective_group_rank;

      m->collective_master_node_id = msg->collective_master_node_id;
      m->io_type = msg->io_type;
      m->io_tag = msg->io_tag;
      m->message_ION_source = msg->message_ION_source;
      m->message_CN_source = msg->message_CN_source;
      m->message_FS_source = lp->gid;

      m->IsLastPacket = msg->IsLastPacket;

      m->MC = msg->MC;
      tw_event_send(e);
      
    }

}


void bgp_fs_eventHandler( FS_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  
  switch(msg->event_type)
    {
    case LOOKUP_ARRIVE:
      fs_lookup_arrive(s, bf, msg, lp);
      break;
    case CREATE_ARRIVE:
      fs_create_arrive(s, bf, msg, lp);
      break;
    case HANDSHAKE_ARRIVE:
      fs_handshake_arrive(s, bf, msg, lp);
      break;
    case DATA_ARRIVE:
      fs_data_arrive(s, bf, msg, lp);
      break;
    case CLOSE_ARRIVE:
      fs_close_arrive(s, bf, msg, lp);
      break;
    case LOOKUP_PROCESS:
      fs_lookup_process(s, bf, msg, lp);
      break;
    case CREATE_PROCESS:
      fs_create_process(s, bf, msg, lp);
      break;
    case HANDSHAKE_PROCESS:
      fs_handshake_process(s, bf, msg, lp);
      break;
    case DATA_PROCESS:
      fs_data_process(s, bf, msg, lp);
      break;
    case HANDSHAKE_SEND:
      fs_handshake_send(s, bf, msg, lp);
      break;
    case CREATE_SEND:
      fs_create_send(s, bf, msg, lp);
      break;
    case DATA_SEND:
      fs_data_send(s, bf, msg, lp);
      break;
    case CLOSE_SEND:
      fs_close_send(s, bf, msg, lp);
      break;
    case LOOKUP_ACK:
      fs_lookup_ack(s, bf, msg, lp);
      break;
    case CREATE_ACK:
      fs_create_ack(s, bf, msg, lp);
      break;
    case HANDSHAKE_ACK:
      fs_handshake_ack(s, bf, msg, lp);
      break;
    case HANDSHAKE_END:
      fs_handshake_end(s, bf, msg, lp);
      break;
    case DATA_ACK:
      fs_data_ack(s, bf, msg, lp);
      break;
    case CLOSE_ACK:
      fs_close_ack(s, bf, msg, lp);
      break;
    default:
      printf("Scheduled wrong events on file server\n");
    }
}

void bgp_fs_eventHandler_rc( FS_state* s, tw_bf* bf, MsgData* m, tw_lp* lp )
{}

void bgp_fs_finish( FS_state* s, tw_lp* lp )
{}
