/*
 *  Blue Gene/P model
 *  IO node
 *  by Ning Liu 
 */
#include "bgp.h"

void bgp_ion_init( ION_state* s,  tw_lp* lp )
{

  N_PE = tw_nnodes(); 

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

  s->myID_in_ION = localID + PEid*nlp_ION;
  // calculate the io node array  
  s->io_node = (int *)calloc(nlp_ION*N_PE, sizeof(int));
  int i, j, base;
  for ( i=0; i<N_PE; i++ )
    {
      base = i*nlp_per_pe;
      for ( j=0; j<nlp_ION; j++ )
	{
	  s->io_node[i*nlp_ION + j] = base + 
	    N_CN_per_ION*nlp_ION + j;
	}
    }

  // calculate the file server array  
  s->file_server = (int *)calloc(nlp_FS*N_PE, sizeof(int));
  for ( i=0; i<N_PE; i++ )
    {
      base = i*nlp_per_pe;
      for ( j=0; j<nlp_FS; j++ )
	{
	  s->file_server[i*nlp_FS + j] = base + 
	    N_CN_per_ION*nlp_ION +
	    nlp_ION + j;
	}
    }

  s->cn_sender_next_available_time = 0;  
  s->fs_sender_next_available_time = 0;  
  s->processor_next_available_time = 0;  
  s->cn_receiver_next_available_time = 0;
  s->fs_receiver_next_available_time = 0;
  s->msg_counter = 0;
  s->ack_counter = 0;
  s->close_counter = 0;
  s->close_flag = 0;

  s->cumulated_data_size = 0;

  s->buffer_pool_leftover = burst_buffer_size;
  for (i=0; i<N_sample; i++)
    {
      s->bb_record1[i] = 0;
      s->bb_record2[i] = 0;
      s->bb_record3[i] = 0;
    }

  for (i=0; i<16384; i++)
    {
      s->saved_message_size[i] = 0;
    }

  s->io_counter = (int *)calloc(256,sizeof(int));
  for (i=0;i<256;i++)
    s->io_counter = 0;
  
  N_FS_active = min(N_FS_active, nlp_FS*N_PE);

}

void ion_handshake_arrive( ION_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  tw_event * e;
  tw_stime ts;
  MsgData * m;

  double transmission_time = CN_ION_meta_payload/ION_CN_in_bw;

  if (s->myID_in_ION<N_ION_active)
    {

#ifdef TRACE
      printf("Handshake %d arrive at ION %d travel time is %lf\n",
	     msg->message_CN_source,
	     s->myID_in_ION,
	     tw_now(lp) - msg->travel_start_time );
#endif
      //printf("group id is %d and root is %d\n",msg->MC.group_id,msg->MC.Local_CN_root);

      s->cn_receiver_next_available_time = max(s->cn_receiver_next_available_time, tw_now(lp) - transmission_time);
      ts = s->cn_receiver_next_available_time - tw_now(lp) + transmission_time;

      s->cn_receiver_next_available_time += transmission_time;

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
      m->io_tag = msg->io_tag;
      m->message_CN_source = msg->message_CN_source;

      m->MC = msg->MC;
      tw_event_send(e);
    }

}

void ion_data_arrive( ION_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  tw_event * e;
  tw_stime ts;
  MsgData * m;

  double transmission_time = msg->io_payload_size/ION_CN_in_bw;

#ifdef TRACE
  printf("Data %d arrive at ION %d travel time is %lf IO tag is %d\n",
         msg->message_CN_source,
	 s->myID_in_ION,
         tw_now(lp) - msg->travel_start_time,
	 msg->io_tag);
#endif

  s->cn_receiver_next_available_time = max(s->cn_receiver_next_available_time, tw_now(lp) - transmission_time);
  ts = s->cn_receiver_next_available_time - tw_now(lp) + transmission_time;

  s->cn_receiver_next_available_time += transmission_time;

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
  m->io_tag = msg->io_tag;
  m->message_CN_source = msg->message_CN_source;
  m->message_ION_source = msg->message_ION_source;
  m->message_FS_source = msg->message_FS_source;

  m->MC = msg->MC;
  tw_event_send(e);

}

void ion_handshake_process( ION_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  tw_event * e;
  tw_stime ts;
  MsgData * m;

#ifdef TRACE
  printf("Handshake %d processed at ION %d travel time is %lf\n",
         msg->message_CN_source,
	 s->myID_in_ION,
         tw_now(lp) - msg->travel_start_time );
#endif

  ts = ION_CONT_msg_prep_time;
  e = tw_event_new( lp->gid, ts , lp );
  m = tw_event_data(e);
  m->event_type = LOOKUP_SEND;

  m->travel_start_time = msg->travel_start_time;

  m->io_offset = msg->io_offset;
  m->io_payload_size = msg->io_payload_size;
  m->collective_group_size = msg->collective_group_size;
  m->collective_group_rank = msg->collective_group_rank;

  m->collective_master_node_id = msg->collective_master_node_id;
  m->io_type = msg->io_type;
  m->io_tag = msg->io_tag;
  m->message_CN_source = msg->message_CN_source;

  m->MC = msg->MC;
  tw_event_send(e);

}

