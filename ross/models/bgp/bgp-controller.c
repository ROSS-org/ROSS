/*
 *  Blue Gene/P model
 *  DDN controller
 *  by Ning Liu 
 */
#include "bgp.h"

void bgp_controller_init( CON_state* s,  tw_lp* lp )
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
  localID = localID - nlp_CN - nlp_ION - nlp_FS;
  localID /= NumControllerPerDDN;
  s->ddn_id = PEid * nlp_per_pe + nlp_CN + nlp_ION + nlp_FS+ nlp_Controller + localID;

  // get the IDs of the file servers which are hooked to this DDN
  int i;
  s->previous_FS_id = (int *)calloc( NumFSPerController, sizeof(int) );
  int base = nlp_CN + nlp_ION;
  localID = lp->gid % nlp_per_pe;
  localID = localID - base - nlp_FS;

  for (i=0; i<NumFSPerController; i++)
    s->previous_FS_id[i] = PEid * nlp_per_pe + base +
      localID * NumFSPerController + i;

  s->processor_next_available_time = 0;

#ifdef PRINTid
  printf("controller LP %d speaking, my DDN is %d \n", lp->gid, s->ddn_id);
  for (i=0; i<NumFSPerController; i++)
    printf("Controller LP %d speaking, my FS is %d\n", lp->gid,s->previous_FS_id[i]);
#endif

}

void cont_create_arrive( CON_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  tw_event * e;
  tw_stime ts;
  MsgData * m;

  double transmission_time = FS_DDN_meta_payload/CONT_FS_in_bw;

#ifdef TRACE
  printf("create %d arrive at controller travel time is %lf\n",
         msg->message_CN_source,
         tw_now(lp) - msg->travel_start_time );
#endif
  s->fs_receiver_next_available_time = max(s->fs_receiver_next_available_time, tw_now(lp) - transmission_time);
  ts = s->fs_receiver_next_available_time - tw_now(lp) + transmission_time;
  s->fs_receiver_next_available_time += transmission_time;

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
  m->message_FS_source = msg->message_FS_source;
  m->message_CN_source = msg->message_CN_source;
  m->io_tag = msg->io_tag;

  tw_event_send(e);

}

void cont_create_process( CON_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  tw_event * e;
  tw_stime ts;
  MsgData * m;

#ifdef TRACE
  printf("create %d process at controller travel time is %lf\n",
         msg->message_CN_source,
         tw_now(lp) - msg->travel_start_time );
#endif
  ts = CONT_CONT_msg_prep_time;

  e = tw_event_new( lp->gid, ts , lp );
  m = tw_event_data(e);
  m->event_type = CREATE_ACK;

  m->travel_start_time = msg->travel_start_time;

  m->io_offset = msg->io_offset;
  m->io_payload_size = msg->io_payload_size;
  m->collective_group_size = msg->collective_group_size;
  m->collective_group_rank = msg->collective_group_rank;

  m->collective_master_node_id = msg->collective_master_node_id;
  m->io_type = msg->io_type;

  m->message_ION_source = msg->message_ION_source;
  m->message_FS_source = msg->message_FS_source;
  m->message_CN_source = msg->message_CN_source;
  m->io_tag = msg->io_tag;

  tw_event_send(e);

}

void cont_create_ack( CON_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  tw_event * e;
  tw_stime ts;
  MsgData * m;

#ifdef TRACE
  printf("create %d ACKed at controller travel time is %lf\n",
         msg->message_CN_source,
         tw_now(lp) - msg->travel_start_time );
#endif
  ts = CONT_CONT_msg_prep_time;

  e = tw_event_new( msg->message_FS_source, ts , lp );
  m = tw_event_data(e);
  m->event_type = CREATE_ACK;

  m->travel_start_time = msg->travel_start_time;

  m->io_offset = msg->io_offset;
  m->io_payload_size = msg->io_payload_size;
  m->collective_group_size = msg->collective_group_size;
  m->collective_group_rank = msg->collective_group_rank;

  m->collective_master_node_id = msg->collective_master_node_id;
  m->io_type = msg->io_type;

  m->message_ION_source = msg->message_ION_source;
  m->message_FS_source = msg->message_FS_source;
  m->message_CN_source = msg->message_CN_source;
  m->io_tag = msg->io_tag;

  tw_event_send(e);

}

void cont_data_arrive( CON_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  tw_event * e;
  tw_stime ts;
  MsgData * m;

  double transmission_time = msg->io_payload_size/CONT_FS_in_bw;

#ifdef TRACE
  printf("data %d arrive at controller travel time is %lf\n",
         msg->message_CN_source,
         tw_now(lp) - msg->travel_start_time );
#endif
  s->fs_receiver_next_available_time = max(s->fs_receiver_next_available_time, tw_now(lp) - transmission_time);
  ts = s->fs_receiver_next_available_time - tw_now(lp) + transmission_time;
  s->fs_receiver_next_available_time += transmission_time;

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
  m->message_FS_source = msg->message_FS_source;
  m->message_CN_source = msg->message_CN_source;
  m->io_tag = msg->io_tag;

  m->IsLastPacket = msg->IsLastPacket;

  tw_event_send(e);

}

