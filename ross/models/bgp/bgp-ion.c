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

  s->io_counter = (int *)calloc(256,sizeof(int));
  for (i=0;i<256;i++)
    s->io_counter = 0;
  
}

void ion_handshake_arrive( ION_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  tw_event * e;
  tw_stime ts;
  MsgData * m;

  if (s->myID_in_ION<N_ION_active)
    {

#ifdef TRACE
  printf("Handshake %d arrive at ION %d travel time is %lf\n",
         msg->message_CN_source,
	 s->myID_in_ION,
         tw_now(lp) - msg->travel_start_time );
#endif

  s->cn_receiver_next_available_time = max(s->cn_receiver_next_available_time, tw_now(lp));
  ts = s->cn_receiver_next_available_time - tw_now(lp);

  s->cn_receiver_next_available_time += CN_ION_meta_payload/ION_CN_in_bw;

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

  tw_event_send(e);
    }

}

void ion_data_arrive( ION_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  tw_event * e;
  tw_stime ts;
  MsgData * m;

#ifdef TRACE
  printf("Data %d arrive at ION %d travel time is %lf IO tag is %d\n",
         msg->message_CN_source,
	 s->myID_in_ION,
         tw_now(lp) - msg->travel_start_time,
	 msg->io_tag);
#endif

  s->cn_receiver_next_available_time = max(s->cn_receiver_next_available_time, tw_now(lp));
  ts = s->cn_receiver_next_available_time - tw_now(lp);

  s->cn_receiver_next_available_time += msg->io_payload_size/ION_CN_in_bw;

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

  tw_event_send(e);

}

