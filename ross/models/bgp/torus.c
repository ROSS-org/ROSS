// Ning's version
#include "bgp.h"

void
torus_setup(CN_state * s, tw_lp * lp)
{
  int i, j;
  int dim_N[N_dims+1];
  printf("N_dims is %d\n",N_dims);
  dim_N[0]=(int)lp->gid;
  printf("N_dim00 is %d\n",N_dims);

  // calculate my torus co-ordinates
  for (i=0; i<N_dims; i++)
    {
      s->dim_position[i] = dim_N[i]%dim_length[i];
      dim_N[i+1] = ( dim_N[i] - s->dim_position[i] )/dim_length[i];

      half_length[i] = dim_length[i]/2;
    }

  for (i=0; i<N_dims; i++)
    {
      s->node_queue_length[0][i] = 0;
      s->node_queue_length[1][i] = 0;
    }

  int factor[N_dims];
  factor[0]=1;
  for (i=1; i<N_dims; i++)
    {
      factor[i]=1;
      for (j=0; j<i; j++)
        factor[i]*=dim_length[j];
    }

  int temp_dim_pos[N_dims];
  for (i=0; i<N_dims; i++)
    temp_dim_pos[i] = s->dim_position[i];

  // calculate minus neighbour's lpID
  for (j=0; j<N_dims; j++)
    {
      temp_dim_pos[j] = (s->dim_position[j] -1 + dim_length[j])%dim_length[j];
      s->neighbour_minus_lpID[j]=0;
      for (i=0; i<N_dims; i++)
        s->neighbour_minus_lpID[j]+=factor[i]*temp_dim_pos[i];
      temp_dim_pos[j] = s->dim_position[j];
    }

  // calculate plus neighbour's lpID
  for (j=0; j<N_dims; j++)
    {
      temp_dim_pos[j] = (s->dim_position[j] + 1 + dim_length[j])%dim_length[j];
      s->neighbour_plus_lpID[j]=0;
      for (i=0; i<N_dims; i++)
        s->neighbour_plus_lpID[j]+=factor[i]*temp_dim_pos[i];
      temp_dim_pos[j] = s->dim_position[j];
    }

  // record LP time
  s->packet_counter = 0;
  s->next_available_time = 0;
  s->N_wait_to_be_processed = 0;
}


void
torus_init(CN_state * s, tw_lp * lp)
{
  tw_event *e;
  tw_stime ts;
  MsgData *m;

  /*
   * Set up the initial state for each LP (node)
   * according to the number of dimensions
   */
  //torus_setup(s, lp);

  /*
   * Start a GENERATE event on each LP
   */
  //ts = tw_rand_exponential(lp->rng, MEAN_INTERVAL);
  ts = 10000;
  e = tw_event_new(lp->gid, ts, lp);
  m = tw_event_data(e);
  m->type = GENERATE;

  m->dest_lp = lp->gid;
  tw_event_send(e);

}


void
packet_arrive(CN_state * s, tw_bf * bf, MsgData * msg, tw_lp * lp)
{
  int i;
  tw_stime ts;
  tw_event *e;
  MsgData *m;

  bf->c2 = 1;
  bf->c6 = 1;
  bf->c10 = 1;

  if( msg->packet_ID == TRACK )
    {
      printf( "packet %lld has arrived\n", 
	      msg->packet_ID );
      for( i = 0; i < N_dims; i++ )
	printf( "the %d dim position is %d\n",
		i,
		s->dim_position[i]);
      printf("packet %lld destination is\n",
	     msg->packet_ID);
      for( i = 0; i < N_dims; i++ )
	printf(" the %d dim position is %d\n", 
	       i,
	       msg->dest[i]);
      printf("lp time is %f travel start time is %f\n",
	     tw_now(lp),
	     msg->travel_start_time);
      printf("My hop now is %d\n",msg->my_N_hop);
      //printf("accumulated queue length is %lld\n",
      //msg->accumulate_queue_length);
      printf("\n");
    }

  // Packet arrives and accumulate # queued
  msg->queueing_times += s->N_wait_to_be_processed;
  // One more arrives and wait to be processed
  s->N_wait_to_be_processed++;

    msg->my_N_hop++;
    msg->my_N_queue+=s->node_queue_length[msg->source_direction][msg->source_dim];

    /*
     * If no packet is queueing, router is available now
     * if there are some packets queueing, then available time > tw_now(lp)
     * add one more MEAN_PROCESS to available time
     */

    msg->saved_available_time = s->next_available_time;
    s->next_available_time = max(s->next_available_time, tw_now(lp));
    // consider 1% noise on packet header parsing
    ts = tw_rand_exponential(lp->rng, (double)MEAN_PROCESS/1000)+MEAN_PROCESS;
    //ts = MEAN_PROCESS;


    //s->queue_length_sum += 
    //s->node_queue_length[msg->source_direction][msg->source_dim];
    s->node_queue_length[msg->source_direction][msg->source_dim]++;

    e = tw_event_new(lp->gid, s->next_available_time - tw_now(lp), lp);

    s->next_available_time += ts;    

    m = tw_event_data(e);
    m->type = PROCESS;
    
    //carry on the message info	
    for( i = 0; i < N_dims; i++ )
      m->dest[i] = msg->dest[i];
    m->dest_lp = msg->dest_lp;
    m->transmission_time = msg->transmission_time;
    
    m->source_dim = msg->source_dim;
    m->source_direction = msg->source_direction;
    
    m->packet_ID = msg->packet_ID;	  
    m->travel_start_time = msg->travel_start_time;

    m->my_N_hop = msg->my_N_hop;
    m->my_N_queue = msg->my_N_queue;
    m->queueing_times = msg->queueing_times;
    
    tw_event_send(e);
    //  }
}

