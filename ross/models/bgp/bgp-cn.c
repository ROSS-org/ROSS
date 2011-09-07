/*
 *  Blue Gene/P model
 *  Compute node
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

  N_ION_active = min (N_ION_active, nlp_ION*N_PE);

#ifdef PRINTid
  printf("CN %d local ID is %d next hop is %d and CN ID is %d\n", 
	 lp->gid,
	 s->CN_ID_in_tree,
	 s->tree_next_hop_id,
	 s->CN_ID );
#endif

  //CONT message stream
  // avoid 0 time stamp events

  // configure step
  // each CN tells root CN its lp ID and ID in CNs
  // root CN register all these 
  // sync up at this step

  s->bandwidth = 10000000000;
  s->compute_node = (int *)calloc(nlp_CN*N_PE, sizeof(int));
  s->time_stamp = (double *)calloc(N_checkpoint, sizeof(double));
  s->sync_counter = 0;
  s->checkpoint_counter = 0;
  s->write_counter = 0;

  ts = s->CN_ID_in_tree;

  e = tw_event_new( 0, ts, lp );
  m = tw_event_data(e);
  m->event_type = CONFIGURE;

  m->collective_group_rank = s->CN_ID;
  m->message_CN_source = lp->gid;

  tw_event_send(e);
}

void cn_configure( CN_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  tw_event * e; 
  tw_stime ts;
  MsgData * m;
  int i;

  s->sync_counter++;
  // register cn id
  s->compute_node[msg->collective_group_rank] = msg->message_CN_source;
  if( s->sync_counter==nlp_CN*tw_nnodes() )
    {
      s->sync_counter = 0;
      printf("Sync here 1 time!\n");

      for (i=0; i<nlp_CN*tw_nnodes(); i++)
	{
	  //printf("compute node %d lp id is %d\n",i,s->compute_node[i]);
	  // avoid 0 time step event
	  // mod avoid significant delay at high rank 
	  ts = i%nlp_CN;

	  e = tw_event_new( lp->gid, ts, lp );
	  m = tw_event_data(e);
	  m->event_type = SYNC;

	  tw_event_send(e);
	}
    }

}

void cn_sync( CN_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  tw_event * e; 
  tw_stime ts;
  MsgData * m;
  int i;

  s->sync_counter++;

  if( s->sync_counter==nlp_CN*tw_nnodes() )
    {
      s->sync_counter = 0;
      s->checkpoint_counter++;

      if( s->checkpoint_counter <= N_checkpoint )
	{
	  s->time_stamp[s->checkpoint_counter-1] = tw_now(lp);
	  printf("\n Sync here ****************  %d  ************* at %lf \n", 
		 s->checkpoint_counter,
		 tw_now(lp) );
	  // after computation time, send out another request
	  for (i=0; i<nlp_CN*tw_nnodes(); i++)
	    {
	      e = tw_event_new( s->compute_node[i], computation_time, lp );
	      m = tw_event_data(e);
	      m->event_type = APP_IO_REQUEST;
	      
	      tw_event_send(e);
	    }
	}
    }
 
}


void cn_checkpoint( CN_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  tw_event * e; 
  tw_stime ts;
  MsgData * m;
  int i;

  if ( s->bandwidth > msg->travel_start_time )
    s->bandwidth = msg->travel_start_time; 

  s->sync_counter++;

  //if( s->sync_counter==nlp_CN*tw_nnodes() )
  if( s->sync_counter==N_ION_active*N_CN_per_ION )
    {
      //#ifdef ALIGNED
      if (msg->io_type == WRITE_ALIGNED)
	printf("\n Observed write aligned %d K processes bandwidth is %lf GB/sec \n\n",
	       N_ION_active * N_CN_per_ION / 1024,
	       s->bandwidth );
      //#endif

      //#ifdef UNALIGNED
      if (msg->io_type == WRITE_UNALIGNED)
      	printf("\n Observed write unaligned %d K processes bandwidth is %lf GB/sec \n\n",
      	       N_ION_active * N_CN_per_ION / 1024,
      	       s->bandwidth );
      //#endif
      //#ifdef UNIQUE
      if (msg->io_type == WRITE_UNIQUE)
	printf("\n Observed write unique %d K processes bandwidth is %lf GB/sec \n\n",
	       N_ION_active * N_CN_per_ION / 1024,
	       s->bandwidth );
      //#endif
      //#ifdef ALIGNED
      if (msg->io_type == READ_ALIGNED)
	printf("\n Observed read aligned %d K processes bandwidth is %lf GB/sec \n\n",
	       N_ION_active * N_CN_per_ION / 1024,
	       s->bandwidth );
      //#endif

      //#ifdef UNALIGNED
      if (msg->io_type == READ_UNALIGNED)
      	printf("\n Observed read unaligned %d K processes bandwidth is %lf GB/sec \n\n",
      	       N_ION_active * N_CN_per_ION / 1024,
      	       s->bandwidth );
      //#endif
      //#ifdef UNIQUE
      if (msg->io_type == READ_UNIQUE)
	printf("\n Observed read unique %d K processes bandwidth is %lf GB/sec \n\n",
	       N_ION_active * N_CN_per_ION / 1024,
	       s->bandwidth );
      //#endif

      s->sync_counter = 0;
      s->checkpoint_counter++;

      // do N steps of checkpoint
      if( s->checkpoint_counter <= N_checkpoint )
	{
	  s->time_stamp[s->checkpoint_counter-1] = tw_now(lp);
	  printf("\n Sync here ****************  %d  ************* at %lf \n", 
		 s->checkpoint_counter,
		 tw_now(lp) );
	  // after computation time, send out another request
	  for (i=0; i<nlp_CN*tw_nnodes(); i++)
	    {
	      e = tw_event_new( s->compute_node[i], computation_time, lp );
	      m = tw_event_data(e);
	      m->event_type = APP_IO_REQUEST;
	      
	      tw_event_send(e);
	    }
	}
      s->bandwidth = 10000000000000;
    }
 
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

  m->travel_start_time = tw_now(lp) + ts;

  // aligned 
#ifdef ALIGNED
  m->io_offset = s->CN_ID * 16 * PVFS_payload_size;
  m->io_payload_size = 16 * PVFS_payload_size;

  m->collective_group_size = nlp_CN * N_PE;
  m->collective_group_rank = s->CN_ID;

  m->collective_master_node_id = 0;
  //m->io_type = WRITE_ALIGNED;
  m->io_type = READ_ALIGNED;
  m->io_tag = 12;
#endif

  // unaligned
#ifdef UNALIGNED
  m->io_offset = s->CN_ID * 16 * payload_size;
  m->io_payload_size = 16 * payload_size;

  m->collective_group_size = nlp_CN * N_PE;
  m->collective_group_rank = s->CN_ID;
  
  m->collective_master_node_id = 0;
  m->io_type = READ_UNALIGNED;
  //m->io_type = WRITE_UNALIGNED;
  m->io_tag = 12;
#endif

  // unique
#ifdef UNIQUE
  m->io_offset = 0;
  m->io_payload_size = 16 * PVFS_payload_size;

  m->collective_group_size = 1;
  m->collective_group_rank = 0;

  m->collective_master_node_id = 0;
  m->io_type = READ_UNIQUE;
  //m->io_type = WRITE_UNIQUE;
  //m->io_tag = 12;
  m->io_tag = tw_rand_integer(lp->rng,0,nlp_FS*N_PE);
#endif

  m->message_CN_source = lp->gid;

  tw_event_send(e);
}

void cn_close_ack( CN_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  tw_event * e;
  tw_stime ts;
  MsgData * m;
  int i;

  double virtual_delay;
  long long size;
  double payload = PVFS_payload_size;

#ifdef TRACE
      printf("close %d ACKed at CN travel time is %lf, IO tag is %d\n",
             msg->message_CN_source,
             tw_now(lp) - msg->travel_start_time,
             msg->io_tag);
#endif

/* #ifdef UNALIGNED */
/*       payload = payload_size; */
/* #endif */

      if ( msg->io_type==WRITE_UNALIGNED)
	payload = payload_size;

      if ( msg->io_type==READ_UNALIGNED)
	payload = payload_size;

      virtual_delay = (tw_now(lp) - msg->travel_start_time) ;
      size = N_ION_active*payload*16*N_CN_per_ION/1.024/1.024/1.024;
     
      /* //if (msg->message_CN_source == 0) */
      /* printf("Round %d, close %d ACKed by CN, travel time is %lf, bandwidth is %lf GB/s\n", */
      /* 	     s->checkpoint_counter, */
      /* 	     msg->message_CN_source, */
      /* 	     virtual_delay, */
      /* 	     size/virtual_delay); */
      
      // send sync signal to root 0
      e = tw_event_new( 0, 1, lp );
      m = tw_event_data(e);
      m->event_type = CHECKPOINT;

      // use travel_start_time (double) to
      // piggyback the bandwidth value
      m->travel_start_time = size/virtual_delay;

      m->io_type = msg->io_type;

      /* if (msg->io_type==WRITE_ALIGNED) */
      /* 	printf("It works ******* \n"); */

      //printf("Close ack here\n");

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
  
  switch(msg->io_type)
    {
    case READ_ALIGNED:
      ts = CN_CONT_msg_prep_time;
      
      e = tw_event_new( lp->gid, ts , lp );
      m = tw_event_data(e);
      m->event_type = READ_SEND;
      
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

      break;
    case READ_UNALIGNED:
      ts = CN_CONT_msg_prep_time;
      
      e = tw_event_new( lp->gid, ts , lp );
      m = tw_event_data(e);
      m->event_type = READ_SEND;
      
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

      break;
    case READ_UNIQUE:
      ts = CN_CONT_msg_prep_time;
      
      e = tw_event_new( lp->gid, ts , lp );
      m = tw_event_data(e);
      m->event_type = READ_SEND;
      
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

      break;
    default: // write requests

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
}

