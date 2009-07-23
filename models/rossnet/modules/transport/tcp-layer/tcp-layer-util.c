#include <tcp-layer.h>

/*********************************************************************
		       Creates a message 
*********************************************************************/

void
tcp_layer_util_event(tw_lp * lp, int type, int source, int dest,
			int seq_num, int ack, int size)
{
  rn_message  * r;
  tcp_layer_message * TWMsg;
  tw_event *CurEvent;

  // should put in the tcp_util_nexthop 
  CurEvent = rn_event_new(tw_getlp(dest), 0.0, lp, DOWNSTREAM, size);
  
  
  r = rn_event_data(CurEvent);
  
  TWMsg = rn_message_data(r);
  
  //printf("%d: TCP sending event to lp %d ts = %f %f\n", CurEvent->src_lp->id,
  //	   CurEvent->dest_lp->id, CurEvent->recv_ts, ts);
  
  
  TWMsg->MethodName = type;
  TWMsg->source = source;
  TWMsg->dest = dest;
  TWMsg->seq_num = seq_num;
  TWMsg->ack = ack;
  
  rn_event_send(CurEvent); 
}





