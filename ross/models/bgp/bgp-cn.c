/*
 *  Blue Gene/P model
 *  Compute node / Tree Network
 *  by Ning Liu 
 */
#include "bgp.h"

void bgp_cn_init( CN_state* s,  tw_lp* lp )
{

  tw_event *e;
  tw_stime ts;
  MsgData *m;

  int N_PE = tw_nnodes();

  nlp_DDN = NumDDN / N_PE;
  nlp_Controller = nlp_DDN * NumControllerPerDDN;
  nlp_FS = nlp_Controller * NumFSPerController;
  nlp_ION = nlp_FS * N_ION_per_FS;
  nlp_CN = nlp_ION * N_CN_per_ION;

  int basePE = lp->gid/nlp_per_pe;
  // CN ID in the local process
  int localID = lp->gid - basePE * nlp_per_pe;
  int basePset = localID/N_CN_per_ION;
  s->CN_ID_in_tree = localID % N_CN_per_ION;

  s->CN_ID = basePE * nlp_CN + basePset * N_CN_per_ION + s->CN_ID_in_tree;
  
  s->tree_next_hop_id = basePE * nlp_per_pe + nlp_CN + basePset;
  s->sender_next_available_time = 0;

#ifdef PRINTid
  printf("CN %d local ID is %d next hop is %d and CN ID is %d\n", 
	 lp->gid,
	 s->CN_ID_in_tree,
	 s->tree_next_hop_id,
	 s->CN_ID );
#endif

  //CONT message stream
  // avoid 0 time stamp events
  ts = s->CN_ID_in_tree;
  e = tw_event_new( lp->gid, ts, lp );
  m = tw_event_data(e);
  m->event_type = APP_IO_REQUEST;

  m->travel_start_time = tw_now(lp) + ts;

  //m->io_offset = s->CN_ID * 16 * PVFS_payload_size;
  m->io_offset = 0;
  m->io_payload_size = 16 * PVFS_payload_size;
  // if collective size = 1  then unique
  // if collective size = full CN then collective
  //m->collective_group_size = nlp_CN * N_PE;
  m->collective_group_size = 1;
  //m->collective_group_rank = s->CN_ID;
  m->collective_group_rank = 0;
  m->collective_master_node_id = 0;

  // send out write reques
  //m->io_type = WRITE_INDIVIDUAL;
  m->io_type = WRITE_COLLECTIVE;
  //m->io_type = WRITE_UNALIGNED;
  //m->io_type = READ_COLLECTIVE;
  //m->io_type = READ_UNALIGNED;
  //m->io_type = READ_INDIVIDUAL;

  //m->io_tag = 12;
  m->io_tag = tw_rand_integer(lp->rng,0,nlp_FS*N_PE);;

  tw_event_send(e);
}

void cn_io_request( CN_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  tw_event * e; 
  tw_stime ts;
  MsgData * m;

#ifdef TRACE
  printf("Message %d start from CN %d travel time is %lf\n",
	 lp->gid,
	 s->CN_ID,
	 tw_now(lp) - msg->travel_start_time );
#endif

  ts = s->CN_ID_in_tree;
  e = tw_event_new( lp->gid, ts, lp );
  m = tw_event_data(e);
  m->event_type = HANDSHAKE_SEND;

  m->travel_start_time = msg->travel_start_time;

  m->io_offset = msg->io_offset;
  m->io_payload_size = msg->io_payload_size;
  m->collective_group_size = msg->collective_group_size;
  m->collective_group_rank = msg->collective_group_rank;

  m->collective_master_node_id = msg->collective_master_node_id;
  m->io_type = msg->io_type;
  m->io_tag = msg->io_tag;

  m->message_CN_source = lp->gid;

  tw_event_send(e);
}

void cn_handshake_send( CN_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  tw_event * e; 
  tw_stime ts;
  MsgData * m;

#ifdef TRACE
  printf("Handshake %d start from CN %d travel time is %lf\n",
	 msg->message_CN_source,
	 s->CN_ID,
	 tw_now(lp) - msg->travel_start_time );
#endif
  s->sender_next_available_time = max(s->sender_next_available_time, tw_now(lp));
  ts = s->sender_next_available_time - tw_now(lp);

  s->sender_next_available_time += CN_ION_meta_payload/CN_out_bw;

  e = tw_event_new( s->tree_next_hop_id, ts + CN_ION_meta_payload/CN_out_bw, lp );
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

  m->message_CN_source = msg->message_CN_source;

  tw_event_send(e);

}

void cn_handshake_end( CN_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  tw_event * e; 
  tw_stime ts;
  MsgData * m;

#ifdef TRACE
  printf("Handshake %d END from CN %d travel time is %lf\n open create end\n",
	 msg->message_CN_source,
	 s->CN_ID,
	 tw_now(lp) - msg->travel_start_time );
#endif
  ts = CN_CONT_msg_prep_time;

  e = tw_event_new( lp->gid, ts , lp );
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

  m->message_CN_source = msg->message_CN_source;
  m->message_ION_source = msg->message_ION_source;
  m->message_FS_source = msg->message_FS_source;

  tw_event_send(e);

}