void ion_data_process( ION_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  tw_event * e;
  tw_stime ts;
  MsgData * m;

  int random_FS;
  int my_start_FS;

  if ( burst_buffer_on )
    {
      s->cumulated_data_size += msg->io_payload_size;

      random_FS = msg->io_tag % ( N_FS_active );
      my_start_FS = (int)(msg->io_offset / stripe_size);
      my_start_FS += random_FS;
      my_start_FS %= N_FS_active;
      
#ifdef TRACE
      printf("burst buffer on data %d processed at ION %d travel time is %lf\n",
	     msg->message_CN_source,
	     s->myID_in_ION,
	     tw_now(lp) - msg->travel_start_time );
#endif
      
      // two tracks, 1 if burst buffer is not filled, then go to data ack, the background
      // io traffic is still going, this background message will come back to data ack and release
      // the buffer, 2 if burst buffer is filled, go as normal messages
      int index = tw_now(lp)/timing_granuality;

      if( msg->io_payload_size < s->buffer_pool_leftover )//track 1
	{
	  // reserve the buffer space
	  s->buffer_pool_leftover -= msg->io_payload_size;

	  // record burst buffer usage
	  if (index < N_sample)
	    {
	      if (msg->MC.group_id == 1)
		s->bb_record1[index] = (burst_buffer_size - s->buffer_pool_leftover)/msg->MC.num_in_group*N_CN_per_ION;
	      else if (msg->MC.group_id == 2)
		s->bb_record2[index] = (burst_buffer_size - s->buffer_pool_leftover)/msg->MC.num_in_group*N_CN_per_ION;
	      else if (msg->MC.group_id == 3)
		s->bb_record3[index] = (burst_buffer_size - s->buffer_pool_leftover)/msg->MC.num_in_group*N_CN_per_ION;
	      else
		printf("Not recorded\n");
	    }

	  ts = msg->io_payload_size/bb_mem_copy_bw;
	  
	  e = tw_event_new( msg->message_CN_source, ts , lp );
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
	  m->message_ION_source = lp->gid;
	  m->message_FS_source = s->file_server[my_start_FS];
	  
	  m->MC = msg->MC;
	  m->MC.bb_flag = 0;
	  tw_event_send(e);

	  ts = ION_CONT_msg_prep_time;
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
	  m->io_tag = msg->io_tag;
	  m->message_CN_source = msg->message_CN_source;

	  m->MC = msg->MC;

	  m->MC.bb_flag = 1;
	  tw_event_send(e);

	}
      else// track 2
	{
	  if (index < N_sample)
	    {
	      if (msg->MC.group_id == 1)
		s->bb_record1[index] = (burst_buffer_size - s->buffer_pool_leftover)/msg->MC.num_in_group*N_CN_per_ION;
	      else if (msg->MC.group_id == 2)
		s->bb_record2[index] = (burst_buffer_size - s->buffer_pool_leftover)/msg->MC.num_in_group*N_CN_per_ION;
	      else if (msg->MC.group_id == 3)
		s->bb_record3[index] = (burst_buffer_size - s->buffer_pool_leftover)/msg->MC.num_in_group*N_CN_per_ION;
	      else
		printf("Not recorded\n");
	    }


	  ts = ION_CONT_msg_prep_time;
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
	  m->io_tag = msg->io_tag;
	  m->message_CN_source = msg->message_CN_source;

	  m->MC = msg->MC;

	  m->MC.bb_flag = 0;

	  tw_event_send(e);
	  
	}
    }
  else
    {
      
#ifdef TRACE
      printf("burst buffer off data %d processed at ION %d travel time is %lf\n",
	     msg->message_CN_source,
	     s->myID_in_ION,
	     tw_now(lp) - msg->travel_start_time );
#endif

      ts = ION_CONT_msg_prep_time;
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
      m->io_tag = msg->io_tag;
      m->message_CN_source = msg->message_CN_source;

      m->MC = msg->MC;
      tw_event_send(e);
  }
}

