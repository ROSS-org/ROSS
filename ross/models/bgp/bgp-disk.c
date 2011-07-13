/*
 *  Blue Gene/P model
 *  DDN
 *  by Ning Liu 
 */
#include "bgp.h"

void bgp_disk_init( Disk_state* s,  tw_lp* lp )
{
  //printf("This is ION init\n");
  //printf("ION LP %d talking\n",lp->gid);
  //printf("Disk LP %d speaking\n", lp->gid);
}

void bgp_disk_eventHandler( Disk_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  tw_event * e;
  tw_stime ts;
  MsgData * m;
  int i;

  switch(msg->type)
    {
    case GENERATE:
      //packet_generate(s,bf,msg,lp); 
      break;
    case ARRIVAL:
      //printf("Disk arrival\n");
      /*     ts = 10000;
      e = tw_event_new( lp->gid, ts, lp );
      m = tw_event_data(e);
      m->type = PROCESS;

      m->MsgSrc = ComputeNode;
      tw_event_send(e);
      */
      //packet_arrive(s,bf,msg,lp);  

      printf("Travel time is %lf\n",tw_now(lp) - msg->travel_start_time );

      break;
    case SEND:
      /*     for (i=0; i<N_Disk_per_FS; i++)
	{
	  ts = 10000;
	  e = tw_event_new( s->disk_id[i], ts, lp );
	  m = tw_event_data(e);
	  m->type = ARRIVAL;

	  m->MsgSrc = ComputeNode;
	  tw_event_send(e);
	  //packet_send(s,bf,msg,lp);                                              
	  }*/
      //printf("File Server Sent \n");
      break;

    case PROCESS:
      /*
      ts = 10000;
      e = tw_event_new( lp->gid, ts, lp );
      m = tw_event_data(e);
      m->type = SEND;

      m->MsgSrc = ComputeNode;
      tw_event_send(e);
      //packet_process(s,bf,msg,lp);
      */ 
      break;
    }


}

void bgp_disk_eventHandler_rc( Disk_state* s, tw_bf* bf, MsgData* m, tw_lp* lp )
{}

void bgp_disk_finish( Disk_state* s, tw_lp* lp )
{}