void cont_read_arrive( CON_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  tw_event * e;
  tw_stime ts;
  MsgData * m;

  double transmission_time = FS_DDN_read_meta/CONT_FS_in_bw;

#ifdef TRACE
  printf("read %d arrive at controller travel time is %lf\n",
         msg->message_CN_source,
         tw_now(lp) - msg->travel_start_time );
#endif
  s->fs_receiver_next_available_time = max(s->fs_receiver_next_available_time, tw_now(lp) - transmission_time);
  ts = s->fs_receiver_next_available_time - tw_now(lp) + transmission_time;
  s->fs_receiver_next_available_time += transmission_time;

  e = tw_event_new( lp->gid, ts , lp );
  m = tw_event_data(e);
  m->event_type = READ_PROCESS;

  m->travel_start_time = msg->travel_start_time;

  m->io_offset = msg->io_offset;
  m->io_payload_size = msg->io_payload_size;
  m->collective_group_size = msg->collective_group_size;
  m->collective_group_rank = msg->collective_group_rank;

  m->collective_master_node_id = msg->collective_master_node_id;
  m->io_type = msg->io_type;

  m->message_ION_source = msg->message_ION_source;
  m->message_FS_source = msg->message_FS_source;
  m->message_CN_source = msg->message_CN_source;
  m->io_tag = msg->io_tag;

  m->IsLastPacket = msg->IsLastPacket;

  tw_event_send(e);

}

void cont_close_arrive( CON_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  tw_event * e;
  tw_stime ts;
  MsgData * m;

  double transmission_time = close_meta_size/CONT_FS_in_bw;

#ifdef TRACE
  printf("close %d arrive at controller travel time is %lf\n",
         msg->message_CN_source,
         tw_now(lp) - msg->travel_start_time );
#endif
  s->fs_receiver_next_available_time = max(s->fs_receiver_next_available_time, tw_now(lp) - transmission_time);
  ts = s->fs_receiver_next_available_time - tw_now(lp) + transmission_time;
  s->fs_receiver_next_available_time += transmission_time;

  e = tw_event_new( lp->gid, ts , lp );
  m = tw_event_data(e);
  m->event_type = CLOSE_ACK;

  m->travel_start_time = msg->travel_start_time;

  m->io_offset = msg->io_offset;
  m->io_payload_size = msg->io_payload_size;
  m->collective_group_size = msg->collective_group_size;
  m->collective_group_rank = msg->collective_group_rank;

  m->collective_master_node_id = msg->collective_master_node_id;
  m->io_type = msg->io_type;

  m->message_ION_source = msg->message_ION_source;
  m->message_FS_source = msg->message_FS_source;
  m->message_CN_source = msg->message_CN_source;
  m->io_tag = msg->io_tag;

  m->IsLastPacket = msg->IsLastPacket;

  tw_event_send(e);

}

void cont_data_process( CON_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  tw_event * e;
  tw_stime ts;
  MsgData * m;

#ifdef TRACE
  printf("data %d process at controller travel time is %lf\n",
         msg->message_CN_source,
         tw_now(lp) - msg->travel_start_time );
#endif
  ts = CONT_CONT_msg_prep_time;

  e = tw_event_new( lp->gid, ts , lp );
  m = tw_event_data(e);
  m->event_type = DATA_ACK;

  m->travel_start_time = msg->travel_start_time;

  m->io_offset = msg->io_offset;
  m->io_payload_size = msg->io_payload_size;
  m->collective_group_size = msg->collective_group_size;
  m->collective_group_rank = msg->collective_group_rank;

  m->collective_master_node_id = msg->collective_master_node_id;
  m->io_type = msg->io_type;

  m->message_ION_source = msg->message_ION_source;
  m->message_FS_source = msg->message_FS_source;
  m->message_CN_source = msg->message_CN_source;
  m->io_tag = msg->io_tag;

  m->IsLastPacket = msg->IsLastPacket;

  tw_event_send(e);


}

void cont_read_process( CON_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  tw_event * e;
  tw_stime ts;
  MsgData * m;

#ifdef TRACE
  printf("read %d process at controller travel time is %lf\n",
         msg->message_CN_source,
         tw_now(lp) - msg->travel_start_time );
#endif
  ts = CONT_CONT_msg_prep_time;

  e = tw_event_new( lp->gid, ts , lp );
  m = tw_event_data(e);
  m->event_type = READ_ACK;

  m->travel_start_time = msg->travel_start_time;

  m->io_offset = msg->io_offset;
  m->io_payload_size = msg->io_payload_size;
  m->collective_group_size = msg->collective_group_size;
  m->collective_group_rank = msg->collective_group_rank;

  m->collective_master_node_id = msg->collective_master_node_id;
  m->io_type = msg->io_type;

  m->message_ION_source = msg->message_ION_source;
  m->message_FS_source = msg->message_FS_source;
  m->message_CN_source = msg->message_CN_source;
  m->io_tag = msg->io_tag;

  m->IsLastPacket = msg->IsLastPacket;

  tw_event_send(e);

}