void ion_lookup_send( ION_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  tw_event * e;
  tw_stime ts;
  MsgData * m;

  //printf("group id is %d and root is %d\n",msg->MC.group_id,msg->MC.Local_CN_root);

  if (msg->collective_group_rank == msg->collective_master_node_id)
    {
      
#ifdef TRACE
  printf("Lookup %d send at ION %d travel time is %lf\n",
         msg->message_CN_source,
	 s->myID_in_ION,
         tw_now(lp) - msg->travel_start_time );
#endif
  s->fs_sender_next_available_time = max(s->fs_sender_next_available_time, tw_now(lp));
  ts = s->fs_sender_next_available_time - tw_now(lp);

  s->fs_sender_next_available_time += lookup_meta_size/ION_FS_out_bw;

  e = tw_event_new( s->file_server[0], ts + lookup_meta_size/ION_FS_out_bw, lp );
  m = tw_event_data(e);
  m->event_type = LOOKUP_ARRIVE;

  m->travel_start_time = msg->travel_start_time;

  m->io_offset = msg->io_offset;
  m->io_payload_size = msg->io_payload_size;
  m->collective_group_size = msg->collective_group_size;
  m->collective_group_rank = msg->collective_group_rank;

  m->collective_master_node_id = msg->collective_master_node_id;
  m->io_type = msg->io_type;

  m->message_ION_source = lp->gid;
  m->io_tag = msg->io_tag;
  m->message_CN_source = msg->message_CN_source;

  m->MC = msg->MC;
  tw_event_send(e);

    }
  else
    {
#ifdef TRACE
  printf("Lookup %d send at ION %d travel time is %lf\n",
         msg->message_CN_source,
	 s->myID_in_ION,
         tw_now(lp) - msg->travel_start_time );
#endif
/*   s->fs_sender_next_available_time = max(s->fs_sender_next_available_time, tw_now(lp)); */
/*   ts = s->fs_sender_next_available_time - tw_now(lp); */

/*   s->fs_sender_next_available_time += ION_FS_meta_payload/ION_FS_out_bw; */

  ts = ION_CONT_msg_prep_time;
  e = tw_event_new( lp->gid, ts , lp );
  m = tw_event_data(e);
  m->event_type = CREATE_END;

  m->travel_start_time = msg->travel_start_time;

  m->io_offset = msg->io_offset;
  m->io_payload_size = msg->io_payload_size;
  m->collective_group_size = msg->collective_group_size;
  m->collective_group_rank = msg->collective_group_rank;

  m->collective_master_node_id = msg->collective_master_node_id;
  m->io_type = msg->io_type;

  m->message_ION_source = lp->gid;
  m->io_tag = msg->io_tag;
  m->message_CN_source = msg->message_CN_source;

  m->MC = msg->MC;
  tw_event_send(e);
      
    }

}

