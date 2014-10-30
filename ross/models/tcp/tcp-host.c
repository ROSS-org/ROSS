/*********************************************************************
		              tcp-host.c
*********************************************************************/

#include "tcp.h"


/*********************************************************************
          Initializes a host and send appropriate messages
*********************************************************************/

void 
tcp_host_startup(Host_State *SV, tw_lp * lp)
{
  int    i;
  int    id = lp->id;
  int    id_offset = lp->id - g_routers;
  double  ts; 
  
  SV->sent_packets = 0;  
  SV->received_packets = 0;
  SV->timedout_packets = 0;
  SV->seq_num = 0;
  SV->len = 0;
  SV->unack = 0;
  SV->smoothed_rtt = 0;
  SV->dev = 0;

  SV->cwnd = 1;
  SV->ssthresh = TCP_SND_WND;
  SV->rto = 3 * g_frequency;
  SV->rto_seq = 0;
  SV->dup_count = 0;
  SV->rto_timer = NULL;
  SV->out_of_order = (short *) calloc(g_recv_wnd + 1, sizeof(short));

  switch (g_hosts_info[id - g_routers].type) 
    {
    case 0:
      ts = tw_rand_exponential(lp->rng, 10 * g_frequency);
    
      SV->start_time = ts;
      SV->len = 200000; // g_hosts_info[id_offset].size;
            
      SV->lastsent = ts += TCP_TRANSFER_SIZE / g_hosts_links[id_offset].link_speed ;
      ts += g_hosts_links[id_offset].delay;
      
      /*  Creates and event with: current lp, type, dest, 
	  seq_num, ack, and timestamp */
      tcp_util_event(lp, FORWARD, lp->id, tw_rand_integer(lp->rng,0,(g_hosts-1)/2) * 2 + g_routers, 0, 0, ts);

      
      SV->seq_num = g_mss;
      SV->sent_packets++;
      SV->rtt_seq = 0;
      SV->rtt_time = ts;
      break;

    case 1:      
      ts = tw_rand_exponential(lp->rng, 10);
    
      SV->start_time = ts;
      SV->len =   g_hosts_info[id_offset].size;
            
      SV->lastsent = ts += TCP_TRANSFER_SIZE / g_hosts_links[id_offset].link_speed ;
      ts += g_hosts_links[id_offset].delay;
      
      /*  Creates and event with: current lp, type, dest, 
	  seq_num, ack, and timestamp */
      tcp_util_event(lp, FORWARD, lp->id, g_hosts_info[id_offset].connected, 0, 0, ts);

      SV->seq_num = g_mss;
      SV->sent_packets++;
      SV->rto_timer = tw_timer_init(lp, ts + SV->rto);
      if(SV->rto_timer) {
	Msg_Data *M;
	M = tw_event_data(SV->rto_timer);
	M->source = g_hosts_info[id_offset].connected;
	M->seq_num= 0;
	M->MethodName = RTO;
      }
      
      SV->rtt_seq = 0;
      SV->rtt_time = ts;
      break;

    case 2:
      SV->connection = 0;
      for(i = 0 ; i < g_recv_wnd; i++)
	SV->out_of_order[i] = 0;
      break;
    }
}


/*********************************************************************
                Processes a host's incoming packets
*********************************************************************/

void
tcp_host_process(Host_State *SV,  tw_bf * CV,Msg_Data *M, tw_lp * lp)
{ 
  // need to check message type, but that's later.
  switch (g_hosts_info[lp->id - g_routers].type)
    {
    case 0:
      // not implemented
      tcp_host_process_ack(SV,CV,M,lp);
      break;
    case 1:
      tcp_host_process_ack(SV,CV,M,lp);
      break;
    case 2:
      if((CV->c10 = (SV->connection == 0))){
	SV->connection = M->source;
	tcp_host_process_data(SV,CV,M,lp);
      }
      else if ((CV->c11 = (SV->connection == M->source))){
	tcp_host_process_data(SV,CV,M,lp);
      }
      else 
	printf("here is the problem %d %d\n", SV->connection, M->source);
      break;
    }    
}


/*********************************************************************
                Processes a host's incoming acks
*********************************************************************/

