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

  // specific to this configuration, Kamil's BG/P tree graph
  rootCN = 7;
  
  // tree, previous hop
  s->tree_previous_hop_id[0] = -1;
  s->tree_previous_hop_id[1] = -1;

  s->MsgPrepTime = CN_message_wrap_time;
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
  
  s->tree_next_hop_id = get_tree_next_hop(s->CN_ID_in_tree);

  // get reverse mapping

  s->tree_next_hop_id += basePE * nlp_per_pe + basePset * N_CN_per_ION;
  if ( s->CN_ID_in_tree == rootCN )
    s->tree_next_hop_id = basePE * nlp_per_pe + nlp_CN + basePset;
  s->upID = s->tree_next_hop_id;

  /////////////////////////////
  // calculate packet round
  s->N_packet_round = collective_block_size/payload_size;

  
  //printf("packet round is %d\n",s->N_packet_round);

#ifdef PRINTid
  printf("CN %d local ID is %d next hop is %d \n", 
	 lp->gid,
	 s->CN_ID_in_tree,
	 s->tree_next_hop_id);
#endif

  // configure tree network, before GENERATE
  // seperate line from the DATA stream
  ts = 10;
  e = tw_event_new( s->tree_next_hop_id, ts, lp );
  m = tw_event_data(e);
  m->type = CONFIG;

  m->msg_src_lp_id = lp->gid;
  m->message_type = CONT;
  m->travel_start_time = tw_now(lp);

  tw_event_send(e);

  /*// DATA stream
  ts = 10000;
  e = tw_event_new( lp->gid, ts, lp );
  m = tw_event_data(e);
  m->type = GENERATE;
  
  //m->msg_src_lp_id = lp->gid;
  m->message_type = DATA;

  tw_event_send(e);*/

  // CONT message stream
  ts = 10000;
  e = tw_event_new( lp->gid, ts, lp );
  m = tw_event_data(e);
  m->type = IOrequest;

  m->travel_start_time = tw_now(lp);
  m->message_type = CONT;

  tw_event_send(e);
  
}

