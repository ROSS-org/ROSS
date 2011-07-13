/*
 *  Blue Gene/P model
 *  Compute node / Tree Network
 *  by Ning Liu 
 */
#include "bgp.h"

void bgp_cn_init( CN_state* s,  tw_lp* lp )
{

  /////////////////////////
  tw_event *e;
  tw_stime ts;
  MsgData *m;

  rootCN = 7;
  
  int N_PE = tw_nnodes();

  nlp_DDN = NumDDN / N_PE;
  nlp_Controller = nlp_DDN * NumControllerPerDDN;
  nlp_FS = nlp_Controller * NumFSPerController;
  nlp_ION = nlp_FS * N_ION_per_FS;
  nlp_CN = nlp_ION * N_CN_per_ION;

  // printf("Compute Node LP %d speaking \n",lp->gid);  
// nlp_ION is %d \n nlp_FS is %d \n nlp_Controller is %d\n nlp_DDN is %d\n", lp->gid, nlp_ION, nlp_FS, nlp_Controller, nlp_DDN );

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

  //printf("compute node %d local id is %d next hop is %d basePE is %d basePset is %d \n",lp->gid,s->CN_ID_in_tree,s->tree_next_hop_id,basePE,basePset);


  ts = 10000;
  e = tw_event_new( lp->gid, ts, lp );
  m = tw_event_data(e);
  m->type = GENERATE;

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
      //printf("Generate\n");
      //generate another generate event
      
      msg_collective_counter++;

      ts = 100000;
      e = tw_event_new(lp->gid, ts, lp);
      m = tw_event_data(e);
      m->type = GENERATE;
      
      m->MsgSrc = ComputeNode;
      tw_event_send(e);
      

      ts = 10000;
      e = tw_event_new( lp->gid, ts, lp );
      m = tw_event_data(e);
      m->type = ARRIVAL;

      //trace packet time
      m->travel_start_time = tw_now(lp);
      //trace collective calls
      m->collective_msg_tag = msg_collective_counter;
      m->MsgSrc = ComputeNode;
      tw_event_send(e);


      break;
    case ARRIVAL:
      //printf("CN CN arrived\n");

      s->next_available_time = max(s->next_available_time, tw_now(lp));
      ts = s->next_available_time - tw_now(lp);
      
      s->next_available_time += CN_packet_service_time;

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

      s->nextLinkAvailableTime = max(s->nextLinkAvailableTime, tw_now(lp));
      ts = s->nextLinkAvailableTime - tw_now(lp);

      s->nextLinkAvailableTime += link_transmission_time;

      e = tw_event_new( s->tree_next_hop_id, ts, lp );
      m = tw_event_data(e);
      m->type = ARRIVAL;

      m->travel_start_time = msg->travel_start_time;
      m->collective_msg_tag = msg->collective_msg_tag;
      m->MsgSrc = ComputeNode;
      tw_event_send(e);
      //packet_send(s,bf,msg,lp);
      break;
    case PROCESS:
      ts = CN_packet_service_time;
      e = tw_event_new( lp->gid, ts, lp );
      m = tw_event_data(e);
      m->type = SEND;

      m->travel_start_time = msg->travel_start_time;
      m->collective_msg_tag = msg->collective_msg_tag;
      m->MsgSrc = ComputeNode;
      tw_event_send(e);
      //packet_process(s,bf,msg,lp);
      break;
    }
  
}

void bgp_cn_eventHandler_rc( CN_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{}

void bgp_cn_finish( CN_state* s, tw_lp* lp )
{}

int get_tree_next_hop( int TreeNextHopIndex )
{
  // give me the index and return the next hop ID
  int TreeMatrix[32];

  TreeMatrix[0]=4;
  TreeMatrix[1]=17;
  TreeMatrix[2]=6;
  TreeMatrix[3]=7;


  //TreeMatrix[0]=1;
  //TreeMatrix[1]=17;
  //TreeMatrix[2]=6;
  //TreeMatrix[3]=7;
  

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