void
tcp_host_process_ack(Host_State *SV,  tw_bf * CV, Msg_Data *M, tw_lp * lp)
{  
  int     id = lp->id;
  int     id_offset = lp->id - g_routers;
  int     ack;
  double  ts = 0;

  if((CV->c2 = (SV->unack <= M->ack)))
    {

      if((CV->c9 = (M->ack >= SV->seq_num))){   // CHANGE
	M->RC.seq_num = SV->seq_num;
	//printf("new seq %d \n",SV->seq_num);
	SV->seq_num = M->ack + g_mss;
      }

      M->RC.dup_count = SV->dup_count;
      SV->dup_count = 0;

      ack = SV->unack;
      SV->unack = M->ack + g_mss;
      tcp_host_update_cwnd(SV,CV,M,lp);
      
      /*      Prints the time it took to transfer, the rate and
	      congestion window size     */                    
      
      if(SV->unack >= SV->len)  
	printf("\nTransfer Time = %f at %f kbps \nCWND = %f lp = %d\n\n",
	       (tw_now(lp) - SV->start_time) / g_frequency, ((SV->len)/ 
	       (tw_now(lp) - SV->start_time)) * (((g_frequency)*8)/1000), SV->cwnd,id);
      
      tcp_host_update_rtt(SV,CV,M,lp);
      
      M->seq_num = 0;
      M->RC.lastsent = SV->lastsent;

      if(SV->lastsent <  tw_now(lp))
	SV->lastsent = tw_now(lp);
      else 
	ts = SV->lastsent - tw_now(lp);

      ts += g_hosts_links[id_offset].delay;
      while( SV->seq_num < SV->len  &&
	     (SV->seq_num + g_mss)  <= 
	     (SV->unack + ROSS_MIN(SV->cwnd,g_recv_wnd) * g_mss) )
	{ 
	  M->seq_num++;
	  SV->lastsent +=  TCP_TRANSFER_SIZE / g_hosts_links[id_offset].link_speed;
	  ts += TCP_TRANSFER_SIZE / g_hosts_links[id_offset].link_speed;
#ifdef LOGGING
	  fprintf(host_sent_log[lp->id - g_routers],"\tHOST SENT %f %f %f %d id%d\n",
		  ts,g_hosts_links[id_offset].link_speed, 
		  (TCP_TRANSFER_SIZE/g_hosts_links[id_offset].link_speed),
		  SV->seq_num , id);
	  fprintf(serv_tcpdump[lp->id - g_routers],"%f %d 10 %d 10001 %d %d 0 0 1 0\n",
		   tw_now(lp), id, 0, SV->seq_num + g_iss, g_mss ); 
#endif

	  tcp_util_event(lp, FORWARD, lp->id, M->source, SV->seq_num, 0, ts);
	  SV->sent_packets++;
	  SV->seq_num += g_mss;
	}
      M->ack = ack;
    }
  else
    {
      if((CV->c3 = ((SV->unack - g_mss) == M->ack)))
	{
	  SV->dup_count += 1;
	  if((CV->c4 = (SV->dup_count == 4)))
	    {
	      
	      M->dest = SV->ssthresh;
	      SV->ssthresh = (ROSS_MIN(((int) SV->cwnd + 1),g_recv_wnd) / 2) * g_mss;
	      //CHANGED
	      M->RC.cwnd = SV->cwnd;
	      SV->cwnd = 1;
	      
	      M->RC.lastsent = SV->lastsent;
	      if(SV->lastsent < tw_now(lp))
		SV->lastsent = tw_now(lp);
	      else
		ts = SV->lastsent - tw_now(lp);

	      SV->lastsent += TCP_TRANSFER_SIZE/g_hosts_links[id_offset].link_speed;
	      
	      ts += (TCP_TRANSFER_SIZE / g_hosts_links[id_offset].link_speed + 
		     g_hosts_links[id_offset].delay);
	      tcp_util_event(lp, FORWARD, lp->id, M->source, SV->unack, 0, ts);
	      
	      M->ack = SV->seq_num;
	      SV->seq_num = SV->unack + g_mss;      

#ifdef LOGGING    
	       fprintf(serv_tcpdump[lp->id - g_routers],"%f %d 10 %d 10001 %d %d 0 0 1 0\n",
		   tw_now(lp), id, 0, SV->unack + g_iss, g_mss ); 
#endif	      
	       // CHANGE MAYBE.
	       SV->rto_seq++;
	       //tcp_util_event(lp, RTO, M->source, lp->id, SV->unack, SV->rto_seq , SV->rto);
	       
	       if((CV->c13 = (SV->rto_timer != NULL))){
		 Msg_Data *M1 = tw_event_data(SV->rto_timer);
		 M->RC.timer_ts = SV->rto_timer->recv_ts;
		 M->RC.timer_seq = M1->seq_num;
		 
		 tw_timer_reset(lp, &(SV->rto_timer), tw_now(lp) + SV->rto);
	
		 if(SV->rto_timer) {
		   M1 = tw_event_data(SV->rto_timer);
		   M1->seq_num= SV->unack;
		 }
	       }


	       M->RC.seq_num = SV->rtt_seq;
	       SV->rtt_seq = SV->unack;
	       M->RC.rtt_time = SV->rtt_time;
	       SV->rtt_time = 0;  
	    }
	}
#ifdef LOGGING    
      fprintf(serv_cwnd[lp->id - g_routers],"%f %f %f %f 0\n",   //CHANGED 
	      tw_now(lp),SV->cwnd*g_mss,
	      SV->ssthresh, g_recv_wnd * g_mss);  

      fprintf(host_sent_log[lp->id - g_routers], "%f Wanted ack = %d, Recieved ack = %d\n",
	      tw_now(lp), SV->unack , M->ack); 
#endif
    }
}