void 
dimension_order_routing(CN_state * s, MsgData * msg, tw_lpid * dst_lp)
{
  int i;
  for( i = 0; i < N_dims; i++ )
    {
      if ( s->dim_position[i] - msg->dest[i] > half_length[i] )
	{
	  *dst_lp = s->neighbour_plus_lpID[i];
	  s->source_dim = i;
	  s->direction = 1;
	  break;
	}
      if ( s->dim_position[i] - msg->dest[i] < -half_length[i] )
	{
	  *dst_lp = s->neighbour_minus_lpID[i];
	  s->source_dim = i;
	  s->direction = 0;
	  break;
	}
      if (( s->dim_position[i] - msg->dest[i] <= half_length[i] )&&(s->dim_position[i] - msg->dest[i] > 0))
	{
	  *dst_lp = s->neighbour_minus_lpID[i];
	  s->source_dim = i;
	  s->direction = 0;
	  break;
	}
      if (( s->dim_position[i] - msg->dest[i] >= -half_length[i] )&&(s->dim_position[i] - msg->dest[i] < 0))
	{
	  *dst_lp = s->neighbour_plus_lpID[i];
	  s->source_dim = i;
	  s->direction = 1;
	  break;
	}
    }
  
}

void
packet_send(CN_state * s, tw_bf * bf, MsgData * msg, tw_lp * lp)
{
  int i;
  tw_lpid dst_lp;
  tw_stime ts;
  tw_event *e;
  MsgData *m;

  ts = LINK_DELAY+PACKET_SIZE;//msg->transmission_time;
  
  /*
   * Routing in the torus, start from the first dimension
   */
  
  if (lp->gid !=  msg->dest_lp )  
    {
      bf->c3 = 1;
      dimension_order_routing(s,msg,&dst_lp);
      msg->source_dim = s->source_dim;
      msg->source_direction = s->direction;
    }
  else
    {
      bf->c3 = 0;
      dst_lp = lp->gid;
    }
  
  //////////////////////////////////////////
  /*
  e = tw_event_new(dst_lp, ts, lp);
  m = tw_event_data(e);
  m->type = ARRIVAL;
  */
  ////////////////////////////////////////////////////////////////////
  msg->saved_source_dim = s->source_dim;
  msg->saved_direction = s->direction;
  msg->saved_link_available_time[s->direction][s->source_dim] = 
    s->next_link_available_time[s->direction][s->source_dim];

  s->next_link_available_time[s->direction][s->source_dim] = 
    max(s->next_link_available_time[s->direction][s->source_dim], tw_now(lp));
  // consider 1% noise on packet header parsing
  //ts = tw_rand_exponential(lp->rng, (double)MEAN_PROCESS/1000)+MEAN_PROCESS;
  ts = tw_rand_exponential(lp->rng, (double)LINK_DELAY/1000)+
    LINK_DELAY+PACKET_SIZE;
  
  //s->queue_length_sum += 
  //s->node_queue_length[msg->source_direction][msg->source_dim];
  //s->node_queue_length[msg->source_direction][msg->source_dim]++;

  s->next_link_available_time[s->direction][s->source_dim] += ts;      

  e = tw_event_new( dst_lp, 
		    s->next_link_available_time[s->direction][s->source_dim] 
		    - tw_now(lp), 
		    lp);

  m = tw_event_data(e);
  m->type = ARRIVAL;
  
  ///////////////////////////////////////////////////////////////
  
  // Carry on the message info
  for( i = 0; i < N_dims; i++ )
    m->dest[i] = msg->dest[i];
  m->dest_lp = msg->dest_lp;
  m->transmission_time = msg->transmission_time;
  
  m->source_dim = msg->source_dim;
  m->source_direction = msg->source_direction;
  
  m->packet_ID = msg->packet_ID;	  
  m->travel_start_time = msg->travel_start_time;

  m->my_N_hop = msg->my_N_hop;
  m->my_N_queue = msg->my_N_queue;
  m->queueing_times = msg->queueing_times;
  
  tw_event_send(e);
}