void ion_handshake_send( ION_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  tw_event * e;
  tw_stime ts;
  MsgData * m;

  int random_FS;
  int my_start_FS;
  int i;
  int last_FS;

  long long first_block;
  long long last_block;

  int N_stripe;

  long long remainder;
  long long start_offset_in_stripe;

  s->msg_counter++;

  #ifdef TRACE
  printf("handshake %d send at ION %d travel time is %lf IO tag is %d and size is %lld\n",
         msg->message_CN_source,
	 s->myID_in_ION,
         tw_now(lp) - msg->travel_start_time,
	 msg->io_tag,
	 msg->io_payload_size);
  #endif

  // break data to stripes and 
  // random_FS is the start FS
  random_FS = msg->io_tag % ( N_FS_active );
  my_start_FS = (int)(msg->io_offset / stripe_size);
  my_start_FS += random_FS;
  my_start_FS %= N_FS_active;

  if ( (msg->io_payload_size % stripe_size)==0 )
    {
      N_stripe = msg->io_payload_size/stripe_size;

      // mark message
      s->piece_tag[s->msg_counter - 1] = N_stripe;

      for ( i=0; i<N_stripe; i++ )
	{
	  // 1 ---- N blocks
	  s->fs_sender_next_available_time = max(s->fs_sender_next_available_time, tw_now(lp));
	  ts = s->fs_sender_next_available_time - tw_now(lp);

	  s->fs_sender_next_available_time += handshake_payload_size/ION_FS_out_bw;

	  e = tw_event_new( s->file_server[( my_start_FS + i)%(N_FS_active)], ts + handshake_payload_size/ION_FS_out_bw, lp );
	  m = tw_event_data(e);
	  m->event_type = HANDSHAKE_ARRIVE;

	  m->travel_start_time = msg->travel_start_time;

	  m->io_offset = msg->io_offset;
	  // size change
	  m->io_payload_size = stripe_size;
	  m->collective_group_size = msg->collective_group_size;
	  m->collective_group_rank = msg->collective_group_rank;

	  m->collective_master_node_id = msg->collective_master_node_id;
	  m->io_type = msg->io_type;

	  m->message_ION_source = lp->gid;
	  m->io_tag = msg->io_tag;
	  m->message_CN_source = msg->message_CN_source;
	  m->message_ION_source = lp->gid;

	  /* m->IsLastPacket = 0; */

	  /* if ( i==N_stripe-1 ) */
	  /*   m->IsLastPacket = 1; */

	  m->IsLastPacket = s->msg_counter;

	  m->MC = msg->MC;
	  tw_event_send(e);

	}
    }
  else if ( msg->io_payload_size >= stripe_size )
    {

      start_offset_in_stripe = msg->io_offset % stripe_size;
      first_block = stripe_size - start_offset_in_stripe;
      // last block could be 0
      last_block = (msg->io_offset + msg->io_payload_size) % stripe_size;

      //printf("First block is %ld and Last block size is %ld \n",first_block,last_block);
      
      N_stripe = (int)((msg->io_payload_size - first_block - last_block)/stripe_size);

      // first block
      s->fs_sender_next_available_time = max(s->fs_sender_next_available_time, tw_now(lp));
      ts = s->fs_sender_next_available_time - tw_now(lp);

      s->fs_sender_next_available_time += handshake_payload_size/ION_FS_out_bw;

      e = tw_event_new( s->file_server[my_start_FS], ts + handshake_payload_size/ION_FS_out_bw, lp );
      m = tw_event_data(e);
      m->event_type = HANDSHAKE_ARRIVE;

      m->travel_start_time = msg->travel_start_time;

      m->io_offset = msg->io_offset;
      // size change
      m->io_payload_size = first_block;
      m->collective_group_size = msg->collective_group_size;
      m->collective_group_rank = msg->collective_group_rank;

      m->collective_master_node_id = msg->collective_master_node_id;
      m->io_type = msg->io_type;

      m->message_ION_source = lp->gid;
      m->io_tag = msg->io_tag;
      m->message_CN_source = msg->message_CN_source;
      m->message_ION_source = lp->gid;

      //m->IsLastPacket = 0;
      m->IsLastPacket = s->msg_counter;
      
      m->MC = msg->MC;
      tw_event_send(e);

      for ( i=0; i<N_stripe; i++ )
	{
	  // 2 ---- N-1 blocks
	  s->fs_sender_next_available_time = max(s->fs_sender_next_available_time, tw_now(lp));
	  ts = s->fs_sender_next_available_time - tw_now(lp);

	  s->fs_sender_next_available_time += handshake_payload_size/ION_FS_out_bw;

	  e = tw_event_new( s->file_server[( my_start_FS + i)%(N_FS_active)], ts + handshake_payload_size/ION_FS_out_bw, lp );
	  m = tw_event_data(e);
	  m->event_type = HANDSHAKE_ARRIVE;

	  m->travel_start_time = msg->travel_start_time;

	  m->io_offset = msg->io_offset;
	  // size change
	  m->io_payload_size = stripe_size;
	  m->collective_group_size = msg->collective_group_size;
	  m->collective_group_rank = msg->collective_group_rank;

	  m->collective_master_node_id = msg->collective_master_node_id;
	  m->io_type = msg->io_type;

	  m->message_ION_source = lp->gid;
	  m->io_tag = msg->io_tag;
	  m->message_CN_source = msg->message_CN_source;
	  m->message_ION_source = lp->gid;

	  //m->IsLastPacket = 0;
	  m->IsLastPacket = s->msg_counter;

	  m->MC = msg->MC;
	  tw_event_send(e);
	  
	}

      if (last_block > 0)
	{
	  last_FS = ( my_start_FS + N_stripe)%(N_FS_active);
	  s->piece_tag[s->msg_counter - 1] = N_stripe + 2;
	}
	else
	{
	  last_FS = ( my_start_FS + N_stripe - 1 + N_FS_active)%(N_FS_active);
	  s->piece_tag[s->msg_counter - 1] = N_stripe + 1;
	  //printf("Last fs %d is %d\n",last_FS,s->file_server[last_FS]);
	}

      // last block, if size = 0, serve as ghost message
      s->fs_sender_next_available_time = max(s->fs_sender_next_available_time, tw_now(lp));
      ts = s->fs_sender_next_available_time - tw_now(lp);

      s->fs_sender_next_available_time += handshake_payload_size/ION_FS_out_bw;

      e = tw_event_new( s->file_server[last_FS], ts + handshake_payload_size/ION_FS_out_bw, lp );
      m = tw_event_data(e);
      m->event_type = HANDSHAKE_ARRIVE;

      m->travel_start_time = msg->travel_start_time;

      m->io_offset = msg->io_offset;
      // size change
      m->io_payload_size = last_block;
      m->collective_group_size = msg->collective_group_size;
      m->collective_group_rank = msg->collective_group_rank;

      m->collective_master_node_id = msg->collective_master_node_id;
      m->io_type = msg->io_type;

      m->message_ION_source = lp->gid;
      m->io_tag = msg->io_tag;
      m->message_CN_source = msg->message_CN_source;
      m->message_ION_source = lp->gid;

      //m->IsLastPacket = 1;
      m->IsLastPacket = s->msg_counter;

      m->MC = msg->MC;
      tw_event_send(e);

      /* printf("first block is %lld and start FS is %d\n",first_block,my_start_FS); */
      /* for ( i=0; i<N_stripe; i++ ) */
      /* 	printf("middle random start FS is %d, my start is %d I am talking to %d\n", */
      /* 	       random_FS, */
      /* 	       my_start_FS, */
      /* 	       (  my_start_FS + 1 + i )%( N_FS_active ) ); */
      /* printf("last block is %lld and finish FS is %d\n",last_block, (my_start_FS+N_stripe+1)%(N_FS_active)); */

    }
  else
    {
      remainder = msg->io_offset % stripe_size;
      remainder = stripe_size - remainder;
      if ( msg->io_payload_size <= remainder )
	{
	  s->piece_tag[s->msg_counter - 1] = 1;
	  // enough space for current request
	  s->fs_sender_next_available_time = max(s->fs_sender_next_available_time, tw_now(lp));
	  ts = s->fs_sender_next_available_time - tw_now(lp);

	  s->fs_sender_next_available_time += handshake_payload_size/ION_FS_out_bw;

	  e = tw_event_new( s->file_server[my_start_FS], ts + handshake_payload_size/ION_FS_out_bw, lp );
	  m = tw_event_data(e);
	  m->event_type = HANDSHAKE_ARRIVE;

	  m->travel_start_time = msg->travel_start_time;

	  m->io_offset = msg->io_offset;
	  m->io_payload_size = msg->io_payload_size;
	  m->collective_group_size = msg->collective_group_size;
	  m->collective_group_rank = msg->collective_group_rank;

	  m->collective_master_node_id = msg->collective_master_node_id;
	  m->io_type = msg->io_type;

	  m->message_ION_source = lp->gid;
	  m->io_tag = msg->io_tag;
	  m->message_CN_source = msg->message_CN_source;
	  m->message_ION_source = lp->gid;

	  //m->IsLastPacket = 1;
	  m->IsLastPacket = s->msg_counter;

	  m->MC = msg->MC;
	  tw_event_send(e);
	  
	}
      else
	{
	  s->piece_tag[s->msg_counter - 1] = 2;
	  // need to talk to 2 file server
	  s->fs_sender_next_available_time = max(s->fs_sender_next_available_time, tw_now(lp));
	  ts = s->fs_sender_next_available_time - tw_now(lp);

	  s->fs_sender_next_available_time += handshake_payload_size/ION_FS_out_bw;

	  e = tw_event_new( s->file_server[my_start_FS], ts + handshake_payload_size/ION_FS_out_bw, lp );
	  m = tw_event_data(e);
	  m->event_type = HANDSHAKE_ARRIVE;

	  m->travel_start_time = msg->travel_start_time;

	  m->io_offset = msg->io_offset;
	  // first block size change
	  m->io_payload_size = remainder;
	  m->collective_group_size = msg->collective_group_size;
	  m->collective_group_rank = msg->collective_group_rank;

	  m->collective_master_node_id = msg->collective_master_node_id;
	  m->io_type = msg->io_type;

	  m->message_ION_source = lp->gid;
	  m->io_tag = msg->io_tag;
	  m->message_CN_source = msg->message_CN_source;
	  m->message_ION_source = lp->gid;

	  //m->IsLastPacket = 0;
	  m->IsLastPacket = s->msg_counter;

	  m->MC = msg->MC;
	  tw_event_send(e);

	  ////
	  s->fs_sender_next_available_time = max(s->fs_sender_next_available_time, tw_now(lp));
	  ts = s->fs_sender_next_available_time - tw_now(lp);

	  s->fs_sender_next_available_time += handshake_payload_size/ION_FS_out_bw;

	  e = tw_event_new( s->file_server[(my_start_FS + 1)%(N_FS_active)], ts + handshake_payload_size/ION_FS_out_bw, lp );
	  m = tw_event_data(e);
	  m->event_type = HANDSHAKE_ARRIVE;

	  m->travel_start_time = msg->travel_start_time;

	  m->io_offset = msg->io_offset;
	  // second block
	  m->io_payload_size = msg->io_payload_size - remainder;
	  m->collective_group_size = msg->collective_group_size;
	  m->collective_group_rank = msg->collective_group_rank;

	  m->collective_master_node_id = msg->collective_master_node_id;
	  m->io_type = msg->io_type;

	  m->message_ION_source = lp->gid;
	  m->io_tag = msg->io_tag;
	  m->message_CN_source = msg->message_CN_source;
	  m->message_ION_source = lp->gid;

	  //m->IsLastPacket = 1;
	  m->IsLastPacket = s->msg_counter;

	  m->MC = msg->MC;
	  tw_event_send(e);

	} 
    }
}

