#include <tcp-layer.h>

/*
 * Initializes a host and send appropriate messages
 *
 * This function simply zeros out the host state and then
 * inits some simple values.
 */

void
tcp_layer_init(tcp_layer_state * SV, xmlNodePtr node, tw_lp * lp)
{
  SV->bad_msgs = 0;
  SV->cwnd = 1;
  SV->ssthresh = TCP_LAYER_SND_WND;
  SV->rto = 3;
  SV->host = rn_getmachine(lp->id);

  SV->connection = SV->host->conn;
  
  if(SV->connection > -1) 
    {
      SV->timer = rn_timer_init(lp, 1);
      
      if(SV->timer)
	{
	  rn_message *msg;
	  tcp_layer_message *M;
	  msg = tw_event_data(SV->timer);
	  M = rn_message_data(msg);
	  M->MethodName = STARTUP;
	  printf("%d %d \n",STARTUP, M->MethodName);
	}
    }   
}


void
tcp_layer_startup(tcp_layer_state * SV, tw_bf * CV, rn_message * M, tw_lp * lp)
{
  SV->len =  pow(2,31) - 1;      
  SV->start_time = 1;

  tcp_layer_util_event(lp, FORWARD, lp->id, SV->connection, 0, 0, 
		       TCP_LAYER_TRANSFER_SIZE);
  SV->seq_num = g_tcp_layer_mss;
  SV->sent_packets++;
  
   SV->timer = rn_timer_init(lp, SV->rto);
   
   if(SV->timer){
     rn_message *msg;
     tcp_layer_message *M;
     msg = tw_event_data(SV->timer);
     M = rn_message_data(msg);
     M->source = SV->connection;
     M->seq_num = 0;
     M->MethodName = RTO;
   }
  
   
  //tcp_util_event(lp, RTO, SV->connection , lp->id, 0, 0, ts + SV->rto);
  
  SV->rtt_seq = 0;
  SV->rtt_time = 0;  
}

/*
 * EventHandler for a Host
 */

void
tcp_layer_event_handler(tcp_layer_state * SV, tw_bf * CV, rn_message * M, tw_lp * lp)
{
  tcp_layer_message    *msg;

  msg = rn_message_data(M);
  CV = &(msg->CV); 

  *(int *)CV = (int)0;

  
  switch (msg->MethodName)
    {
    case FORWARD:
      switch (SV->connection)
	{
	case -1:
	  tcp_layer_process_data(SV, CV, M, lp);
	  break;
	  
	default:
	  tcp_layer_process_ack(SV, CV, M, lp);
	  break;
	}
      break;
    case RTO:
      tcp_layer_timeout(SV, CV, msg, lp);
      break;
    case STARTUP:
      tcp_layer_startup(SV, CV, M, lp);
      break;
    default:
      tw_error(TW_LOC,
	       "APP_ERROR(%ld): Invalid MethodName(%d):Time (%f)\n",
	       lp->id, msg->MethodName, tw_now(lp));
    }
}

/*
 * RC EventHandler for a Host
 */

void
tcp_layer_rc_event_handler(tcp_layer_state * SV, tw_bf * CV, rn_message * M, tw_lp * lp)
{
  tcp_layer_message    *msg;
       
  msg = rn_message_data(M);
  
  CV = &(msg->CV);
   switch (msg->MethodName)
     {
     case FORWARD:
       switch (SV->connection)
	 {
	 case -1:
	  tcp_layer_process_data_rc(SV, CV, M, lp);
	  break;

	 default:
	   tcp_layer_process_ack_rc(SV, CV, M, lp);
	   break;
	 }
       break;
    case RTO:
      tcp_layer_timeout_rc(SV, CV, msg, lp);
      break;
     default:
       tw_error(TW_LOC,
		"APP_ERROR(%d): Invalid MethodName(%d):Time (%f)\n",
		lp->id, msg->MethodName, tw_now(lp));
     }
}


void
tcp_layer_final(tcp_layer_state * SV, tw_lp * lp)
{
  printf("%ld: TCP bad messages received = %d \n", lp->id, SV->bad_msgs);
  g_tcp_layer_stats.sent_packets += SV->sent_packets;
  g_tcp_layer_stats.received_packets += SV->received_packets;
  g_tcp_layer_stats.timedout_packets += SV->timedout_packets;
  
  if (SV->connection > -1){
    g_tcp_layer_stats.throughput += ((SV->unack / (tw_now(lp) - SV->start_time)) / 1000);
    
    printf("\nThroughput %f Kbps, Sent Packets %d, Timeouts %d, Cwnd %f\n",
	   (((SV->unack *(double) 8)/ (tw_now(lp) - SV->start_time)) / 1000) , 
	   SV->sent_packets, SV->timedout_packets, SV->cwnd);
  } 
}
