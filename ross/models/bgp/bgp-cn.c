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
  s->N_packet_round = collective_block_size/payload_size/N_CN_per_ION;
  
  //printf("packet round is %d\n",s->N_packet_round);

#ifdef PRINTid
  printf("CN %d local ID is %d next hop is %d \n", 
	 lp->gid,
	 s->CN_ID_in_tree,
	 s->tree_next_hop_id);
#endif

  ts = 10000;
  e = tw_event_new( lp->gid, ts, lp );
  m = tw_event_data(e);
  m->type = GENERATE;

  m->CN_message_round = s->N_packet_round;

  tw_event_send(e);
}

void bgp_cn_eventHandler( CN_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  
  tw_event * e; 
  tw_stime ts;
  MsgData * m;
  
  switch(msg->type)
    {
    case GENERATE:
      
      // initialized to 0, count collective message
      msg_collective_counter++;
      msg->CN_message_round--;

      // enter next round message send
      if ( msg->CN_message_round > 0 )
	{
	  ts = 50;
	  e = tw_event_new(lp->gid, ts, lp);
	  m = tw_event_data(e);
	  m->type = GENERATE;
      
	  m->CN_message_round = msg->CN_message_round;
	  tw_event_send(e);
	}
      
      // send this message out
      // add randomness

      ts = s->MsgPrepTime + lp->gid%23;
      e = tw_event_new( lp->gid, ts, lp );
      m = tw_event_data(e);
      m->type = ARRIVAL;

      m->message_size = payload_size;

      //trace packet time
      m->travel_start_time = tw_now(lp);

      //trace collective calls
      //printf("collective counter is %d\n",msg_collective_counter);
      //m->collective_msg_tag = msg_collective_counter;

      m->collective_msg_tag = 12180;
      m->message_type = DATA;

      tw_event_send(e);

      break;
    case ARRIVAL:
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
      break;
    case SEND:

      s->nextLinkAvailableTime = max(s->nextLinkAvailableTime, tw_now(lp));
      ts = s->nextLinkAvailableTime - tw_now(lp);

      s->nextLinkAvailableTime += msg->message_size/CN_tree_bw;

      e = tw_event_new( s->tree_next_hop_id, ts + msg->message_size/CN_tree_bw, lp );
      m = tw_event_data(e);
      m->type = ARRIVAL;

      m->travel_start_time = msg->travel_start_time;
      m->collective_msg_tag = msg->collective_msg_tag;
      m->message_type = msg->message_type;
      m->message_size = msg->message_size;

      tw_event_send(e);

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