void ion_handshake_end( ION_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  tw_event * e;
  tw_stime ts;
  MsgData * m;

  double transmission_time = handshake_payload_size/ION_FS_in_bw;

#ifdef TRACE
  printf("handshake %d End at ION %d travel time is %lf\n",
         msg->message_CN_source,
	 s->myID_in_ION,
         tw_now(lp) - msg->travel_start_time );
#endif
  s->fs_receiver_next_available_time = max(s->fs_receiver_next_available_time, tw_now(lp) - transmission_time);
  ts = s->fs_receiver_next_available_time - tw_now(lp) + transmission_time;

  s->fs_receiver_next_available_time += transmission_time;

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

  m->message_ION_source = lp->gid;
  m->io_tag = msg->io_tag;
  m->message_CN_source = msg->message_CN_source;
  m->message_FS_source = msg->message_FS_source;
  m->message_ION_source = lp->gid;

  m->IsLastPacket = msg->IsLastPacket;

  m->MC = msg->MC;
  tw_event_send(e);

}


void ion_lookup_end( ION_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  tw_event * e;
  tw_stime ts;
  MsgData * m;

#ifdef TRACE
  printf("Lookup %d End at ION %d travel time is %lf\n",
         msg->message_CN_source,
	 s->myID_in_ION,
         tw_now(lp) - msg->travel_start_time );
#endif
  s->fs_receiver_next_available_time = max(s->fs_receiver_next_available_time, tw_now(lp));
  ts = s->fs_receiver_next_available_time - tw_now(lp);

  s->fs_receiver_next_available_time += ION_FS_meta_payload/ION_FS_in_bw;

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

  m->message_ION_source = lp->gid;
  m->io_tag = msg->io_tag;
  m->message_CN_source = msg->message_CN_source;

  m->MC = msg->MC;
  tw_event_send(e);

}