void
packet_process( CN_state * s, tw_bf * bf, MsgData * msg, tw_lp * lp)
{
  int i;
  tw_event *e;
  MsgData *m;

  bf->c3 = 1;
  // One packet leaves the queue
  s->node_queue_length[msg->source_direction][msg->source_dim]--;
  s->N_wait_to_be_processed--;
  
  if(lp->gid==msg->dest_lp)
    {
      // one packet arrives and dies
      bf->c3 = 0;
	N_finished++;
	int index = floor(N_COLLECT_POINTS*(tw_now(lp)/g_tw_ts_end));
	N_finished_storage[index]++;
	total_time += tw_now(lp) - msg->travel_start_time;
	if (max_latency<tw_now(lp) - msg->travel_start_time)
	  max_latency=tw_now(lp) - msg->travel_start_time;
	total_hops += msg->my_N_hop;
	total_queue_length += msg->my_N_queue;
	queueing_times_sum += msg->queueing_times;
	//total_queue_length += msg->accumulate_queue_length;
    }
  else
    {
      e = tw_event_new(lp->gid, MEAN_PROCESS, lp);
      m = tw_event_data(e);
      m->type = SEND;
      
      // Carry on the message info
      for( i = 0; i < N_dims; i++ )
	m->dest[i] = msg->dest[i];
      m->dest_lp = msg->dest_lp;
      m->transmission_time = msg->transmission_time;
      
      m->source_dim = msg->source_dim;
      m->source_direction = msg->source_direction;
      
      m->packet_ID = msg->packet_ID;	  
      m->travel_start_time = msg->travel_start_time;

      m->my_N_hop = msg->my_N_hop;
      m->my_N_queue = msg->my_N_queue;
      m->queueing_times = msg->queueing_times;

      tw_event_send(e);
      
    }
  
}

void 
packet_generate(CN_state * s, tw_bf * bf, MsgData * msg, tw_lp * lp)
{
  //unsigned long long i;
  int i;
  tw_lpid dst_lp;
  tw_stime ts;
  tw_event *e;
  MsgData *m;

  // Send the packet out
  e = tw_event_new(lp->gid, 0, lp);
  m = tw_event_data(e);
  m->type = SEND;
  m->transmission_time = PACKET_SIZE;
  
  // Set up random destination
  dst_lp = tw_rand_integer(lp->rng,0,N_nodes-1);
  //rand_total += dst_lp;
  int dim_N[N_dims];
  dim_N[0]=dst_lp;
    
  for (i=0; i<N_dims; i++)
    {
      m->dest[i] = dim_N[i]%dim_length[i];
      dim_N[i+1] = ( dim_N[i] - m->dest[i] )/dim_length[i];
    }

  // record start time
  m->travel_start_time = tw_now(lp);
  m->my_N_queue = 0;
  m->my_N_hop = 0;
  m->queueing_times = 0;
  
  // set up packet ID
  // each packet has a unique ID
  m->packet_ID = lp->gid + g_tw_nlp*s->packet_counter;
  
  m->dest_lp = dst_lp;
  tw_event_send(e);	    

  // One more packet is generating 
  s->packet_counter++;
  int index = floor(N_COLLECT_POINTS*(tw_now(lp)/g_tw_ts_end));
  N_generated_storage[index]++;

  // schedule next GENERATE event
  ts = tw_rand_exponential(lp->rng, MEAN_INTERVAL);	
  //ts = MEAN_INTERVAL;
  e = tw_event_new(lp->gid, ts, lp);
  m = tw_event_data(e);
  m->type = GENERATE;
  
  m->dest_lp = lp->gid;
  tw_event_send(e);

}