void bgp_cn_eventHandler( CN_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  
  tw_event * e; 
  tw_stime ts;
  MsgData * m;
  int i;
  
  switch(msg->type)
    {
    case CONFIG:
      switch(msg->message_type)
	{
	case CONT:
	  {
	    // when receive data message, record msg source
	    if ( s->tree_previous_hop_id[0] == -1)
	      s->tree_previous_hop_id[0] = msg->msg_src_lp_id;
	    else
	      s->tree_previous_hop_id[1] = msg->msg_src_lp_id;
	
#ifdef TRACE 
	    printf("CN %d received CONFIG CONT message\n",s->CN_ID_in_tree);
	    for ( i=0; i<2; i++ )
	      printf("CN %d previous hop [%d] is %d\n",
		     s->CN_ID_in_tree,
		     i,
		     s->tree_previous_hop_id[i]);
#endif

	    // configure the tree, identify previous hops
	    ts = s->MsgPrepTime + lp->gid % 64;
	    
	    // handshake: send back
	    for ( i=0; i<2; i++ )
	      {
		if ( s->tree_previous_hop_id[i] >0 )
		  {
		    e = tw_event_new( s->tree_previous_hop_id[i], ts, lp );
		    m = tw_event_data(e);
		    
		    m->type = CONFIG;
		    m->msg_src_lp_id = lp->gid;
		    m->travel_start_time = msg->travel_start_time;
		    
		    m->collective_msg_tag = 12180;
		    m->message_type = ACK;
		    
		    tw_event_send(e);
		  }
	      }
	    
	  }
	  break;
	case ACK:
#ifdef TRACE
	  printf("CN %d received CONFIG ACK message, travel time is %lf\n",
		 s->CN_ID_in_tree,
		 tw_now(lp) - msg->travel_start_time );
	  
#endif
	  break;
	}
      break;
    case IOrequest:
      switch (msg->message_type)
	{
	case CONT:
	  ts = s->MsgPrepTime + s->CN_ID_in_tree;
	  e = tw_event_new( s->tree_next_hop_id, ts, lp );
	  m = tw_event_data(e);
	  m->type = IOrequest;
	  
	  //trace packet time
	  m->travel_start_time = msg->travel_start_time;
	  
	  m->collective_msg_tag = 12180;
	  m->message_type = CONT;
	  
	  tw_event_send(e);
	  
	  break;
	case ACK:
	  //printf("CN %d received IO request ACK\n",s->CN_ID_in_tree);
	  ts = 10;
	  // send ACK message to all my leafs
	  for ( i=0; i<2; i++)
	    {
	      if ( s->tree_previous_hop_id[i] > -1 )
		{
		  e = tw_event_new( s->tree_previous_hop_id[i], 
				    ts, lp );
		  m = tw_event_data(e);
		  m->type = IOrequest;
		  
		  m->travel_start_time = msg->travel_start_time;
		  m->collective_msg_tag = msg->collective_msg_tag;
		  m->message_type = msg->message_type;
		  m->message_size = msg->message_size;
		  
		  tw_event_send(e);
		}
	    }
	  // ready to send data
	  // ACK requests and turn to DATA stream
	  e = tw_event_new( lp->gid, ts, lp );
	  m = tw_event_data(e);
	  m->type = GENERATE;

	  m->travel_start_time = msg->travel_start_time;
	  m->CN_message_round = s->N_packet_round;
	  tw_event_send(e);
	  
	  break;
	case DATA:
	  printf("Error: data message has been sent to IOrequest\n");
	  break;
	  }
      break;
    case GENERATE:

      // enter next round message send
      /*
      if ( msg->CN_message_round > 0 )
	{
	  ts = tw_rand_exponential(lp->rng, 5);
	  e = tw_event_new(lp->gid, ts, lp);
	  m = tw_event_data(e);
	  m->type = GENERATE;
      
	  m->CN_message_round = msg->CN_message_round;
	  tw_event_send(e);
	}
      */
      // send this message out
      // add randomness
      /*
      printf("lp %d this round is %d\n",
	     s->CN_ID_in_tree,
	     msg->CN_message_round);
      */
      for ( i=0; i<msg->CN_message_round; i++)
	{
	  ts = s->MsgPrepTime + i;
	  e = tw_event_new( s->tree_next_hop_id, ts, lp );
	  m = tw_event_data(e);
	  m->type = ARRIVAL;
	  
	  m->message_size = payload_size;
	  
	  //trace packet time
	  //m->travel_start_time = tw_now(lp);
	  m->travel_start_time = msg->travel_start_time;
	  
	  //trace collective calls
	  //printf("collective counter is %d\n",msg_collective_counter);
	  //m->collective_msg_tag = msg_collective_counter;
	  
	  m->collective_msg_tag = 12180;
	  m->message_type = DATA;
	  
	  tw_event_send(e);
	}
      for ( i=0; i<msg->CN_message_round; i++)
	{
	  ts = s->MsgPrepTime + i;
	  e = tw_event_new( lp->gid, ts, lp );
	  m = tw_event_data(e);
	  m->type = ARRIVAL;
	  
	  m->message_size = payload_size;
	  
	  //trace packet time
	  //m->travel_start_time = tw_now(lp);
	  m->travel_start_time = msg->travel_start_time;
	  
	  //trace collective calls
	  //printf("collective counter is %d\n",msg_collective_counter);
	  //m->collective_msg_tag = msg_collective_counter;
	  
	  m->collective_msg_tag = 12180;
	  m->message_type = DATA;
	  
	  tw_event_send(e);
	}
      break;
    case ARRIVAL:
      {
	switch( msg->message_type )
	  {
	case DATA:
	  {

	    //printf("CN received DATA message\n");
	    s->next_available_time = max(s->next_available_time, tw_now(lp));
	    ts = s->next_available_time - tw_now(lp);
	    
	    s->next_available_time += CN_packet_service_time;
	    
	    e = tw_event_new( lp->gid, ts, lp );
	    m = tw_event_data(e);
	    m->type = PROCESS;
	    
	    // pass msg info
	    m->travel_start_time = msg->travel_start_time;
	    m->collective_msg_tag = msg->collective_msg_tag;
	    m->message_type = msg->message_type;
	    m->message_size = msg->message_size;
	    
	    tw_event_send(e);
	  }
	  break;
	  }
      }
      break;
    case SEND:
      switch( msg->message_type )
        {
	case CONT:
	  ts = 10;
	  e = tw_event_new( s->tree_next_hop_id, ts, lp );
	  m = tw_event_data(e);
	  m->type = ARRIVAL;
	  
	  // pass msg info
	  m->travel_start_time = msg->travel_start_time;
	  m->collective_msg_tag = msg->collective_msg_tag;
	  m->message_type = CONT;
	  m->MsgSrc = ComputeNode;
	  
	  tw_event_send(e);
	  
	  break;
        case ACK:
          {   
	    ts = 10;
	    // send ACK message to all my leafs
	    for ( i=0; i<2; i++)
	      {
		if ( s->tree_previous_hop_id[i] > -1 )
		  {
		    e = tw_event_new( s->tree_previous_hop_id[i], 
				      ts, lp );
		    m = tw_event_data(e);
		    m->type = ARRIVAL;
		    
		    m->travel_start_time = msg->travel_start_time;
		    m->collective_msg_tag = msg->collective_msg_tag;
		    m->message_type = msg->message_type;
		    m->message_size = msg->message_size;
		    
		    tw_event_send(e);
		  }
	      }
	  }	
	  break;
	case DATA:
	  {    
	    s->nextLinkAvailableTime = max(s->nextLinkAvailableTime, tw_now(lp));
	    ts = s->nextLinkAvailableTime - tw_now(lp);
	    
	    s->nextLinkAvailableTime += msg->message_size/CN_tree_bw;
	    
	    e = tw_event_new( s->tree_next_hop_id, 
			      ts + msg->message_size/CN_tree_bw, lp );
	    m = tw_event_data(e);
	    m->type = ARRIVAL;
	    
	    m->travel_start_time = msg->travel_start_time;
	    m->collective_msg_tag = msg->collective_msg_tag;
	    m->message_type = msg->message_type;
	    m->message_size = msg->message_size;
	    
	    tw_event_send(e);
	    
	  }
	  break;
	}
      
      break;
    case PROCESS:
      ts = CN_packet_service_time;
      e = tw_event_new( lp->gid, ts, lp );
      m = tw_event_data(e);
      m->type = SEND;

      m->travel_start_time = msg->travel_start_time;
      m->collective_msg_tag = msg->collective_msg_tag;
      m->message_type = msg->message_type;
      m->message_size = msg->message_size;

      tw_event_send(e);

      break;
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