/*********************************************************************
                  Processes incoming data for a host
*********************************************************************/

void
tcp_host_process_data(Host_State *SV,  tw_bf * CV,Msg_Data *M, tw_lp * lp)
{   
    int     id_offset = lp->id - g_routers;
    double  ts;

#ifdef LOGGING
  fprintf(host_received_log[lp->id - g_routers],"%f %f seq num %d message seq %d lp %d \n",
	  tw_now(lp), ts, SV->seq_num, M->seq_num, id);
#endif

#ifdef CLIENT_DROP
  if(g_hosts_info[id_offset].drop_index < g_hosts_info[id_offset].max && 
     g_hosts_info[id_offset].packets[g_hosts_info[id_offset].drop_index] == SV->count)
    {
      SV->count++;
      g_hosts_info[id_offset].drop_index++;
    }
  else
    {
      SV->count++;
#endif
      
    if((CV->c2 =(M->seq_num == SV->seq_num))) 
      {
	SV->received_packets++;    
	M->RC.lastsent = SV->lastsent;
	if(SV->lastsent < tw_now(lp)){
	  SV->lastsent = tw_now(lp);
	  ts =  g_hosts_links[id_offset].delay + TCP_HEADER_SIZE/g_hosts_links[id_offset].link_speed;
	}
	else{
	  printf("why 2\n");
	  ts = (SV->lastsent - tw_now(lp)) + TCP_HEADER_SIZE/g_hosts_links[id_offset].link_speed + 
	    g_hosts_links[id_offset].delay;
	  SV->lastsent += TCP_TRANSFER_SIZE/g_hosts_links[id_offset].link_speed;
	}
	//if(SV->lastsent < tw_now(lp))
	//	SV->lastsent = tw_now(lp);
	//SV->lastsent += 512/TCP_HOST_SPEED;
	//ts = TCP_LINK_DELAY + SV->lastsent - tw_now(lp);
	
#ifdef LOGGING
	fprintf(host_received_log[lp->id - g_routers],"%f %f seq num %d message seq %d lp %d \n",
		tw_now(lp), ts, SV->seq_num, M->seq_num, id);
#endif
	M->RC.dup_count = 0;
	SV->seq_num += g_mss;
	while(SV->out_of_order[(SV->seq_num  / (int) g_mss) % g_recv_wnd]){  // need to change
 	  M->RC.dup_count++;
	  SV->out_of_order[(SV->seq_num / (int) g_mss) % g_recv_wnd] = 0;
	  SV->seq_num += g_mss;
	}

	tcp_util_event(lp, FORWARD, lp->id, M->source, 0, SV->seq_num - g_mss, ts);    
	
      }
    else 
      {  
	M->RC.lastsent = SV->lastsent;
	if(SV->lastsent < tw_now(lp)){
	  SV->lastsent = tw_now(lp);
	  ts =  g_hosts_links[id_offset].delay + TCP_HEADER_SIZE/g_hosts_links[id_offset].link_speed;
	}
	else{
	  printf("why 1 %f %f %d\n", SV->lastsent, tw_now(lp), SV->seq_num);
	  ts = (SV->lastsent - tw_now(lp)) + TCP_HEADER_SIZE/g_hosts_links[id_offset].link_speed
	    + g_hosts_links[id_offset].delay;
	  SV->lastsent += TCP_HEADER_SIZE / g_hosts_links[id_offset].link_speed;
	}
	if((CV->c3 = (M->seq_num > SV->seq_num)))
	  {
	    if((CV->c4 = (M->seq_num > (SV->seq_num + (g_recv_wnd * g_mss)- g_mss))))
	      {  
	    // look up need rc code  CHANGE
	    printf("The revc_wnd buffer over flow %d %d\n",
		  M->seq_num, SV->seq_num + TCP_SND_WND - g_mss);
	    exit(0);
	    //for(i = 0; i < g_recv_wnd ; i++)
	    // SV->out_of_order[i] = 0 ;
	  }
	  else{
	    //   printf("M %d window %d, loc %d\n",M->seq_num,
	    //	   (SV->seq_num + TCP_SND_WND - g_mss), (M->seq_num / g_mss) % g_recv_wnd);
	    SV->out_of_order[(M->seq_num / g_mss) % g_recv_wnd] = 1;
	  }  
	}
	tcp_util_event(lp, FORWARD, lp->id, M->source , 0, SV->seq_num - g_mss, ts);
		
#ifdef LOGGING
	fprintf(host_received_log[lp->id - g_routers],
		"%f %f Received seq_num = %d, Wanted seq_num = %d \n", 
		tw_now(lp),ts,M->seq_num, SV->seq_num);
#endif
      }
    
#ifdef CLIENT_DROP
    }
#endif

}