void ion_create_end( ION_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  tw_event * e;
  tw_stime ts;
  MsgData * m;

#ifdef TRACE
  printf("create %d End at ION %d travel time is %lf\n",
         msg->message_CN_source,
	 s->myID_in_ION,
         tw_now(lp) - msg->travel_start_time );
#endif
  ts = ION_CONT_msg_prep_time;
  e = tw_event_new( msg->message_CN_source, ts , lp );
  m = tw_event_data(e);
  m->event_type = HANDSHAKE_END;

  m->travel_start_time = msg->travel_start_time;

  m->io_offset = msg->io_offset;
  m->io_payload_size = msg->io_payload_size;
  m->collective_group_size = msg->collective_group_size;
  m->collective_group_rank = msg->collective_group_rank;

  m->collective_master_node_id = msg->collective_master_node_id;
  m->io_type = msg->io_type;

  m->message_ION_source = lp->gid;
  m->io_tag = msg->io_tag;
  m->message_CN_source = msg->message_CN_source;
  m->message_ION_source = msg->message_ION_source;
  m->message_FS_source = msg->message_FS_source;

  m->MC = msg->MC;
  tw_event_send(e);

}

void ion_create_send( ION_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  tw_event * e;
  tw_stime ts;
  MsgData * m;
  int random_FS;

#ifdef TRACE
  printf("Create %d send at ION %d travel time is %lf, IO tag is %d \n",
         msg->message_CN_source,
	 s->myID_in_ION,
         tw_now(lp) - msg->travel_start_time,
	 msg->io_tag );
#endif

  s->fs_sender_next_available_time = max(s->fs_sender_next_available_time, tw_now(lp));
  ts = s->fs_sender_next_available_time - tw_now(lp);

  s->fs_sender_next_available_time += ION_FS_meta_payload/ION_FS_out_bw;

  random_FS = msg->io_tag % ( N_FS_active ) ; 

  e = tw_event_new( s->file_server[random_FS], ts + ION_FS_meta_payload/ION_FS_out_bw, lp );
  m = tw_event_data(e);
  m->event_type = CREATE_ARRIVE;

  m->travel_start_time = msg->travel_start_time;

  m->io_offset = msg->io_offset;
  m->io_payload_size = msg->io_payload_size;
  m->collective_group_size = msg->collective_group_size;
  m->collective_group_rank = msg->collective_group_rank;

  m->collective_master_node_id = msg->collective_master_node_id;
  m->io_type = msg->io_type;

  m->message_ION_source = lp->gid;
  m->message_CN_source = msg->message_CN_source;
  m->io_tag = msg->io_tag;

  m->MC = msg->MC;
  tw_event_send(e);

}

void ion_data_send( ION_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  tw_event * e;
  tw_stime ts;
  MsgData * m;

  double transmission_time;
  /* if (msg->io_payload_size<4*1024) */
  /*   { */
  /*     transmission_time = msg->io_payload_size/ION_FS_out_bw*100; */
  /*   } */
  /* else if (msg->io_payload_size<8*1024) */
  /*   { */
  /*     transmission_time = msg->io_payload_size/ION_FS_out_bw*50; */
  /*   } */
  /* else if (msg->io_payload_size<16*1024) */
  /*   { */
  /*     transmission_time = msg->io_payload_size/ION_FS_out_bw*30; */
  /*   } */
  /* else if (msg->io_payload_size<32*1024) */
  /*   { */
  /*     transmission_time = msg->io_payload_size/ION_FS_out_bw*15; */
  /*   } */
  /* else if (msg->io_payload_size<64*1024) */
  /*   { */
  /*     transmission_time = msg->io_payload_size/ION_FS_out_bw*10; */
  /*   } */
  /* else if (msg->io_payload_size<128*1024) */
  /*   { */
  /*     transmission_time = msg->io_payload_size/ION_FS_out_bw*6; */
  /*   } */
  /* else if (msg->io_payload_size<256*1024) */
  /*   { */
  /*     transmission_time = msg->io_payload_size/ION_FS_out_bw*4; */
  /*   } */
  /* else if (msg->io_payload_size<1024*1024) */
  /*   { */
  /*     transmission_time = msg->io_payload_size/ION_FS_out_bw*1.5; */
  /*   } */
  /* else */
  /*   { */
  /*     transmission_time = msg->io_payload_size/ION_FS_out_bw*2; */
  /*   } */

  transmission_time = msg->io_payload_size/ION_FS_out_bw;

#ifdef TRACE
  printf(" Data %d send at ION %d travel time is %lf, IO tag is %d \n",
         msg->message_CN_source,
	 s->myID_in_ION,
         tw_now(lp) - msg->travel_start_time,
	 msg->io_tag );
#endif

  s->fs_sender_next_available_time = max(s->fs_sender_next_available_time, tw_now(lp));
  ts = s->fs_sender_next_available_time - tw_now(lp);

  s->fs_sender_next_available_time += msg->io_payload_size/ION_FS_out_bw;

  e = tw_event_new( msg->message_FS_source, ts + transmission_time, lp );
  m = tw_event_data(e);
  m->event_type = DATA_ARRIVE;

  m->travel_start_time = msg->travel_start_time;

  m->io_offset = msg->io_offset;
  m->io_payload_size = msg->io_payload_size;
  m->collective_group_size = msg->collective_group_size;
  m->collective_group_rank = msg->collective_group_rank;

  m->collective_master_node_id = msg->collective_master_node_id;
  m->io_type = msg->io_type;

  m->message_ION_source = lp->gid;
  m->io_tag = msg->io_tag;

  m->message_CN_source = msg->message_CN_source;
  m->message_FS_source = msg->message_FS_source;

  m->IsLastPacket = msg->IsLastPacket;

  m->MC = msg->MC;
  tw_event_send(e);

}

