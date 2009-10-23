#include <tcp-layer.h>

/*********************************************************************
                Processes a host's incoming acks
*********************************************************************/

void
tcp_layer_process_ack(tcp_layer_state * SV, tw_bf * CV, rn_message * msg, tw_lp * lp)
{
  int             ack;
  tw_stime        ts;
  tcp_layer_message    *M;
  
  ts = 0.0;
  M = rn_message_data(msg);
  
  
  if ((CV->c2 = (SV->unack <= M->ack)))
    {
        if ((CV->c9 = (M->ack >= SV->seq_num)))
	{						// CHANGE
	  M->RC.seq_num = SV->seq_num;
	  SV->seq_num = M->ack + g_tcp_layer_mss;
	}
      
      M->RC.dup_count = SV->dup_count;
      SV->dup_count = 0;
      
      ack = SV->unack;
      SV->unack = M->ack + g_tcp_layer_mss;
      tcp_layer_update_cwnd(SV, CV, M, lp);
      
      /*
       * Prints the time it took to transfer, the rate and congestion
       * window size     
       */

      if (SV->unack >= SV->len)
	{
	  printf("\n\t\tTransfer Time = %f at %f kbps \n\t\tCWND = %f lp = %ld\n\n",
	     (tw_now(lp) - SV->start_time) ,
	     ((SV->len) / (tw_now(lp) - SV->start_time))/ 1000, 
	     SV->cwnd, lp->id);	  
	}

      tcp_layer_update_rtt(SV, CV, M, lp);
      
      M->seq_num = 0;

      while (SV->seq_num < SV->len &&
	     (SV->seq_num + g_tcp_layer_mss) <=
	     (SV->unack + min(SV->cwnd, g_tcp_layer_recv_wnd) * g_tcp_layer_mss))
	{
	  M->seq_num++;
	  tcp_layer_util_event(lp, FORWARD, lp->id, M->source, SV->seq_num, 0, 
			       TCP_LAYER_TRANSFER_SIZE);
	  SV->sent_packets++;
	  SV->seq_num += g_tcp_layer_mss;
	  //if(SV->seq_num < 0 )
	  // tw_error(TW_LOC,"OverFlow\n");
	}
      M->ack = ack;
    } else
      {
	// printf("Invalid Ack %d\n", M->ack);
	if ((CV->c3 = ((SV->unack - g_tcp_layer_mss) == M->ack)))
	  {
	    SV->dup_count += 1;
	    if ((CV->c4 = (SV->dup_count == 4)))
	      {
		
		M->dest = SV->ssthresh;
		SV->ssthresh =
		  (min(((int)SV->cwnd + 1), g_tcp_layer_recv_wnd) / 2) * g_tcp_layer_mss;
				// CHANGED
		M->RC.cwnd = SV->cwnd;
		SV->cwnd = 1;

		tcp_layer_util_event(lp, FORWARD, lp->id,
			       M->source, SV->unack, 0, TCP_LAYER_TRANSFER_SIZE);
		
		M->ack = SV->seq_num;
		SV->seq_num = SV->unack + g_tcp_layer_mss;
		
				// CHANGE MAYBE.
		SV->rto_seq++;
		//tcp_layer_util_event(lp, RTO, M->source, lp->id, SV->unack,
		//	       SV->rto_seq, SV->rto);
		
		if((CV->c13 = (SV->timer != NULL))) {
		  rn_message *rn_m;
		  tcp_layer_message *m1;

		  rn_m = tw_event_data(SV->timer); 
		  m1 = rn_message_data(rn_m);
		  M->RC.timer_ts = SV->timer->recv_ts;
		  M->RC.timer_seq = m1->seq_num;
		  
		  rn_timer_reset(lp, &(SV->timer), tw_now(lp) + SV->rto);
		  
		  if(SV->timer) 
		    m1->seq_num = SV->unack;
		}

		M->RC.seq_num = SV->rtt_seq;
		SV->rtt_seq = SV->unack;
		M->RC.rtt_time = SV->rtt_time;
		SV->rtt_time = 0;
	      }
	  }
      }
}