/*********************************************************************
                          Updates the cwnd
*********************************************************************/

void
tcp_host_update_cwnd(Host_State *SV, tw_bf * CV,Msg_Data *M, tw_lp * lp)
{
  if((CV->c3 = ((SV->cwnd * g_mss ) < SV->ssthresh && SV->cwnd * g_mss < TCP_SND_WND))) 
    { 
      M->RC.cwnd = SV->cwnd;
      SV->cwnd += 1;
    }
  else if ((CV->c4 = (SV->cwnd * g_mss < TCP_SND_WND)))
    {
      M->RC.cwnd = SV->cwnd;
      SV->cwnd += 1/SV->cwnd; 
    }

#ifdef LOGGING
  fprintf(host_sent_log[lp->id - g_routers],"\tcwnd %f cwnd = %f ssth = %f\n",
	  tw_now(lp),SV->cwnd, SV->ssthresh);
  fprintf(serv_cwnd[lp->id - g_routers],"%f %f %f %f 0\n",
	  tw_now(lp), SV->cwnd*g_mss, 
	  SV->ssthresh, g_recv_wnd * g_mss);  
#endif
}
  

/*********************************************************************
                          Updates the rtt
*********************************************************************/

void
tcp_host_update_rtt(Host_State *SV, tw_bf * CV,Msg_Data *M, tw_lp * lp)
{
  double err;

#ifdef LOGGING 
  fprintf(host_sent_log[lp->id - g_routers],
	  "\trrt %f M->ack %d SV->rtt_seq %d SV->unack %d SV->len %d \n\t SV->seq_num %d\n",
	  tw_now(lp),M->ack, SV->rtt_seq, SV->unack, SV->len, SV->seq_num);
#endif 

  if((CV->c5 = (M->ack == SV->rtt_seq && SV->unack < SV->len)))
    {
      if((CV->c6 = (SV->rtt_time != 0))) 
	{
	  M->dest = SV->rto;
	  M->RC.smoothed_rtt = SV->smoothed_rtt;
	  M->RC.dev = SV->dev;

	  if(SV->smoothed_rtt){
	    err =  ceil(((tw_now(lp) - SV->rtt_time) / g_frequency) * 2 ) / 2.0 * g_frequency -  SV->smoothed_rtt;
	    SV->smoothed_rtt =  SV->smoothed_rtt + (.125 * err);
	    
	    if(SV->smoothed_rtt <= 0 )
	      SV->smoothed_rtt = .125 * g_frequency;
	    
	    SV->dev = SV->dev + .25 * (fabs(err) - SV->dev);
	    if(SV->dev <= 0)
	      SV->dev = .5 * g_frequency;
	    
	  }
	  else {
	    SV->smoothed_rtt = ceil(((tw_now(lp) - SV->rtt_time) / g_frequency) * 2) / 2.0 * g_frequency;
	    SV->dev =  SV->smoothed_rtt / 2.0;
	    //printf("smoothed_rtt %f SV->dev %f %f %f\n",SV->smoothed_rtt, SV->dev, ((tw_now(lp) - SV->rtt_time) / g_frequency) * 2,
	    //		   (tw_now(lp) - SV->rtt_time) / g_frequency);
	  }

#ifdef LOGGING
	  fprintf(host_sent_log[lp->id - g_routers],"\t%d %d %d %d \n", 
		  SV->unack, M->ack, SV->seq_num,  SV->rtt_seq);
#endif	
	  SV->rto = SV->smoothed_rtt + 4 * SV->dev;

	}
      
      M->RC.rtt_seq = SV->rtt_seq;
      M->RC.rtt_time = SV->rtt_time; 
      SV->rtt_time = tw_now(lp);
      SV->rtt_seq = SV->seq_num;	 
      
#ifdef LOGGING
      fprintf(host_sent_log[lp->id - g_routers],"\t\t%f RTO set for %d\n", 
	      SV->rto, SV->seq_num);
#endif
      SV->rto_seq++;
      //tcp_util_event(lp, RTO, M->source, lp->id, SV->seq_num, SV->rto_seq , SV->rto);
      
       
       if((CV->c13 = (SV->rto_timer != NULL))){
	 Msg_Data *M1 = tw_event_data(SV->rto_timer);
	 M->RC.timer_ts = SV->rto_timer->recv_ts;
	 M->RC.timer_seq = M1->seq_num;
	 
	 tw_timer_reset(lp, &(SV->rto_timer), tw_now(lp) + SV->rto);
	 
	 if(SV->rto_timer) {
	   M1 = tw_event_data(SV->rto_timer);
	   M1->seq_num= SV->seq_num;
	 }
       }
    }
  else if((CV->c7 = (M->ack > SV->rtt_seq && SV->unack < SV->len)))
    { 
      
      M->RC.rtt_seq = SV->rtt_seq;
      M->RC.rtt_time = SV->rtt_time; 
      SV->rtt_time = tw_now(lp);
      SV->rtt_seq = SV->seq_num;	 
      
#ifdef LOGGING
      fprintf(host_sent_log[lp->id - g_routers],"\t\t%f RTO set for %d\n", 
	      SV->rto+tw_now(lp), SV->seq_num);
#endif
      SV->rto_seq++;
      //tcp_util_event(lp, RTO, M->source, lp->id, SV->seq_num, SV->rto_seq, SV->rto);
      
      if((CV->c13 = (SV->rto_timer != NULL))){
	 Msg_Data *M1 = tw_event_data(SV->rto_timer);
	 M->RC.timer_ts = SV->rto_timer->recv_ts;
	 M->RC.timer_seq = M1->seq_num;
	 
	 tw_timer_reset(lp, &(SV->rto_timer), tw_now(lp) + SV->rto);
	 
	 if(SV->rto_timer) {
	   M1 = tw_event_data(SV->rto_timer);
	   M1->seq_num= SV->seq_num;
	 }
       }
    }
}


