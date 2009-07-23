/*********************************************************************
		         tcp-host-rc.c
*********************************************************************/

#include "tcp-layer.h"


/*********************************************************************
                Receives the processing of an ack   
*********************************************************************/

void
tcp_layer_process_ack_rc(tcp_layer_state *SV,  tw_bf *CV, rn_message *msg, tw_lp *lp)
{
  int i; 
  int ack;
  tcp_layer_message    *M;
  // printf("%f roll ack %d %d lp %d\n",tw_now(lp), SV->unack, M->ack, lp->id);
 
  
  M = rn_message_data(msg);
  
  if(CV->c2)
    {
      for(i=0; i<M->seq_num; i++)
	{
	  //  tw_rand_reverse_unif(lp->id);
	  SV->seq_num -= g_tcp_layer_mss; 
	  SV->sent_packets--;
	}
      
      // reverse tcp_update_rtt 
      if(CV->c5 || CV->c7)
	{
	  
	  SV->rtt_seq = M->RC.rtt_seq; 
	  // M->RC.rtt_seq = SV->rtt_seq;
	  //SV->rtt_seq = M->ack;  // wrong CHANGE
	  SV->rtt_time = M->RC.rtt_time;
	  
	  if(CV->c6 && CV->c5)
	    SV->rto = M->dest;
	  SV->rto_seq--;
	  
	  if(CV->c13){
	    tcp_layer_message *M1;
	    rn_message *msg1;
	    
	    if(SV->timer)
	      tw_timer_reset(lp, &(SV->timer), M->RC.timer_ts);
	    else
	      SV->timer = tw_timer_init(lp, M->RC.timer_ts);
	    
	    msg1 = tw_event_data(SV->timer);
	    M1 = rn_message_data(msg1);
	    M1->seq_num = M->RC.timer_seq;
	    M1->source = M->source;
	    M1->MethodName = RTO;
	  }
	}

      
    
    // Reverse tcp_update_cwnd
    if(CV->c3 || CV->c4)
      SV->cwnd = M->RC.cwnd; 
  
    ack = SV->unack;
    SV->unack = M->ack;
    M->ack = ack - g_tcp_layer_mss;
    SV->dup_count = M->RC.dup_count;
    if(CV->c9)
      SV->seq_num = M->RC.seq_num;
  }
  else 
    {
      if(CV->c3)
	{
	  SV->dup_count -= 1;
	  if(CV->c4)
	    {
	      
	      SV->rtt_time = M->RC.rtt_time;
	      SV->rtt_seq  = M->RC.seq_num;//SV->unack; //big CHANGE 
	      //SV->timedout_packets--;
	      //SV->sent_packets--;
	      
	      SV->rto_seq--;
	      
	      SV->seq_num = M->ack;
	      M->ack = SV->unack - g_tcp_layer_mss;
	      
	      SV->cwnd = M->RC.cwnd;
	      SV->ssthresh = M->dest;

	      
	      if(CV->c13){
		//rn_message *msg1;
		tcp_layer_message *M1;
		if(SV->timer)
		  tw_timer_reset(lp, &(SV->timer), M->RC.timer_ts);
		else
		  SV->timer = tw_timer_init(lp, M->RC.timer_ts);
		M1 = tw_event_data(SV->timer);
		M1->seq_num = M->RC.timer_seq;
		M1->source = M->source;
		M1->MethodName = RTO;
	      }
	    }
	}
    }
}


/*********************************************************************
               Receives the processing of data packet
*********************************************************************/

void
tcp_layer_process_data_rc(tcp_layer_state *SV,  tw_bf *CV, rn_message *msg, tw_lp *lp)
{
  tcp_layer_message    *M;
  M = rn_message_data(msg);

  if(CV->c2)
    {
      // tw_rand_reverse_unif(lp->id);
      SV->received_packets--;
      SV->seq_num -= g_tcp_layer_mss;
      while(M->RC.dup_count){
      	SV->out_of_order[(SV->seq_num / (int) g_tcp_layer_mss) % g_tcp_layer_recv_wnd ] = 1;
      	SV->seq_num -= g_tcp_layer_mss;
      	M->RC.dup_count--;
      }
    }
  else
    {
      if(CV->c3){
	if(CV->c4) {} // need to think about this one  WILL NOT WORK UNTILL YOU FIX!!!!!
	else
	  SV->out_of_order[(M->seq_num / (int) g_tcp_layer_mss) % g_tcp_layer_recv_wnd] = 0;
      }
    }
}


/*********************************************************************
                 Receives the processing of a timeout    
*********************************************************************/

void 
tcp_layer_timeout_rc(tcp_layer_state *SV, tw_bf *CV, tcp_layer_message *M, tw_lp *lp)
{
  int ssthresh = M->dest;
  int seq_num = M->ack; 

  
  if(CV->c1)
    {
      SV->rtt_time = M->RC.rtt_time;
      SV->rtt_seq = M->RC.rtt_seq;//M->seq_num; CHANGE
      SV->rto_seq--;
      SV->timedout_packets--;
      //SV->sent_packets--;
      
      
      SV->rto = M->RC.dup_count;//SV->rto/2; //possible an error.
      SV->seq_num = seq_num;
      
      SV->cwnd = M->RC.cwnd;
      SV->ssthresh = ssthresh;
      
      tw_timer_cancel(lp, &(SV->timer));
      SV->timer = M->RC.timer;
    }
}