void cn_data_send( CN_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  tw_event * e; 
  tw_stime ts;
  MsgData * m;
  int i;
  double piece_size;

#ifdef TRACE
  printf("data %d start from CN %d travel time is %lf\n",
	 msg->message_CN_source,
	 s->CN_ID,
	 tw_now(lp) - msg->travel_start_time );
#endif

  //#ifdef ALIGNED
  if (msg->io_type==WRITE_ALIGNED)
    {
      piece_size = msg->io_payload_size / 16;

      for ( i=0; i<16; i++ )
	{
	  s->sender_next_available_time = max(s->sender_next_available_time, tw_now(lp));
	  ts = s->sender_next_available_time - tw_now(lp);

	  s->sender_next_available_time += piece_size/CN_out_bw;

	  e = tw_event_new( s->tree_next_hop_id, ts + piece_size/CN_out_bw, lp );
	  m = tw_event_data(e);
	  m->event_type = DATA_ARRIVE;

	  m->travel_start_time = msg->travel_start_time;

	  // change of piece size
	  m->io_offset = msg->io_offset + piece_size * i;
	  m->io_payload_size = piece_size;

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
    }
  //#endif

  //#ifdef UNALIGNED
  if(msg->io_type==WRITE_UNALIGNED)
    {
      piece_size = msg->io_payload_size / 16;

      for ( i=0; i<16; i++ )
	{
	  s->sender_next_available_time = max(s->sender_next_available_time, tw_now(lp));
	  ts = s->sender_next_available_time - tw_now(lp);

	  s->sender_next_available_time += piece_size/CN_out_bw;

	  e = tw_event_new( s->tree_next_hop_id, ts + piece_size/CN_out_bw, lp );
	  m = tw_event_data(e);
	  m->event_type = DATA_ARRIVE;

	  m->travel_start_time = msg->travel_start_time;

	  // change of piece size
	  m->io_offset = msg->io_offset + piece_size * i;
	  m->io_payload_size = piece_size;

	  m->collective_group_size = msg->collective_group_size;
	  m->collective_group_rank = msg->collective_group_rank;

	  m->collective_master_node_id = msg->collective_master_node_id;
	  m->io_type = msg->io_type;
	  //m->io_tag = i;
	  m->io_tag = msg->io_tag;

	  m->message_CN_source = msg->message_CN_source;
	  m->message_ION_source = msg->message_ION_source;
	  m->message_FS_source = msg->message_FS_source;

	  tw_event_send(e);
      
	}
    }
  //#endif

  //#ifdef UNIQUE
  if(msg->io_type==WRITE_UNIQUE)
    {
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
  //#endif
}

void cn_read_send( CN_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  tw_event * e; 
  tw_stime ts;
  MsgData * m;
  int i;
  double piece_size;

#ifdef TRACE
  printf("read %d start from CN %d travel time is %lf\n",
	 msg->message_CN_source,
	 s->CN_ID,
	 tw_now(lp) - msg->travel_start_time );
#endif

  if (msg->io_type==READ_ALIGNED)
    {
      piece_size = msg->io_payload_size / 16;

      for ( i=0; i<16; i++ )
	{
	  s->sender_next_available_time = max(s->sender_next_available_time, tw_now(lp));
	  ts = s->sender_next_available_time - tw_now(lp);

	  s->sender_next_available_time += CN_ION_meta_payload/CN_out_bw;

	  e = tw_event_new( s->tree_next_hop_id, ts + CN_ION_meta_payload/CN_out_bw, lp );
	  m = tw_event_data(e);
	  m->event_type = READ_ARRIVE;

	  m->travel_start_time = msg->travel_start_time;

	  // change of piece size
	  m->io_offset = msg->io_offset + piece_size * i;
	  m->io_payload_size = piece_size;

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
    }

  if(msg->io_type==READ_UNALIGNED)
    {
      piece_size = msg->io_payload_size / 16;

      for ( i=0; i<16; i++ )
	{
	  s->sender_next_available_time = max(s->sender_next_available_time, tw_now(lp));
	  ts = s->sender_next_available_time - tw_now(lp);

	  s->sender_next_available_time += CN_ION_meta_payload/CN_out_bw;

	  e = tw_event_new( s->tree_next_hop_id, ts + CN_ION_meta_payload/CN_out_bw, lp );
	  m = tw_event_data(e);
	  m->event_type = READ_ARRIVE;

	  m->travel_start_time = msg->travel_start_time;

	  // change of piece size
	  m->io_offset = msg->io_offset + piece_size * i;
	  m->io_payload_size = piece_size;

	  m->collective_group_size = msg->collective_group_size;
	  m->collective_group_rank = msg->collective_group_rank;

	  m->collective_master_node_id = msg->collective_master_node_id;
	  m->io_type = msg->io_type;
	  //m->io_tag = i;
	  m->io_tag = msg->io_tag;

	  m->message_CN_source = msg->message_CN_source;
	  m->message_ION_source = msg->message_ION_source;
	  m->message_FS_source = msg->message_FS_source;

	  tw_event_send(e);
      
	}
    }

  if(msg->io_type==READ_UNIQUE)
    {
      s->sender_next_available_time = max(s->sender_next_available_time, tw_now(lp));
      ts = s->sender_next_available_time - tw_now(lp);

      s->sender_next_available_time += CN_ION_meta_payload/CN_out_bw;

      e = tw_event_new( s->tree_next_hop_id, ts + CN_ION_meta_payload/CN_out_bw, lp );
      m = tw_event_data(e);
      m->event_type = READ_ARRIVE;

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

      //#ifdef ALIGNED
      if(msg->io_type==WRITE_ALIGNED)
	{
	  s->write_counter++;
	  if ( s->write_counter == 16 )
	    {
	      s->write_counter = 0;

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
	}
      //#endif

	//#ifdef UNALIGNED
      if(msg->io_type==WRITE_UNALIGNED)
	{
	  s->write_counter++;
	  if ( s->write_counter == 16 )
	    {
	      s->write_counter = 0;

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
	}
      //#endif

      //#ifdef UNIQUE
      if(msg->io_type==WRITE_UNIQUE)
	{
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
	//#endif
}

void cn_read_ack( CN_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  tw_event * e;
  tw_stime ts;
  MsgData * m;

#ifdef TRACE
      printf("read %d ACKed at CN travel time is %lf, IO tag is %d\n",
             msg->message_CN_source,
             tw_now(lp) - msg->travel_start_time,
             msg->io_tag);
#endif

      if(msg->io_type==READ_ALIGNED)
	{
	  s->write_counter++;
	  if ( s->write_counter == 16 )
	    {
	      s->write_counter = 0;

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
	}

      if(msg->io_type==READ_UNALIGNED)
	{
	  s->write_counter++;
	  if ( s->write_counter == 16 )
	    {
	      s->write_counter = 0;

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
	}

      if(msg->io_type==READ_UNIQUE)
	{
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
    case CONFIGURE:
      cn_configure( s, bf, msg, lp );
      break;
    case SYNC:
      cn_sync( s, bf, msg, lp );
      break;
    case CHECKPOINT:
      cn_checkpoint( s, bf, msg, lp );
      break;
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
    case READ_SEND:
      cn_read_send( s, bf, msg, lp );
      break;
    case DATA_ACK:
      cn_data_ack( s, bf, msg, lp );
      break;
    case READ_ACK:
      cn_read_ack( s, bf, msg, lp );
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