/*********************************************************************
 Check if the timeout is valid, if so it restransmits the lost packet 
*********************************************************************/

void 
tcp_host_timeout(Host_State *SV, tw_bf * CV,Msg_Data *M, tw_lp * lp)
{  
  int    id_offset = lp->id - g_routers;

  if((CV->c1 = (M->seq_num >= SV->unack && SV->unack < SV->len && M->ack == SV->rto_seq)))
    {

#ifdef LOGGING
     fprintf(host_sent_log[lp->id - g_routers],
	     "%f Timeout happened at time, Retransmitting %d lp %d\n", 
	     tw_now(lp), SV->unack,id);
     fprintf(serv_tcpdump[lp->id - g_routers],"%f %d 10 %d 10001 %d %d 0 0 1 0\n",
	     tw_now(lp), id, (id + 3) % 6, SV->unack + g_iss, g_mss );  
#endif

      M->dest = SV->cwnd;
      M->RC.cwnd = SV->ssthresh;
      SV->ssthresh = (ROSS_MIN(((int) SV->cwnd + 1),g_recv_wnd) / 2) * g_mss;
      //CHANGED
      SV->cwnd = 1;
      
#ifdef LOGGING        
      fprintf(host_sent_log[lp->id - g_routers],"\ttimeout ssthresh %f cwnd %f\n",
	      SV->cwnd, SV->ssthresh);
      fprintf(serv_cwnd[lp->id - g_routers],"%f %f %f %f 0\n",
	      tw_now(lp),SV->cwnd*g_mss, SV->ssthresh,
	      g_mss * g_recv_wnd);  
#endif      
      // need to add correct time stamp. 

      M->RC.lastsent = SV->lastsent;
      if(SV->lastsent < tw_now(lp))
	SV->lastsent = tw_now(lp);
      SV->lastsent += TCP_TRANSFER_SIZE/g_hosts_links[id_offset].link_speed;
      
      tcp_util_event(lp, FORWARD, lp->id, M->source, SV->unack, 0, 
		     SV->lastsent - tw_now(lp) + g_hosts_links[id_offset].delay);

      M->ack = SV->seq_num;
      SV->seq_num = SV->unack + g_mss;      
      SV->timedout_packets++;

      M->RC.dup_count = SV->rto;

      // Should be in here
      //      SV->dev += SV->smooth_rtt 

      M->RC.smoothed_rtt = SV->smoothed_rtt;
      SV->smoothed_rtt = 0;
      SV->rto = SV->rto*2;  // PUT IN MAX TIME CHANGE

      // M->RC.event = SV->event;
      
      SV->rto_seq++;
      // tcp_util_event(lp, RTO, M->source, lp->id, SV->unack, SV->rto_seq, SV->rto);
      
      M->RC.rto_timer = SV->rto_timer;
      SV->rto_timer = tw_timer_init(lp, tw_now(lp) + SV->rto);
      
      if((CV->c13 = (SV->rto_timer != NULL))){
	Msg_Data *M1 = tw_event_data(SV->rto_timer);
	M1->source = M->source;
	M1->seq_num= SV->unack;
	M1->MethodName = RTO;
      }
      
      M->RC.rtt_seq = SV->rtt_seq;
      SV->rtt_seq = SV->unack;
      M->RC.rtt_time = SV->rtt_time;
      SV->rtt_time = 0;  
    } 
  //else
  //  printf("shit\n");
}