void ion_close_send( ION_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  tw_event * e;
  tw_stime ts;
  MsgData * m;

#ifdef TRACE
  printf(" close %d send at ION %d travel time is %lf, IO tag is %d \n",
         msg->message_CN_source,
	 s->myID_in_ION,
         tw_now(lp) - msg->travel_start_time,
	 msg->io_tag );
#endif

  s->fs_sender_next_available_time = max(s->fs_sender_next_available_time, tw_now(lp));
  ts = s->fs_sender_next_available_time - tw_now(lp);

  s->fs_sender_next_available_time += ION_FS_meta_payload/ION_FS_out_bw;

  e = tw_event_new( msg->message_FS_source, ts + ION_FS_meta_payload/ION_FS_out_bw, lp );
  m = tw_event_data(e);
  m->event_type = CLOSE_ARRIVE;

  m->travel_start_time = msg->travel_start_time;

  m->io_offset = msg->io_offset;
  m->io_payload_size = msg->io_payload_size;
  m->collective_group_size = msg->collective_group_size;
  m->collective_group_rank = msg->collective_group_rank;

  m->collective_master_node_id = msg->collective_master_node_id;
  m->io_type = msg->io_type;

  m->message_ION_source = lp->gid;
  m->io_tag = msg->io_tag;

  m->message_CN_source = msg->message_CN_source;
  m->message_FS_source = msg->message_FS_source;

  m->IsLastPacket = msg->IsLastPacket;

  m->MC = msg->MC;
  tw_event_send(e);

}

void ion_close_arrive( ION_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  tw_event * e;
  tw_stime ts;
  MsgData * m;

  double transmission_time = close_meta_size/ION_CN_in_bw;

#ifdef TRACE
  printf("close %d arrive at ION %d travel time is %lf IO tag is %d\n",
         msg->message_CN_source,
	 s->myID_in_ION,
         tw_now(lp) - msg->travel_start_time,
	 msg->io_tag);
#endif

  s->close_counter++;

  if ( s->close_counter == N_CN_per_ION)
    {

      s->close_counter = 0;

      s->cn_receiver_next_available_time = max(s->cn_receiver_next_available_time, tw_now(lp) - transmission_time);
      ts = s->cn_receiver_next_available_time - tw_now(lp) + transmission_time;

      s->cn_receiver_next_available_time += transmission_time;

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
      m->message_CN_source = msg->message_CN_source;
      m->message_ION_source = msg->message_ION_source;
      m->message_FS_source = msg->message_FS_source;

      m->MC = msg->MC;

      if (s->buffer_pool_leftover == burst_buffer_size)
	tw_event_send(e);
      else
	s->close_flag = 1;

    }

}


void ion_data_ack( ION_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  tw_event * e;
  tw_stime ts;
  MsgData * m;

  int i;
  double transmission_time = close_meta_size/ION_CN_in_bw;

  if( burst_buffer_on )
    {
      s->cumulated_data_size -= msg->io_payload_size;
      if (msg->MC.bb_flag == 1)
	{
	  s->buffer_pool_leftover += msg->io_payload_size;
	  if ((s->close_flag==1)&&(s->cumulated_data_size == 0))
	    {
	      // complete the finish request
	      s->cn_receiver_next_available_time = max(s->cn_receiver_next_available_time, tw_now(lp) - transmission_time);
	      ts = s->cn_receiver_next_available_time - tw_now(lp) + transmission_time;

	      s->cn_receiver_next_available_time += transmission_time;

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
	      m->message_CN_source = msg->message_CN_source;
	      m->message_ION_source = msg->message_ION_source;
	      m->message_FS_source = msg->message_FS_source;

	      m->MC = msg->MC;

	      tw_event_send(e);

	    }
	}
      else
	{
	  s->piece_tag[msg->IsLastPacket-1]--;
	  s->saved_message_size[msg->IsLastPacket-1] += msg->io_payload_size;

	  if (!s->piece_tag[msg->IsLastPacket-1])
	    {
	      ts = ION_CONT_msg_prep_time;

	      e = tw_event_new( msg->message_CN_source, ts , lp );
	      m = tw_event_data(e);
	      m->event_type = DATA_ACK;

	      m->travel_start_time = msg->travel_start_time;

	      m->io_offset = msg->io_offset;

	      //m->io_payload_size = msg->io_payload_size;
	      m->io_payload_size = s->saved_message_size[msg->IsLastPacket-1];

	      m->collective_group_size = msg->collective_group_size;
	      m->collective_group_rank = msg->collective_group_rank;

	      m->collective_master_node_id = msg->collective_master_node_id;
	      m->io_type = msg->io_type;
	      m->io_tag = msg->io_tag;
	      m->message_ION_source = lp->gid;
	      m->message_CN_source = msg->message_CN_source;
	      m->message_FS_source = msg->message_FS_source;

	      m->IsLastPacket = msg->IsLastPacket;

	      m->MC = msg->MC;
	      tw_event_send(e);
	    }
	}
    }
  else
    {
      s->piece_tag[msg->IsLastPacket-1]--;
      s->saved_message_size[msg->IsLastPacket-1] += msg->io_payload_size;

      if (!s->piece_tag[msg->IsLastPacket-1])
	{

	  ts = ION_CONT_msg_prep_time;

	  e = tw_event_new( msg->message_CN_source, ts , lp );
	  m = tw_event_data(e);
	  m->event_type = DATA_ACK;

	  m->travel_start_time = msg->travel_start_time;

	  m->io_offset = msg->io_offset;
	  //m->io_payload_size = msg->io_payload_size;
	  m->io_payload_size = s->saved_message_size[msg->IsLastPacket-1];
	  m->collective_group_size = msg->collective_group_size;
	  m->collective_group_rank = msg->collective_group_rank;

	  m->collective_master_node_id = msg->collective_master_node_id;
	  m->io_type = msg->io_type;
	  m->io_tag = msg->io_tag;
	  m->message_ION_source = lp->gid;
	  m->message_CN_source = msg->message_CN_source;
	  m->message_FS_source = msg->message_FS_source;

	  m->IsLastPacket = msg->IsLastPacket;

	  m->MC = msg->MC;
	  tw_event_send(e);
	}
    }
}