void cn_data_send( CN_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  tw_event * e; 
  tw_stime ts;
  MsgData * m;

#ifdef TRACE
  printf("data %d start from CN %d travel time is %lf\n",
	 msg->message_CN_source,
	 s->CN_ID,
	 tw_now(lp) - msg->travel_start_time );
#endif
  s->sender_next_available_time = max(s->sender_next_available_time, tw_now(lp));
  ts = s->sender_next_available_time - tw_now(lp);

  s->sender_next_available_time += msg->io_payload_size/CN_out_bw;

  e = tw_event_new( s->tree_next_hop_id, ts + msg->io_payload_size/CN_out_bw, lp );
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

  m->message_CN_source = msg->message_CN_source;
  m->message_ION_source = msg->message_ION_source;
  m->message_FS_source = msg->message_FS_source;

  tw_event_send(e);

}

void cn_data_ack( CN_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  tw_event * e;
  tw_stime ts;
  MsgData * m;

#ifdef TRACE
      printf("data %d ACKed at CN travel time is %lf, IO tag is %d\n",
             msg->message_CN_source,
             tw_now(lp) - msg->travel_start_time,
             msg->io_tag);
#endif
      ts = ION_CONT_msg_prep_time;

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
      m->io_tag = msg->io_tag;
      m->message_ION_source = msg->message_ION_source;
      m->message_CN_source = lp->gid;
      m->message_FS_source = msg->message_FS_source;

      m->IsLastPacket = msg->IsLastPacket;

      tw_event_send(e);

}

void cn_close_ack( CN_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  tw_event * e;
  tw_stime ts;
  MsgData * m;

  double virtual_delay;
  long long size;

#ifdef TRACE
      printf("close %d ACKed at CN travel time is %lf, IO tag is %d\n",
             msg->message_CN_source,
             tw_now(lp) - msg->travel_start_time,
             msg->io_tag);
#endif

      virtual_delay = (tw_now(lp) - msg->travel_start_time)/1000000000 ;
      size = N_ION_active*PVFS_payload_size*16*N_CN_per_ION/1024/1024/1024;
     
      if (msg->message_CN_source%127 == 0)
	printf("close %d ACKed by CN, travel time is %lf, bandwidth is %lf GB/s\n",
	       msg->message_CN_source,
	       virtual_delay,
	       size/virtual_delay );
      

      /* ts = ION_CONT_msg_prep_time; */

      /* e = tw_event_new( lp->gid, ts , lp ); */
      /* m = tw_event_data(e); */
      /* m->event_type = CLOSE_SEND; */

      /* m->travel_start_time = msg->travel_start_time; */

      /* m->io_offset = msg->io_offset; */
      /* m->io_payload_size = msg->io_payload_size; */
      /* m->collective_group_size = msg->collective_group_size; */
      /* m->collective_group_rank = msg->collective_group_rank; */

      /* m->collective_master_node_id = msg->collective_master_node_id; */
      /* m->io_type = msg->io_type; */
      /* m->io_tag = msg->io_tag; */
      /* m->message_ION_source = msg->message_ION_source; */
      /* m->message_CN_source = lp->gid; */
      /* m->message_FS_source = msg->message_FS_source; */

      /* m->IsLastPacket = msg->IsLastPacket; */

      /* tw_event_send(e); */

}

void cn_close_send( CN_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  tw_event * e; 
  tw_stime ts;
  MsgData * m;

#ifdef TRACE
  printf("Close %d start from CN %d travel time is %lf\n",
	 msg->message_CN_source,
	 s->CN_ID,
	 tw_now(lp) - msg->travel_start_time );
#endif
  s->sender_next_available_time = max(s->sender_next_available_time, tw_now(lp));
  ts = s->sender_next_available_time - tw_now(lp);

  s->sender_next_available_time += CN_ION_meta_payload/CN_out_bw;

  e = tw_event_new( s->tree_next_hop_id, ts + CN_ION_meta_payload/CN_out_bw, lp );
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
  m->message_CN_source = lp->gid;
  m->message_FS_source = msg->message_FS_source;

  m->IsLastPacket = msg->IsLastPacket;

  tw_event_send(e);

}


