/*********************************************************************
		         tcp-host-rc.c
*********************************************************************/

#include "tcp.h"


/*********************************************************************
              Receives the processing of a packet 
*********************************************************************/

void
tcp_host_process_rc(Host_State *SV,  tw_bf *CV, Msg_Data *M, tw_lp *lp)
{ 

  switch (g_hosts_info[lp->id - g_routers].type)
    {
    case 0:
      // not implemented
      tcp_host_process_ack_rc(SV,CV,M,lp);
      break;
    case 1:
      tcp_host_process_ack_rc(SV,CV,M,lp);
      break;

    case 2:
      if(CV->c10){
	tcp_host_process_data_rc(SV,CV,M,lp);
	SV->connection = 0;
      }
      else if(CV->c11)
	tcp_host_process_data_rc(SV,CV,M,lp);
      break;

#ifdef CLIENT_DROP  // copy above
    case 3:
      tcp_host_process_data_rc(SV,CV,M,lp);
      break;
#endif

    }    
}


/*********************************************************************
                Receives the processing of an ack   
*********************************************************************/

void
tcp_host_process_ack_rc(Host_State *SV,  tw_bf *CV, Msg_Data *M, tw_lp *lp)
{
  int i;
  int w_time = M->seq_num;
  int ack;
  // printf("%f roll ack %d %d lp %d\n",tw_now(lp), SV->unack, M->ack, lp->id);
 
  if(CV->c2)
    {
      for(i=0; i<w_time; i++)
	{
	  //  tw_rand_reverse_unif(lp->id);
	  SV->seq_num -= g_mss; 
	  SV->sent_packets--;
	}
      SV->lastsent = M->RC.lastsent;
  
  // reverse tcp_update_rtt 
    if(CV->c5 || CV->c7)
      {
	
	SV->rtt_seq = M->RC.rtt_seq; 
	// M->RC.rtt_seq = SV->rtt_seq;
	//SV->rtt_seq = M->ack;  // wrong CHANGE
	SV->rtt_time = M->RC.rtt_time;

	if(CV->c6 && CV->c5){
	  SV->rto = M->dest;
	  SV->smoothed_rtt = M->RC.smoothed_rtt;
	  SV->dev = M->RC.dev;
	}
	
	if(CV->c13){
	  Msg_Data *M1;
	  if(SV->rto_timer)
	    tw_timer_reset(lp, &(SV->rto_timer), M->RC.timer_ts);
	  else
	    SV->rto_timer = tw_timer_init(lp, M->RC.timer_ts);
	  M1 = tw_event_data(SV->rto_timer);
	  M1->seq_num = M->RC.timer_seq;
	  M1->source = M->source;
	  M1->MethodName = RTO;
	}
      }
    
    // Reverse tcp_update_cwnd
    if(CV->c3 || CV->c4)
      SV->cwnd = M->RC.cwnd; 
  
    ack = SV->unack;
    SV->unack = M->ack;//512;
    M->ack = ack - g_mss;
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
	      
	      SV->seq_num = M->ack;
	      M->ack = SV->unack - g_mss;

	      SV->lastsent = M->RC.lastsent;
	      SV->cwnd = M->RC.cwnd;
	      SV->ssthresh = M->dest;
	      
	      if(CV->c13){
		Msg_Data *M1;
		if(SV->rto_timer)
		  tw_timer_reset(lp, &(SV->rto_timer), M->RC.timer_ts);
		else
		  SV->rto_timer = tw_timer_init(lp, M->RC.timer_ts);
		M1 = tw_event_data(SV->rto_timer);
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
tcp_host_process_data_rc(Host_State *SV,  tw_bf *CV, Msg_Data *M, tw_lp *lp)
{

  //printf("%f \n", M->RC.lastsent);
  if(CV->c2)
    {
      // tw_rand_reverse_unif(lp->id);
      SV->lastsent = M->RC.lastsent;
      SV->received_packets--;
      SV->seq_num -= g_mss;
      while(M->RC.dup_count){
      	SV->out_of_order[(SV->seq_num / (int) g_mss) % g_recv_wnd ] = 1;
      	SV->seq_num -= g_mss;
      	M->RC.dup_count--;
      }
    }
  else
    {
      SV->lastsent = M->RC.lastsent;
      if(CV->c3){
	if(CV->c4) {
	} // need to think about this one  WILL NOT WORK UNTILL YOU FIX!!!!!
	else
	  SV->out_of_order[(M->seq_num / (int) g_mss) % g_recv_wnd] = 0;
      }
    }
}


/*********************************************************************
                 Receives the processing of a timeout    
*********************************************************************/

void 
tcp_host_timeout_rc(Host_State *SV, tw_bf *CV, Msg_Data *M, tw_lp *lp)
{
  int seq_num = M->ack; 

  if(CV->c1)
    {
      SV->lastsent = M->RC.lastsent;
      SV->rtt_time = M->RC.rtt_time;
      SV->rtt_seq = M->RC.rtt_seq;//M->seq_num; CHANGE
      SV->timedout_packets--;
      //SV->sent_packets--;
      
      SV->smoothed_rtt = M->RC.smoothed_rtt;
      SV->rto = M->RC.dup_count;//SV->rto/2; //possible an error.
      SV->seq_num = seq_num;
      
      SV->cwnd = M->dest;
      SV->ssthresh = M->RC.cwnd;
      
      tw_timer_cancel(lp, &(SV->rto_timer));
      SV->rto_timer = M->RC.rto_timer;
    }
}


/*********************************************************************
               EventHandler for receiving Host events 
*********************************************************************/

void 
tcp_host_rc_EventHandler(Host_State *SV, tw_bf *CV, Msg_Data *M, tw_lp *lp)
{
  //  if(lp->id == 1)
  //  printf(" rollback %f event lp %d %d method %d\n",
  //	 tw_now(lp),lp->id, SV->unack,  M->MethodName);
  switch (M->MethodName)
    {
    case FORWARD:
      tcp_host_process_rc(SV, CV, M, lp);
      break;
    case RTO:
      tcp_host_timeout_rc(SV, CV, M, lp);
      break;
    default:
      tw_error(TW_LOC, "APP_ERROR(8)(%d): InValid MethodName(%d)\n",
	       lp->id, M->MethodName);
      tw_exit(1);
    }
}