/*********************************************************************
                  Processes incoming data for a host
*********************************************************************/

void
tcp_layer_process_data(tcp_layer_state * SV, tw_bf * CV, rn_message * rn_msg,
					  tw_lp * lp)
{
  //int             id_offset;
  tcp_layer_message    *M;
  
  tw_stime        ts;
  
  ts = 0.0;
  M = rn_message_data(rn_msg);
  //id_offset = lp->id;
  
  if ((CV->c2 = (M->seq_num == SV->seq_num)))
    {
      SV->received_packets++;
      
      M->RC.dup_count = 0;
      SV->seq_num += g_tcp_layer_mss;
      while (SV->out_of_order[(SV->seq_num / (int)g_tcp_layer_mss) % g_tcp_layer_recv_wnd])
	{	
	  M->RC.dup_count++;
	  SV->out_of_order[(SV->seq_num / (int)g_tcp_layer_mss) % g_tcp_layer_recv_wnd] = 0;
	  SV->seq_num += g_tcp_layer_mss;
	}
	  
      tcp_layer_util_event(lp, FORWARD, lp->id, M->source,
			   0, SV->seq_num - g_tcp_layer_mss, TCP_LAYER_HEADER_SIZE);
      
      // printf("SV->seq %d and rn_msg %ld\n",SV->seq_num, rn_msg->size);
      
	} 
  else
    { 
      if ((CV->c3 = (M->seq_num > SV->seq_num)))
	{
	  if ((CV->c4 = (M->seq_num > (SV->seq_num + g_tcp_layer_recv_wnd * g_tcp_layer_mss
				       /*
					* TCP_LAYER_SND_WND 
					*/  - g_tcp_layer_mss))))
	    {
				// look up need rc code  CHANGE
	      tw_error(TW_LOC, "revc_wnd buffer overflow %d %d\n",
		       M->seq_num, SV->seq_num + TCP_LAYER_SND_WND - g_tcp_layer_mss);
	    } 
	  else
	    {
	      /*
		printf("M %d window %d, loc %d\n",
		M->seq_num,
		(SV->seq_num + TCP_LAYER_SND_WND - g_tcp_layer_mss),
		  (M->seq_num / g_tcp_layer_mss) % g_tcp_layer_recv_wnd);
	      */
	      SV->out_of_order[(M->seq_num / g_tcp_layer_mss) % g_tcp_layer_recv_wnd] = 1;
	    }
	}
      
      tcp_layer_util_event(lp, FORWARD, lp->id, M->source,
			   0, SV->seq_num - g_tcp_layer_mss, TCP_LAYER_HEADER_SIZE);
      
    }
  
}


/*********************************************************************
                          Updates the cwnd
*********************************************************************/

void
tcp_layer_update_cwnd(tcp_layer_state * SV, tw_bf * CV, tcp_layer_message * M, tw_lp * lp)
{
  if (
      (CV->c3 =
       ((SV->cwnd * g_tcp_layer_mss) < SV->ssthresh && SV->cwnd * g_tcp_layer_mss < TCP_LAYER_SND_WND)))
    {
      M->RC.cwnd = SV->cwnd;
      SV->cwnd += 1;
    } else if ((CV->c4 = (SV->cwnd * g_tcp_layer_mss < TCP_LAYER_SND_WND)))
      {
	M->RC.cwnd = SV->cwnd;
	SV->cwnd += 1 / SV->cwnd;
      }

}


/*********************************************************************
                          Updates the rtt
*********************************************************************/