void ion_data_process( ION_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  tw_event * e;
  tw_stime ts;
  MsgData * m;

#ifdef TRACE
  printf("data %d processed at ION %d travel time is %lf\n",
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

  tw_event_send(e);

}

void ion_lookup_send( ION_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  tw_event * e;
  tw_stime ts;
  MsgData * m;

#ifdef TRACE
  printf("Lookup %d send at ION %d travel time is %lf\n",
         msg->message_CN_source,
	 s->myID_in_ION,
         tw_now(lp) - msg->travel_start_time );
#endif
  s->fs_sender_next_available_time = max(s->fs_sender_next_available_time, tw_now(lp));
  ts = s->fs_sender_next_available_time - tw_now(lp);

  s->fs_sender_next_available_time += ION_FS_meta_payload/ION_FS_out_bw;

  e = tw_event_new( s->file_server[0], ts + ION_FS_meta_payload/ION_FS_out_bw, lp );
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

  tw_event_send(e);

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

#ifdef TRACE
  printf("handshake %d send at ION %d travel time is %lf IO tag is %d \n",
         msg->message_CN_source,
	 s->myID_in_ION,
         tw_now(lp) - msg->travel_start_time,
	 msg->io_tag);
#endif
  // break data to stripes and 
  // random_FS is the start FS
  random_FS = msg->io_tag % ( nlp_FS*N_PE );
  my_start_FS = (int)(msg->io_offset / stripe_size);
  my_start_FS += random_FS;
  my_start_FS %= nlp_FS*N_PE;

  if ( msg->io_payload_size >= stripe_size )
    {

      start_offset_in_stripe = msg->io_offset % stripe_size;
      first_block = stripe_size - start_offset_in_stripe;
      // last block could be 0
      last_block = (msg->io_offset + msg->io_payload_size) % stripe_size;
      
      N_stripe = (int)((msg->io_payload_size - first_block - last_block)/stripe_size);

      // first block
      s->fs_sender_next_available_time = max(s->fs_sender_next_available_time, tw_now(lp));
      ts = s->fs_sender_next_available_time - tw_now(lp);

      s->fs_sender_next_available_time += ION_FS_meta_payload/ION_FS_out_bw;

      e = tw_event_new( s->file_server[my_start_FS], ts + ION_FS_meta_payload/ION_FS_out_bw, lp );
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

      m->IsLastPacket = 0;

      tw_event_send(e);


      for ( i=0; i<N_stripe; i++ )
	{
	  // 2 ---- N-1 blocks
	  s->fs_sender_next_available_time = max(s->fs_sender_next_available_time, tw_now(lp));
	  ts = s->fs_sender_next_available_time - tw_now(lp);

	  s->fs_sender_next_available_time += ION_FS_meta_payload/ION_FS_out_bw;

	  e = tw_event_new( s->file_server[( my_start_FS + i)%(nlp_FS*N_PE)], ts + ION_FS_meta_payload/ION_FS_out_bw, lp );
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

	  m->IsLastPacket = 0;

	  tw_event_send(e);
	  
	}

      if (last_block > 0)
	last_FS = ( my_start_FS + N_stripe)%(nlp_FS*N_PE);
      else
	last_FS = ( my_start_FS + N_stripe - 1)%(nlp_FS*N_PE);

      // last block, if size = 0, serve as ghost message
      s->fs_sender_next_available_time = max(s->fs_sender_next_available_time, tw_now(lp));
      ts = s->fs_sender_next_available_time - tw_now(lp);

      s->fs_sender_next_available_time += ION_FS_meta_payload/ION_FS_out_bw;

      e = tw_event_new( s->file_server[last_FS], ts + ION_FS_meta_payload/ION_FS_out_bw, lp );
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

      m->IsLastPacket = 1;

      tw_event_send(e);

      /* printf("first block is %lld and start FS is %d\n",first_block,my_start_FS); */
      /* for ( i=0; i<N_stripe; i++ ) */
      /* 	printf("middle random start FS is %d, my start is %d I am talking to %d\n", */
      /* 	       random_FS, */
      /* 	       my_start_FS, */
      /* 	       (  my_start_FS + 1 + i )%( nlp_FS*N_PE ) ); */
      /* printf("last block is %lld and finish FS is %d\n",last_block, (my_start_FS+N_stripe+1)%(nlp_FS*N_PE)); */

    }
  else
    {
      remainder = msg->io_offset % stripe_size;
      remainder = stripe_size - remainder;
      if ( msg->io_payload_size <= remainder )
	{
	  // enough space for current request
	  s->fs_sender_next_available_time = max(s->fs_sender_next_available_time, tw_now(lp));
	  ts = s->fs_sender_next_available_time - tw_now(lp);

	  s->fs_sender_next_available_time += ION_FS_meta_payload/ION_FS_out_bw;

	  e = tw_event_new( s->file_server[my_start_FS], ts + ION_FS_meta_payload/ION_FS_out_bw, lp );
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

	  m->IsLastPacket = 1;

	  tw_event_send(e);
	  
	}
      else
	{
	  // need to talk to 2 file server
	  s->fs_sender_next_available_time = max(s->fs_sender_next_available_time, tw_now(lp));
	  ts = s->fs_sender_next_available_time - tw_now(lp);

	  s->fs_sender_next_available_time += ION_FS_meta_payload/ION_FS_out_bw;

	  e = tw_event_new( s->file_server[my_start_FS], ts + ION_FS_meta_payload/ION_FS_out_bw, lp );
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

	  m->IsLastPacket = 0;

	  tw_event_send(e);

	  ////
	  s->fs_sender_next_available_time = max(s->fs_sender_next_available_time, tw_now(lp));
	  ts = s->fs_sender_next_available_time - tw_now(lp);

	  s->fs_sender_next_available_time += ION_FS_meta_payload/ION_FS_out_bw;

	  e = tw_event_new( s->file_server[(my_start_FS + 1)%(nlp_FS*N_PE)], ts + ION_FS_meta_payload/ION_FS_out_bw, lp );
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

	  m->IsLastPacket = 1;

	  tw_event_send(e);

	} 
    }
}

void ion_handshake_end( ION_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  tw_event * e;
  tw_stime ts;
  MsgData * m;

#ifdef TRACE
  printf("handshake %d End at ION %d travel time is %lf\n",
         msg->message_CN_source,
	 s->myID_in_ION,
         tw_now(lp) - msg->travel_start_time );
#endif
  s->fs_receiver_next_available_time = max(s->fs_receiver_next_available_time, tw_now(lp));
  ts = s->fs_receiver_next_available_time - tw_now(lp);

  s->fs_receiver_next_available_time += ION_FS_meta_payload/ION_FS_in_bw;

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

  random_FS = msg->io_tag % ( nlp_FS*N_PE ) ; 

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

  tw_event_send(e);

}

void ion_data_send( ION_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  tw_event * e;
  tw_stime ts;
  MsgData * m;

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

  e = tw_event_new( msg->message_FS_source, ts + msg->io_payload_size/ION_FS_out_bw, lp );
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

  tw_event_send(e);

}

void ion_close_arrive( ION_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  tw_event * e;
  tw_stime ts;
  MsgData * m;

#ifdef TRACE
  printf("close %d arrive at ION %d travel time is %lf IO tag is %d\n",
         msg->message_CN_source,
	 s->myID_in_ION,
         tw_now(lp) - msg->travel_start_time,
	 msg->io_tag);
#endif

  s->cn_receiver_next_available_time = max(s->cn_receiver_next_available_time, tw_now(lp));
  ts = s->cn_receiver_next_available_time - tw_now(lp);

  s->cn_receiver_next_available_time += msg->io_payload_size/ION_CN_in_bw;

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

  tw_event_send(e);

}


void ion_data_ack( ION_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  tw_event * e;
  tw_stime ts;
  MsgData * m;

  if (msg->IsLastPacket)
    {
#ifdef TRACE
      printf("data %d ACKed at ION travel time is %lf, IO tag is %d\n",
             msg->message_CN_source,
             tw_now(lp) - msg->travel_start_time,
             msg->io_tag);
#endif
      ts = ION_CONT_msg_prep_time;

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
      m->message_CN_source = msg->message_CN_source;
      m->message_FS_source = msg->message_FS_source;

      m->IsLastPacket = msg->IsLastPacket;

      tw_event_send(e);
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
      m->event_type = CLOSE_ACK;

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
  free(s->file_server);
  free(s->io_node);
}