void bgp_cn_eventHandler( CN_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  
  tw_event * e; 
  tw_stime ts;
  MsgData * m;
  int i;
  
  switch(msg->event_type)
    {
    case APP_IO_REQUEST:
      cn_io_request( s, bf, msg, lp );
      break;
    case HANDSHAKE_SEND:
      cn_handshake_send( s, bf, msg, lp );
      break;
    case HANDSHAKE_END:
      cn_handshake_end( s, bf, msg, lp );
      break;
    case DATA_SEND:
      cn_data_send( s, bf, msg, lp );
      break;
    case DATA_ACK:
      cn_data_ack( s, bf, msg, lp );
      break;
    case CLOSE_SEND:
      cn_close_send( s, bf, msg, lp );
      break;
    case CLOSE_ACK:
      cn_close_ack( s, bf, msg, lp );
      break;
    default:
      printf("Scheduled the wrong events in compute NODEs!\n");
    }
}

void bgp_cn_eventHandler_rc( CN_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{}

void bgp_cn_finish( CN_state* s, tw_lp* lp )
{}

int get_tree_next_hop( int TreeNextHopIndex )
{
  // tree structure is hidden here
  // only called in init phase
  // give me the index and return the next hop ID
  int TreeMatrix[32];

  TreeMatrix[0]=4;
  TreeMatrix[1]=17;
  TreeMatrix[2]=6;
  TreeMatrix[3]=7;
  TreeMatrix[4]=20;
  TreeMatrix[5]=1;
  TreeMatrix[6]=22;
  TreeMatrix[7]=-1;
  TreeMatrix[8]=24;
  TreeMatrix[9]=13;
  TreeMatrix[10]=26;
  TreeMatrix[11]=10;
  TreeMatrix[12]=28;
  TreeMatrix[13]=29;
  TreeMatrix[14]=10;
  TreeMatrix[15]=11;
  TreeMatrix[16]=20;
  TreeMatrix[17]=21;
  TreeMatrix[18]=2;
  TreeMatrix[19]=3;
  TreeMatrix[20]=21;
  TreeMatrix[21]=6;
  TreeMatrix[22]=7;
  TreeMatrix[23]=19;
  TreeMatrix[24]=25;
  TreeMatrix[25]=26;
  TreeMatrix[26]=22;
  TreeMatrix[27]=11;
  TreeMatrix[28]=24;
  TreeMatrix[29]=25;
  TreeMatrix[30]=14;
  TreeMatrix[31]=15;

  if ( TreeNextHopIndex == 39 )// BGP specific
    { return 35; }
  else if ( TreeNextHopIndex == 35 )// BGP specific
    return 51;
  else if ( TreeNextHopIndex == 51 )// BGP specific
    return 55;
  else if ( TreeNextHopIndex == 55 )// BGP specific
    return 23;
  else if ( TreeNextHopIndex > 31 )// BGP specific
    return TreeMatrix[TreeNextHopIndex-32] + 32;
  else
    return TreeMatrix[TreeNextHopIndex];

}


/* void cn_configure( CN_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp ) */
/* { */
/*   tw_event * e;  */
/*   tw_stime ts; */
/*   MsgData * m; */
/*   int i; */

/*   switch(msg->message_type) */
/*     { */
/*     case CONT: */
/*       { */
/* 	// when receive data message, record msg source */
/* 	if ( s->tree_previous_hop_id[0] == -1) */
/* 	  s->tree_previous_hop_id[0] = msg->msg_src_lp_id; */
/* 	else */
/* 	  s->tree_previous_hop_id[1] = msg->msg_src_lp_id; */
	
/* #ifdef TRACE  */
/* 	printf("CN %d received CONFIG CONT message\n",s->CN_ID_in_tree); */
/* 	for ( i=0; i<2; i++ ) */
/* 	  printf("CN %d previous hop [%d] is %d\n", */
/* 		 s->CN_ID_in_tree, */
/* 		 i, */
/* 		 s->tree_previous_hop_id[i]); */
/* #endif */

/* 	// configure the tree, identify previous hops */
/* 	ts = s->MsgPrepTime + lp->gid % 64; */
	    
/* 	// handshake: send back */
/* 	for ( i=0; i<2; i++ ) */
/* 	  { */
/* 	    if ( s->tree_previous_hop_id[i] >0 ) */
/* 	      { */
/* 		e = tw_event_new( s->tree_previous_hop_id[i], ts, lp ); */
/* 		m = tw_event_data(e); */
		    
/* 		m->type = CONFIG; */
/* 		m->msg_src_lp_id = lp->gid; */
/* 		m->travel_start_time = msg->travel_start_time; */
		    
/* 		m->collective_msg_tag = 12180; */
/* 		m->message_type = ACK; */
		    
/* 		tw_event_send(e); */
/* 	      } */
/* 	  } */
	    
/*       } */
/*       break; */
/*     case ACK: */
/* #ifdef TRACE */
/*       printf("CN %d received CONFIG ACK message, travel time is %lf\n", */
/* 	     s->CN_ID_in_tree, */
/* 	     tw_now(lp) - msg->travel_start_time ); */
	  
/* #endif */
/*       break; */
/*     } */
  
/* } */