/*********************************************************************
		      EventHandler for a Host
*********************************************************************/

void 
tcp_host_EventHandler(Host_State *SV, tw_bf * CV,Msg_Data *M, tw_lp * lp)
{
  *(int *)CV = (int)0;
  M->RC.cwnd = 0;
  //if(lp->id == 2 && tw_now(lp) > 71000 && tw_now(lp) < 73000){
  // printf("%f event lp %d %d method %d\n",tw_now(lp),lp->id, SV->unack,  M->MethodName);
  // if(SV->event)
  //   {
  //	tw_pq_print_info(SV->event->dest_lp->pe->pq, &(SV->event));
  //	printf("hello\n");
  //   }
  //}
  
  switch (M->MethodName)
    {
    case FORWARD:
      tcp_host_process(SV, CV, M, lp);
      break;
    case RTO:
      tcp_host_timeout(SV, CV, M, lp);
      break;
    default:
      tw_error(TW_LOC, "APP_ERROR(8)(%d): InValid MethodName(%d) : Time (%f)\n",
	       lp->id, M->MethodName,tw_now(lp));
      tw_exit(1);
    }
}


/*********************************************************************
     Collects Statistic for a Host when the simulation is over 
*********************************************************************/

extern tcpStatistics TWAppStats;

void 
tcp_host_Statistics_CollectStats(Host_State *SV, tw_lp * lp)
{
  TWAppStats.sent_packets += SV->sent_packets;
  TWAppStats.received_packets += SV->received_packets;
  TWAppStats.timedout_packets += SV->timedout_packets;

  if(SV->unack > 0)
    TWAppStats.throughput += ((SV->unack / ((tw_now(lp) - SV->start_time) / 
					    g_frequency))/1024);

}