void
tcp_layer_update_rtt(tcp_layer_state * SV, tw_bf * CV, tcp_layer_message * M, tw_lp * lp)
{
  
  if ((CV->c5 = (M->ack == SV->rtt_seq && SV->unack < SV->len)))
    {
      if ((CV->c6 = (SV->rtt_time != 0)))
	{
	  M->dest = SV->rto;
	  SV->rto =
	    .875 * SV->rto + (10 * (tw_now(lp) - SV->rtt_time)) * .125;
	  
	}
      M->RC.rtt_seq = SV->rtt_seq;
      M->RC.rtt_time = SV->rtt_time;
      SV->rtt_time = tw_now(lp);
      SV->rtt_seq = SV->seq_num;
      
      SV->rto_seq++;
      
      if((CV->c13 = (SV->timer != NULL))) {
	rn_message *rn_m = tw_event_data(SV->timer);
	tcp_layer_message *m1 = rn_message_data(rn_m);
	M->RC.timer_ts = SV->timer->recv_ts;
	M->RC.timer_seq = m1->seq_num;
	
	rn_timer_reset(lp, &(SV->timer), tw_now(lp) + SV->rto);
	
	if(SV->timer) 
	  m1->seq_num = SV->unack;
      }

    } else if ((CV->c7 = (M->ack > SV->rtt_seq && SV->unack < SV->len)))
      {

	M->RC.rtt_seq = SV->rtt_seq;
	M->RC.rtt_time = SV->rtt_time;
	SV->rtt_time = tw_now(lp);
	SV->rtt_seq = SV->seq_num;
	
	SV->rto_seq++;
	
	if((CV->c13 = (SV->timer != NULL))) {
	  rn_message *rn_m = tw_event_data(SV->timer);
	  tcp_layer_message *m1 = rn_message_data(rn_m);
	  M->RC.timer_ts = SV->timer->recv_ts;
	  M->RC.timer_seq = m1->seq_num;
	  
	  rn_timer_reset(lp, &(SV->timer), tw_now(lp) + SV->rto);
		  
	  if(SV->timer) 
	    m1->seq_num = SV->unack;
	}
      }
}


/*********************************************************************
 Check if the timeout is valid, if so it restransmits the lost packet 
*********************************************************************/

void
tcp_layer_timeout(tcp_layer_state * SV, tw_bf * CV, tcp_layer_message * M, tw_lp * lp)
{
  
  // printf("bw %d de %f last %f \n", SV->host->link->bandwidth,  
  // SV->host->link->delay,  SV->lastsent);

  if ((CV->c1 = (M->seq_num >= SV->unack && SV->unack < SV->len)))
    {
      //printf("should timeout %f %d %f\n", tw_now(lp), lp->id, SV->lastsent );

      M->dest = SV->ssthresh;
      SV->ssthresh = (min(((int)SV->cwnd + 1), g_tcp_layer_recv_wnd) / 2) * g_tcp_layer_mss;
      // CHANGED
      M->RC.cwnd = SV->cwnd;
      SV->cwnd = 1;
      
      //printf("bw %d de %f last %f \n", SV->host->link->bandwidth,  
      //     SV->host->link->delay,  SV->lastsent);

      tcp_layer_util_event(lp, FORWARD, lp->id, M->source, SV->unack, 0,
		     TCP_LAYER_HEADER_SIZE);
      
      M->ack = SV->seq_num;
      SV->seq_num = SV->unack + g_tcp_layer_mss;
      SV->timedout_packets++;
      
      M->RC.dup_count = SV->rto;
      SV->rto = SV->rto * 2;	// PUT IN MAX TIME CHANGE
      // M->RC.event = SV->event;
      
      SV->rto_seq++;
      //tcp_layer_util_event(lp, RTO, M->source, lp->id, SV->unack, SV->rto_seq,
      //	     SV->rto);

      M->RC.timer = SV->timer;
      SV->timer = rn_timer_init(lp, tw_now(lp) + SV->rto);
      
      if((CV->c13 = (SV->timer != NULL))){
	rn_message *rn_m = tw_event_data(SV->timer);
	tcp_layer_message *M1 = rn_message_data(rn_m);
	M1->source = M->source;
	M1->seq_num= SV->unack;
	M1->MethodName = RTO;
      }
      
      M->RC.rtt_seq = SV->rtt_seq;
      SV->rtt_seq = SV->unack;
      M->RC.rtt_time = SV->rtt_time;
      SV->rtt_time = 0;
    }
}