void ion_close_ack( ION_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  tw_event * e;
  tw_stime ts;
  MsgData * m;

#ifdef TRACE
      printf("close %d ACKed at ION travel time is %lf, IO tag is %d\n",
             msg->message_CN_source,
             tw_now(lp) - msg->travel_start_time,
             msg->io_tag);
#endif
      ts = ION_CONT_msg_prep_time;

      e = tw_event_new( msg->message_CN_source, ts , lp );
      m = tw_event_data(e);
      m->event_type = APP_CLOSE_DONE;

      m->travel_start_time = msg->travel_start_time;

      m->io_offset = msg->io_offset;
      m->io_payload_size = msg->io_payload_size;
      m->collective_group_size = msg->collective_group_size;
      m->collective_group_rank = msg->collective_group_rank;

      m->collective_master_node_id = msg->collective_master_node_id;
      m->io_type = msg->io_type;
      m->io_tag = msg->io_tag;
      m->message_ION_source = lp->gid;
      m->message_CN_source = msg->message_CN_source;
      m->message_FS_source = msg->message_FS_source;

      m->IsLastPacket = msg->IsLastPacket;

      m->MC = msg->MC;
      tw_event_send(e);

}


void bgp_ion_eventHandler( ION_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  tw_event * e;
  tw_stime ts;
  MsgData * m;

  switch( msg->event_type )
    {
    case HANDSHAKE_ARRIVE:
      ion_handshake_arrive(s, bf, msg, lp);
      break;      
    case DATA_ARRIVE:
      ion_data_arrive(s, bf, msg, lp);
      break;      
    case CLOSE_ARRIVE:
      ion_close_arrive(s, bf, msg, lp);
      break;      
    case HANDSHAKE_PROCESS:
      ion_handshake_process(s, bf, msg, lp);
      break;
    case DATA_PROCESS:
      ion_data_process(s, bf, msg, lp);
      break;
    case DATA_ACK:
      ion_data_ack(s, bf, msg, lp);
      break;
    case CLOSE_ACK:
      ion_close_ack(s, bf, msg, lp);
      break;
    case HANDSHAKE_SEND:
      ion_handshake_send(s, bf, msg, lp);
      break;
    case DATA_SEND:
      ion_data_send(s, bf, msg, lp);
      break;
    case CLOSE_SEND:
      ion_close_send(s, bf, msg, lp);
      break;
    case HANDSHAKE_END:
      ion_handshake_end(s, bf, msg, lp);
      break;
    case LOOKUP_SEND:
      ion_lookup_send(s, bf, msg, lp);
      break;
    case LOOKUP_END:
      ion_lookup_end(s, bf, msg, lp);
      break;      
    case CREATE_END:
      ion_create_end(s, bf, msg, lp);
      break;      
    case CREATE_SEND:
      ion_create_send(s, bf, msg, lp);
      break;      
    default:
      printf("Scheduled wrong events in IO Nodes %d\n", s->myID_in_ION);
    }
}

void bgp_ion_eventHandler_rc( ION_state* s, tw_bf* bf, MsgData* m, tw_lp* lp )
{
}

void bgp_ion_finish( ION_state* s, tw_lp* lp )
{
  int i;
  for (i=0; i<N_sample; i++)
    {
      BB_monitor1[i] += s->bb_record1[i];
      BB_monitor2[i] += s->bb_record2[i];
      BB_monitor3[i] += s->bb_record3[i];
    }

  free(s->file_server);
  free(s->io_node);
}