void cont_data_ack( CON_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  tw_event * e;
  tw_stime ts;
  MsgData * m;

#ifdef TRACE
  printf("data %d ACKed at controller travel time is %lf\n",
         msg->message_CN_source,
         tw_now(lp) - msg->travel_start_time );
#endif
  ts = CONT_CONT_msg_prep_time;

  e = tw_event_new( msg->message_FS_source, ts , lp );
  m = tw_event_data(e);
  m->event_type = DATA_ACK;

  m->travel_start_time = msg->travel_start_time;

  m->io_offset = msg->io_offset;
  m->io_payload_size = msg->io_payload_size;
  m->collective_group_size = msg->collective_group_size;
  m->collective_group_rank = msg->collective_group_rank;

  m->collective_master_node_id = msg->collective_master_node_id;
  m->io_type = msg->io_type;

  m->message_ION_source = msg->message_ION_source;
  m->message_FS_source = msg->message_FS_source;
  m->message_CN_source = msg->message_CN_source;
  m->io_tag = msg->io_tag;

  m->IsLastPacket = msg->IsLastPacket;

  tw_event_send(e);

}

void cont_read_ack( CON_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  tw_event * e;
  tw_stime ts;
  MsgData * m;

  double transmission_time = msg->io_payload_size/DDN_FS_out_bw;

#ifdef TRACE
  printf("read %d ACKed at controller travel time is %lf\n",
         msg->message_CN_source,
         tw_now(lp) - msg->travel_start_time );
#endif

  s->fs_sender_next_available_time = max(s->fs_sender_next_available_time, tw_now(lp) );
  ts = s->fs_sender_next_available_time - tw_now(lp);
  s->fs_sender_next_available_time += transmission_time;

  e = tw_event_new( msg->message_FS_source, ts + transmission_time , lp );
  m = tw_event_data(e);
  m->event_type = READ_ACK;

  m->travel_start_time = msg->travel_start_time;

  m->io_offset = msg->io_offset;
  m->io_payload_size = msg->io_payload_size;
  m->collective_group_size = msg->collective_group_size;
  m->collective_group_rank = msg->collective_group_rank;

  m->collective_master_node_id = msg->collective_master_node_id;
  m->io_type = msg->io_type;

  m->message_ION_source = msg->message_ION_source;
  m->message_FS_source = msg->message_FS_source;
  m->message_CN_source = msg->message_CN_source;
  m->io_tag = msg->io_tag;

  m->IsLastPacket = msg->IsLastPacket;

  tw_event_send(e);

}

void cont_close_ack( CON_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  tw_event * e;
  tw_stime ts;
  MsgData * m;

#ifdef TRACE
  printf("close %d ACKed at controller travel time is %lf\n",
         msg->message_CN_source,
         tw_now(lp) - msg->travel_start_time );
#endif
  ts = CONT_CONT_msg_prep_time;

  e = tw_event_new( msg->message_FS_source, ts , lp );
  m = tw_event_data(e);
  m->event_type = CLOSE_ACK;

  m->travel_start_time = msg->travel_start_time;

  m->io_offset = msg->io_offset;
  m->io_payload_size = msg->io_payload_size;
  m->collective_group_size = msg->collective_group_size;
  m->collective_group_rank = msg->collective_group_rank;

  m->collective_master_node_id = msg->collective_master_node_id;
  m->io_type = msg->io_type;

  m->message_ION_source = msg->message_ION_source;
  m->message_FS_source = msg->message_FS_source;
  m->message_CN_source = msg->message_CN_source;
  m->io_tag = msg->io_tag;

  m->IsLastPacket = msg->IsLastPacket;

  tw_event_send(e);

}


void bgp_controller_eventHandler( CON_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{

  switch(msg->event_type)
    {
    case CREATE_ARRIVE:
      cont_create_arrive(s, bf, msg, lp);
      break;
    case DATA_ARRIVE:
      cont_data_arrive(s, bf, msg, lp);
      break;
    case READ_ARRIVE:
      cont_read_arrive(s, bf, msg, lp);
      break;
    case CLOSE_ARRIVE:
      cont_close_arrive(s, bf, msg, lp);
      break;
    case CREATE_PROCESS:
      cont_create_process(s, bf, msg, lp);
      break;
    case DATA_PROCESS:
      cont_data_process(s, bf, msg, lp);
      break;
    case READ_PROCESS:
      cont_read_process(s, bf, msg, lp);
      break;
    case CREATE_ACK:
      cont_create_ack(s, bf, msg, lp);
      break;
    case DATA_ACK:
      cont_data_ack(s, bf, msg, lp);
      break;
    case READ_ACK:
      cont_read_ack(s, bf, msg, lp);
      break;
    case CLOSE_ACK:
      cont_close_ack(s, bf, msg, lp);
      break;
    default:
      printf("Scheduled wrong events in controller\n");
    }
}

void bgp_controller_eventHandler_rc( CON_state* s, tw_bf* bf, MsgData* m, tw_lp* lp )
{}

void bgp_controller_finish( CON_state* s, tw_lp* lp )
{
  free( s->previous_FS_id );
}
