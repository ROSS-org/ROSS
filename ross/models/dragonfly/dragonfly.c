#include "dragonfly.h"

// Local router ID: 0 --- total_router-1
// Router LP ID 
// Terminal LP ID

////////////////////////////////////////////////// Router-Group-Terminal mapping functions ///////////////////////////////////////

FILE * dragonfly_event_log=NULL;
int 
get_terminal_rem()
{
   if(terminal_rem > 0 && terminal_rem <= g_tw_mynode)
    return terminal_rem;

   return 0;
}

int 
get_router_rem()
{
  if(router_rem > 0 && router_rem <= g_tw_mynode)
   return router_rem;

  return 0;
}

int 
get_inv_router_rem()
{
   if(router_rem > 0 && router_rem >= g_tw_mynode)
	return router_rem;
 
  return 0;
}
int 
getTerminalID(tw_lpid lpid)
{
    return lpid - total_routers;
}

int 
getProcID(tw_lpid lpid)
{
    return lpid - total_routers - total_terminals;
}

int 
getRouterID(tw_lpid terminal_id)
{
    int tid = getTerminalID(terminal_id);

    return tid/NUM_TERMINALS; 
}

tw_peid 
mapping( tw_lpid gid)
{
   int rank;
   int offset;
   int rem = 0;
   int nlp_per_pe;
   int N_nodes = tw_nnodes();

   if(gid < total_routers)
    {
       rank = gid / nlp_router_per_pe;
       rem = router_rem;

       if(nlp_router_per_pe == (total_routers/N_nodes))
            offset = (nlp_router_per_pe + 1) * router_rem;
       else
	    offset = nlp_router_per_pe * router_rem;

       nlp_per_pe = nlp_router_per_pe;
    }
    else if( gid >= total_routers && gid < total_routers + total_terminals)
     {
       rank = getTerminalID(gid)/nlp_terminal_per_pe;	
       rem = terminal_rem;

       if(nlp_terminal_per_pe == (total_terminals/N_nodes))
            offset = total_routers + (nlp_terminal_per_pe + 1) * terminal_rem;
       else
  	    offset = total_routers + nlp_terminal_per_pe * terminal_rem;     

       nlp_per_pe = nlp_terminal_per_pe;
     }
     else if( gid >= (total_routers + total_terminals) && gid < (total_routers + total_terminals + total_mpi_procs))
	{
	  rank = getProcID(gid)/nlp_mpi_procs_per_pe;
	  rem = terminal_rem; //same as MPI process rem as there is one to one mapping between MPI process and terminal

	  if(nlp_mpi_procs_per_pe == (total_mpi_procs/N_nodes))
            offset = total_routers + total_terminals + (nlp_mpi_procs_per_pe + 1) * terminal_rem;
          else
            offset = total_routers + total_terminals + nlp_mpi_procs_per_pe * terminal_rem;

          nlp_per_pe = nlp_mpi_procs_per_pe;
	}
      else
	  printf("\n Invalid LP ID %d given for mapping ", (int)gid);

   if(rem > 0)
    {
     if(g_tw_mynode >= rem)
        {
	  if(gid < offset) 
	    rank = gid / (nlp_per_pe + 1);
	  else
	    rank = rem + ((gid - offset)/nlp_per_pe);
	}
     else
 	 {
	   if(gid >= offset)
	     rank = rem + ((gid - offset)/(nlp_per_pe - 1)); 
	 }
    }
   return rank;
 }
//////////////////////////////////////// Get router in the group which has a global channel to group id gid /////////////////////////////////
tw_lpid 
getRouterFromGroupID(int gid, 
		    router_state * r)
{
  int group_begin=r->group_id*NUM_ROUTER;
  int group_end=(r->group_id*NUM_ROUTER) + NUM_ROUTER-1;
  int offset=(gid*NUM_ROUTER-group_begin)/NUM_ROUTER;
  
  if((gid * NUM_ROUTER) < group_begin)
    offset=(group_begin-gid*NUM_ROUTER)/NUM_ROUTER; // take absolute value
  
  int half_channel=GLOBAL_CHANNELS/2;
  int index=(offset-1)/(half_channel * NUM_ROUTER);
  
  offset=(offset-1)%(half_channel * NUM_ROUTER);

  // If the destination router is in the same group
  tw_lpid router_id;

  if(index%2 != 0)
    router_id=group_end - (offset/half_channel); // start from the end
  else
    router_id=group_begin + (offset/half_channel);

  return router_id;
}	

/////////////////////////////////////////////////////////////////MPI related functions /////////////////////////////////////////////////////

/* Initializes the MPI process LP */
void 
mpi_init( process_state * s, 
	       tw_lp * lp )
{
    tw_event *e;
    tw_stime ts;
    terminal_message *m;

      /** Start a GENERATE event on each LP **/
    ts = tw_rand_exponential(lp->rng, (double)MEAN_INTERVAL);

    e = tw_event_new(lp->gid, ts, lp);

    m = tw_event_data(e);
    m->type = MPI_SEND;
    tw_event_send(e);

    s->message_counter = 0;
    s->router_id = (lp->gid - total_routers - total_terminals) / NUM_TERMINALS;
    s->group_id = (lp->gid - total_routers - total_terminals) / (NUM_TERMINALS * NUM_ROUTER);

    s->row = getProcID(lp->gid) / NUM_ROWS;
    s->col = getProcID(lp->gid) % NUM_COLS;
}

/* Starts generating MPI messages at the MPI process LP according to the traffic pattern */
void 
mpi_msg_send( process_state * s, 
	      tw_bf * bf, 
	      terminal_message * msg,
	      tw_lp * lp )
{
  tw_lpid dst_lp;
  tw_stime ts;
  tw_event *e;
  terminal_message *m;
  int i;
   
  int terminal_grp_id = ((int)lp->gid - total_routers - total_terminals)/ (NUM_TERMINALS * NUM_ROUTER);
  int offset = NUM_TERMINALS * NUM_ROUTER - 1;
  bf->c1 = 0;
  bf->c3 = 0;
  bf->c2 = 0;
 
  if(s->message_counter >= max_packets)
    {
        bf->c1 = 1;
	return;
    }

   switch(traffic)
    {
	/* bisection traffic pattern sends messages to another randomly selected dragonfly group. It makes sure that the traffic always traverses the global channel*/
	case BISECTION:
	   {
		 bf->c3 = 1;
		 bf->c2 = 1;
		 int group_id = tw_rand_integer(lp->rng, 0, num_groups);
		
		 if(group_id == s->group_id)
			group_id = (s->group_id + (num_groups/2)) % num_groups;

  		 int next_group_begin = group_id * NUM_TERMINALS * NUM_ROUTER;

		 dst_lp = tw_rand_integer(lp->rng, total_routers + next_group_begin, total_routers + next_group_begin + offset);

	/*	 if(dst_lp == lp->gid)
 		  {
			dst_lp = total_routers + next_group_begin + (lp->gid - total_routers) % (NUM_TERMINALS * NUM_ROUTER);	
		  }*/
	   }
	  break;

	/* Worst case traffic pattern always sends messages to the same dragonfly group. This occupies the single global channel connecting the
two groups which is why this traffic is called worst-case traffic */
	case WORST_CASE:
	  {
		bf->c3 = 1;

		int next_group_begin = ((s->group_id + 1) % num_groups) * NUM_TERMINALS * NUM_ROUTER;

		dst_lp = tw_rand_integer(lp->rng, total_routers + next_group_begin, total_routers + next_group_begin + offset);
	  }
	break;
/* uniform random sends MPI messages to a randomly selected compute node/terminal from the entire network */
	case UNIFORM_RANDOM:
	   {
		bf->c3 = 1;

		dst_lp = tw_rand_integer(lp->rng, total_routers, total_routers+total_terminals-1);
		
		if(dst_lp == lp->gid)
		  {
		   dst_lp = total_routers + (s->message_counter % total_terminals);
		  }
	   }
	 break;

	case TRANSPOSE:
	   {
		if( s->col == s->row )
		  {
		   bf->c1 = 1;

		   return;
		  }
	        dst_lp = total_routers +  (s->col * NUM_ROWS + s->row);
	   }
	 break;
/* Sends messages to compute nodes connected to the same dragonfly router */
      case NEAREST_NEIGHBOR:
	 {
	   //bf->c3 = 1;
	  
	   int local_offset = (lp->gid - total_routers) % NUM_TERMINALS;
	   int group_begin = total_routers + (s->router_id * NUM_TERMINALS);
	   dst_lp = group_begin + (NUM_TERMINALS - local_offset);
	   if(dst_lp < group_begin || dst_lp > group_begin + offset)
		printf("\n Incorrect destination for %d to %d ", (int)lp->gid - total_routers, (int)dst_lp - total_routers);
//	   dst_lp = total_routers + tw_rand_integer(lp->rng, s->router_id * NUM_TERMINALS, (s->router_id + 1) * NUM_TERMINALS -1);
	
	   //if(dst_lp == lp->gid)
	   //  dst_lp = total_routers + s->router_id * NUM_TERMINALS + ((lp->gid+1) % NUM_TERMINALS);	  

	   /*if(dst_lp == lp->gid)
	    {
	//	bf->c2 = 0;

	//	dst_lp = total_routers + tw_rand_integer(lp->rng, s->router_id * NUM_TERMINALS, (s->router_id + 1) * NUM_TERMINALS -1);
		dst_lp = total_routers + ((lp->gid+1) % total_terminals);
	    }*/
	}
	break;
/* Sends messages to compute nodes connected to nearest dragonfly router */
    case NEAREST_ROUTER:
	{
		int local_offset = (lp->gid - total_routers) % NUM_TERMINALS;
		int next_router_begin = total_routers + (((s->router_id + 1 ) % total_routers)* NUM_TERMINALS);
		dst_lp = next_router_begin + (NUM_TERMINALS - local_offset);
		if(dst_lp < next_router_begin || dst_lp > next_router_begin + NUM_TERMINALS)
                  printf("\n Incorrect destination for %d to %d ", (int)lp->gid - total_routers, (int)dst_lp - total_routers);
	}
	break;

    DEFAULT:
	      printf("\n Undefined traffic pattern!");
    }
/*  Generate packets  of PACKET_SIZE
  one MPI message can be much larger than the packet size 
  in that case break the MPI message into packets */
  for(i = 0; i < num_packets; i++)
  { 
    msg->saved_available_time = s->available_time;

    s->available_time = ROSS_MAX(tw_now(lp), s->available_time);

    ts = 0.1 + tw_rand_exponential(lp->rng, MEAN_INTERVAL/100);

    s->available_time+=ts;
//    invoke a message at the respective terminal
    e = tw_event_new( lp->gid - total_mpi_procs, s->available_time - tw_now(lp), lp );

//issue a packet_ generate event for each network packet
    m = tw_event_data( e );
    m->dest_terminal_id = dst_lp;
    m->type = T_GENERATE;
    s->message_counter++;

    m->packet_ID = packet_offset * ( lp->gid - total_routers - total_terminals ) + s->message_counter;
    m->travel_start_time = tw_now(lp);

#if DEBUG
if(m->packet_ID == TRACK && msg->chunk_id == num_chunks-1)
   printf("\n MPI message generated ID %lld destination %d ", m->packet_ID, m->dest_terminal_id);
#endif
    tw_event_send( e );
  }
  ts = MEAN_INTERVAL + tw_rand_exponential( lp->rng, MEAN_INTERVAL/100 );
  e = tw_event_new( lp->gid, ts, lp );
  m = tw_event_data( e );
  m->type = MPI_SEND;
  tw_event_send( e );
}

void 
mpi_msg_recv( process_state * s, 
	     tw_bf * bf, 
	     terminal_message * msg, 
	     tw_lp * lp )
{

  int index = floor(N_COLLECT_POINTS*(tw_now(lp)/g_tw_ts_end));
}

void 
mpi_event( process_state * s, 
	      tw_bf * bf, 
	      terminal_message * msg, 
              tw_lp * lp )
{
  *(int *)bf = (int)0;
  switch(msg->type)
   {
    case MPI_SEND:
		mpi_msg_send(s, bf, msg, lp);
    break;

    case MPI_RECV:
		mpi_msg_recv(s, bf, msg, lp);
    break;

    DEFAULT:
    		printf("\n Wrong argument for MPI process LP");
   }
}
/////////////////////////////////// Packet generate, receive functions ////////////////////////////////////////////
/////////////////////////////////////// Credit buffer ////////////////////////////////////////////////////////////////////////////////////////
//Whenever a packet is sent and a buffer slot becomes available, a credit is sent and a waiting packet is scheduled 
void router_credit_send(router_state * s, tw_bf * bf, terminal_message * msg, tw_lp * lp)
{
  tw_event * buf_e;
  tw_stime ts;
  terminal_message * buf_msg;

  int dest, credit_delay;

 // Notify sender terminal about available buffer space
  // delay when there is no network traffic

  if(msg->last_hop == TERMINAL)
  {
   dest = msg->src_terminal_id;
   //determine the time in ns to transfer the credit
   credit_delay = (1 / NODE_BANDWIDTH) * CREDIT_SIZE;
  }
   else if(msg->last_hop == GLOBAL)
   {
     dest = msg->intm_lp_id;
     credit_delay = (1 / GLOBAL_BANDWIDTH) * CREDIT_SIZE;
   }
    else if(msg->last_hop == LOCAL)
     {
        dest = msg->intm_lp_id;
     	credit_delay = (1/LOCAL_BANDWIDTH) * CREDIT_SIZE;
     }
    else
      printf("\n Invalid message type");

   // Assume it takes 0.1 ns of serialization latency for processing the credits in the queue
//     s->next_credit_available_time[output_port] += 0.1;

     int output_port = msg->saved_vc / NUM_VC;
     msg->saved_available_time = s->next_credit_available_time[output_port];
     s->next_credit_available_time[output_port] = ROSS_MAX(tw_now(lp), s->next_credit_available_time[output_port]);
     ts = credit_delay + tw_rand_exponential(lp->rng, (double)credit_delay/1000);
	
    s->next_credit_available_time[output_port]+=ts;
    buf_e = tw_event_new(dest, s->next_credit_available_time[output_port] - tw_now(lp) , lp);
    buf_msg = tw_event_data(buf_e);
    buf_msg->vc_index = msg->saved_vc;
    buf_msg->type=BUFFER;
    buf_msg->last_hop = msg->last_hop;
    buf_msg->packet_ID=msg->packet_ID;

    tw_event_send(buf_e);
}
/* packet is generated when MPI message is received from the MPI process LP */
void packet_generate(terminal_state * s, tw_bf * bf, terminal_message * msg, tw_lp * lp)
{
  tw_lpid dst_lp;
  tw_stime ts;
  tw_event *e;
  terminal_message *m;
  int i;

  for(i = 0; i < num_chunks; i++)
  {
	  // Before generating a packet, check if the input queue is available
        ts = 0.1 + tw_rand_exponential(lp->rng, MEAN_INTERVAL/200);
	int chan=-1, j;
	for(j=0; j<NUM_VC; j++)
	 {
	   //if(s->vc_occupancy[j] >= TERMINAL_THRESHOLD)
	   //  fprintf( dragonfly_event_log, " \n Dragonfly compute node buffer size %d terminal ID %d time stamp %lf ", s->vc_occupancy[j], lp->gid - total_routers, tw_now(lp));
	     if(s->vc_occupancy[j] < TERMINAL_VC_SIZE * num_chunks)
	      {
	       chan=j;
	       break;
	      }
         }

       e = tw_event_new(lp->gid, i + ts, lp);
       m = tw_event_data(e);
       m->travel_start_time = msg->travel_start_time;
       m->my_N_hop = 0;
       m->dest_terminal_id=msg->dest_terminal_id;
       m->packet_ID = msg->packet_ID;
       m->intm_group_id = -1;
       m->saved_vc=0;
       m->chunk_id = i;
       m->output_chan = -1;
       m->wait_loc = -1;
       msg->saved_vc = 0;

       if(chan != -1) // If the input queue is available
   	{
	    // Send the packet out
	     m->type = T_SEND;
	     m->wait_type = -1;
	     // record start time
 	     tw_event_send(e);
        }
      else
         {
	    //schedule a generate event after a certain delay
	    //
	   //m->type = WAIT;
          // m->wait_type = T_SEND;
	  // tw_event_send(e);
	  printf("\n Exceeded queue size, exitting ");
	  exit(-1);
        } //else
  } // for
}

/* Packet is sent once the buffer space is available on the channels */
void packet_send(terminal_state * s, tw_bf * bf, terminal_message * msg, tw_lp * lp)
{
  tw_stime ts;
  tw_event *e;
  terminal_message *m;
  /* Route the packet to its source router */ 

   int vc=msg->saved_vc, i=0;
   bf->c3 = 0;

   if(msg->chunk_id == num_chunks - 1) 
    {
       bf->c3 = 1;
       int index = floor(N_COLLECT_POINTS*(tw_now(lp)/g_tw_ts_end));
       s->packet_counter++;
       N_generated_storage[index]++;
       //if(s->packet_counter >= max_packets)
	//fprintf(dragonfly_event_log, " \n terminal %d done injecting packets storage %d time stamp %lf ", lp->gid - total_routers, s->packet_counter, tw_now(lp));
    }

//  Each packet is broken into chunks and then sent over the channel
   msg->saved_available_time = s->terminal_available_time;
   
   head_delay = (1/NODE_BANDWIDTH) * CHUNK_SIZE;
   ts = head_delay + tw_rand_exponential(lp->rng, (double)head_delay/200);
   
   s->terminal_available_time = ROSS_MAX(s->terminal_available_time, tw_now(lp));
   s->terminal_available_time += ts;
   //fprintf(dragonfly_event_log, " router id %d ", s->router_id);
   e = tw_event_new(s->router_id, s->terminal_available_time - tw_now(lp), lp);

#if DEBUG
if( msg->packet_ID == TRACK && msg->chunk_id == num_chunks-1)
  {
    printf("\n (%lf) [Terminal %d] {Node %d} Packet %lld chunk %d being sent to source router %d Output VC : %d after %lf dest terminal id: %d \n", tw_now(lp), (int)lp->gid - total_routers, (int)g_tw_mynode, msg->packet_ID, msg->chunk_id, (int)s->router_id, (int)msg->saved_vc, s->terminal_available_time - tw_now(lp), msg->dest_terminal_id);
  }
#endif
   m = tw_event_data(e);
   m->type = R_ARRIVE;

	  // Carry on the message info
   m->dest_terminal_id = msg->dest_terminal_id;
   m->src_terminal_id = lp->gid;
   m->packet_ID = msg->packet_ID;
   m->chunk_id = msg->chunk_id;
   m->travel_start_time = msg->travel_start_time;
   m->my_N_hop = msg->my_N_hop;
   m->saved_vc = vc;
   m->last_hop = TERMINAL;
   m->wait_type = msg->wait_type;
   m->intm_group_id = -1;
//  Each chunk is 32B and the VC occupancy is in chunks to enable efficient flow control

   tw_event_send(e);
   
   s->vc_occupancy[vc]++;

   if(s->vc_occupancy[vc] >= (TERMINAL_VC_SIZE * num_chunks))
   {
      s->output_vc_state[vc] = VC_CREDIT;
   }
}

/* flits arrive on the router, when last flit of the packet arrives, packet completion is marked */
void packet_arrive(terminal_state * s, tw_bf * bf, terminal_message * msg, tw_lp * lp)
{
#if DEBUG
if( msg->packet_ID == TRACK && msg->chunk_id == num_chunks-1)
    {
	printf( "(%lf) [Terminal %d] packet %lld has arrived  \n",
              tw_now(lp), (int)lp->gid, msg->packet_ID);

	printf("travel start time is %f\n",
                msg->travel_start_time);

	printf("My hop now is %d\n",msg->my_N_hop);
    }
#endif

  // Packet arrives and accumulate # queued
  // Find a queue with an empty buffer slot

  tw_event * e, * buf_e;
  terminal_message * m, * buf_msg;
  tw_stime ts;
  bf->c1=0;
//  printf("\n Message chunk arrived, ID: %d ", msg->chunk_id);

  if(msg->chunk_id == num_chunks-1)
  {
    bf->c1=1;
    ts = 0.1 + tw_rand_exponential(lp->rng, MEAN_PROCESS/200);
    e = tw_event_new(lp->gid, ts, lp);
    m = tw_event_data( e );
    m->travel_start_time = msg->travel_start_time;
    m->type = FINISH;
    m->packet_ID = msg->packet_ID;
    m->my_N_hop = msg->my_N_hop;
    tw_event_send(e);
  }

  int credit_delay = (1/NODE_BANDWIDTH) * CREDIT_SIZE;
  ts = credit_delay + tw_rand_exponential(lp->rng, credit_delay/1000);
  
  msg->saved_available_time = s->next_credit_available_time;
  s->next_credit_available_time = ROSS_MAX(s->next_credit_available_time, tw_now(lp));
  s->next_credit_available_time += ts;

  buf_e = tw_event_new(s->router_id, s->next_credit_available_time - tw_now(lp), lp);
  buf_msg = tw_event_data(buf_e);
  buf_msg->vc_index = msg->saved_vc;
  buf_msg->type=BUFFER;
  buf_msg->packet_ID=msg->packet_ID;
  buf_msg->last_hop = TERMINAL;

  tw_event_send(buf_e);
}

/* Packet has arrived at the final destination. Update simulation statistics */
void packet_finish(terminal_state * s, tw_bf * bf, terminal_message * msg, tw_lp * lp)
{
  N_finished++;
  bf->c3 = 0;

  int index = floor(N_COLLECT_POINTS*(tw_now(lp)/g_tw_ts_end));
  N_finished_storage[index]++;
  total_time += (tw_now(lp) - msg->travel_start_time);
  total_hops += msg->my_N_hop;

  if (max_latency < (tw_now(lp) - msg->travel_start_time) )
   {
	bf->c3 = 1;	
	msg->saved_available_time = max_latency;
	max_latency=tw_now(lp) - msg->travel_start_time;
//	max_packet = msg->packet_ID;
   }
}

/////////////////// WAiting packets linked list /////////////////////////////
void
waiting_terminal_packet_free(terminal_state * s, int loc)
{
  int i, max_count = s->wait_count - 1;
  
  for(i = loc; i < max_count; i++) {
      s->waiting_list[i].chan = s->waiting_list[i + 1].chan;
      s->waiting_list[i].packet = s->waiting_list[i + 1].packet;
  }

  s->waiting_list[max_count].packet = NULL;
  // 16-05
  s->waiting_list[max_count].chan = -1;

  s->wait_count--;
}

void
update_terminal_waiting_list( terminal_state * s,
			tw_bf * bf,
                        terminal_message * msg,
                        tw_lp * lp )
{
  bf->c3 = 0;
  int loc = s->wait_count;

  if(loc >= TERMINAL_WAITING_PACK_COUNT)
    {
	bf->c3 = 1;
 	printf(" Terminal reached maximum count of linked list %d ", s->wait_count);
	// Do not insert packet in the waiting queue in the unusual case where the waiting queue count exceeds
	return;
    }

  s->waiting_list[loc].chan = msg->saved_vc;
  s->waiting_list[loc].packet = msg;
  s->wait_count++;
}

/*Once a credit arrives at the node, this method picks a waiting packet in the injection queue and schedules it */
void
schedule_terminal_waiting_msg( terminal_state * s,
                           tw_bf * bf,
                           terminal_message * msg,
                           tw_lp * lp )
{
  bf->c3 = 0;
  if( s->wait_count > 0)
   {
    bf->c3 = 1;
    tw_event * e_h;
    terminal_message * m;
    tw_stime ts;
    ts = 0.1 + tw_rand_exponential(lp->rng, MEAN_INTERVAL/1000);
    e_h = tw_event_new( lp->gid, ts, lp );
    m = tw_event_data(e_h);
    memcpy(m, s->waiting_list[0].packet, sizeof(terminal_message));
  
    memcpy(msg, s->waiting_list[0].packet, sizeof(terminal_message));
    msg->wait_loc = 0;
    msg->type = BUFFER;
  
    m->type = T_SEND;
    tw_event_send( e_h );
    waiting_terminal_packet_free(s, 0);
  }
}

////////////////////////////////////////////////// Terminal related functions ///////////////////////////////////////

void 
terminal_init( terminal_state * s, 
	       tw_lp * lp )
{
    int i;

    s->terminal_id=((int)lp->gid);  
 
    // Assign the global router ID
    s->router_id=getRouterID(lp->gid);
    s->terminal_available_time = 0.0;

    //printf("\n (%lf) [Terminal %d] assigned router id %d ", tw_now(lp), (int)lp->gid, (int)s->router_id);
    s->packet_counter = 0;

   for( i = 0; i < NUM_VC; i++ )
    {
      s->vc_occupancy[i]=0;
      s->output_vc_state[i]=VC_IDLE;
    }
   s->waiting_list = tw_calloc(TW_LOC, "waiting list", sizeof(struct waiting_packet), TERMINAL_WAITING_PACK_COUNT);

   int j;
   for (j = 0; j < TERMINAL_WAITING_PACK_COUNT - 1; j++) {
      s->waiting_list[j].next = &s->waiting_list[j + 1];
      // 16-05
      s->waiting_list[j].chan = -1;
      s->waiting_list[j].packet = NULL;
    }

   s->waiting_list[j].next = NULL;
   s->waiting_list[j].chan = -1;
   s->waiting_list[j].packet = NULL;

   s->head = &s->waiting_list[0];
   // 16/05
   s->wait_count = 0;
}

/* buffer status is updated when credit arrives */
void 
terminal_buf_update(terminal_state * s, 
		    tw_bf * bf, 
		    terminal_message * msg, 
		    tw_lp * lp)
{
  // Update the buffer space associated with this router LP 
    int msg_indx = msg->vc_index;
    
    s->vc_occupancy[msg_indx]--;
    s->output_vc_state[msg_indx] = VC_IDLE;
    schedule_terminal_waiting_msg( s, bf, msg, lp );
}

void 
terminal_event( terminal_state * s, 
		tw_bf * bf, 
		terminal_message * msg, 
		tw_lp * lp )
{
  *(int *)bf = (int)0;
  switch(msg->type)
    {
    case T_GENERATE:
       packet_generate(s,bf,msg,lp);
    break;
    
    case T_ARRIVE:
        packet_arrive(s,bf,msg,lp);
    break;
    
    case T_SEND:
      packet_send(s,bf,msg,lp);
    break;
    
    case BUFFER:
       terminal_buf_update(s, bf, msg, lp);
     break;

    case WAIT:
      update_terminal_waiting_list( s, bf, msg, lp );
    break;

    case FINISH:
      packet_finish(s, bf, msg, lp);
    break;

    default:
       printf("\n LP %d Terminal message type not supported %d ", (int)lp->gid, msg->type);
    }
}

void 
final( terminal_state * s, 
      tw_lp * lp )
{

}

/////////////////////////////////////////// Router packet send/receive functions //////////////////////

void 
router_reschedule_event(router_state * s, 
			tw_bf * bf, 
			terminal_message * msg, 
			tw_lp * lp)
{
 // Check again after some time
  terminal_message * m;
  tw_event * e;
  tw_stime ts = 0.1 + tw_rand_exponential(lp->rng, (double)RESCHEDULE_DELAY/10);

  e = tw_event_new(lp->gid, ts, lp);

  m = tw_event_data(e);
  m->travel_start_time = msg->travel_start_time;
  m->dest_terminal_id = msg->dest_terminal_id;
  m->packet_ID = msg->packet_ID;
  m->type = msg->type;
  m->my_N_hop = msg->my_N_hop;
  m->intm_lp_id = msg->intm_lp_id;
  m->saved_vc = msg->saved_vc;
  m->src_terminal_id = msg->src_terminal_id;
  m->last_hop = msg->last_hop;
  m->output_chan = msg->output_chan;
  m->intm_group_id = msg->intm_group_id;
  m->type = WAIT;
  m->wait_type = msg->wait_type;

  tw_event_send(e);
}
/* get next stop (terminal or router) according to the routing algorithm */
tw_lpid 
get_next_stop(router_state * s, 
		      tw_bf * bf, 
		      terminal_message * msg, 
		      tw_lp * lp, 
		      int path)
{
   int dest_lp;
   int i;
   int dest_router_id = getRouterID(msg->dest_terminal_id);
   int dest_group_id;
 
   bf->c2 = 0;

   if(dest_router_id == lp->gid)
    {
        dest_lp = msg->dest_terminal_id;

        return dest_lp;
    }
   // Generate inter-mediate destination
   if(msg->last_hop == TERMINAL && path == NON_MINIMAL)
    {
      if(dest_router_id/NUM_ROUTER != s->group_id)
         {
            bf->c2 = 1;
            int intm_grp_id = tw_rand_integer(lp->rng, 0, num_groups-1);
            msg->intm_group_id = intm_grp_id;
          }    
    }
   if(msg->intm_group_id == s->group_id)
   {  
           msg->intm_group_id = -1;//no inter-mediate group
   } 
  if(msg->intm_group_id >= 0)
   {
      dest_group_id = msg->intm_group_id;
   }
  else
   {
     dest_group_id = dest_router_id / NUM_ROUTER;
   }
  
  if(s->group_id == dest_group_id)
   {
     dest_lp = dest_router_id;
   }
   else
   {
      dest_lp=getRouterFromGroupID(dest_group_id,s);
      if(dest_lp == lp->gid)
      {
        for(i=0; i<GLOBAL_CHANNELS; i++)
           {
            if(s->global_channel[i]/NUM_ROUTER == dest_group_id)
                dest_lp=s->global_channel[i];
          }
      }
   }
  return dest_lp;
}
/* get output port of the router according to the next packet stop */
int 
get_output_port( router_state * s, 
		tw_bf * bf, 
		terminal_message * msg, 
		tw_lp * lp, 
		int next_stop )
{
  int output_port = -1, i;

  if(next_stop == msg->dest_terminal_id)
   {
      output_port = NUM_ROUTER + GLOBAL_CHANNELS +(getTerminalID(msg->dest_terminal_id)%NUM_TERMINALS);
    }
    else
    {
     int intm_grp_id = next_stop / NUM_ROUTER;

     if(intm_grp_id != s->group_id)
      {
        for(i=0; i<GLOBAL_CHANNELS; i++)
         {
           if(s->global_channel[i] == next_stop)
             output_port = NUM_ROUTER + i;
          }
      }
      else
       {
        output_port = next_stop % NUM_ROUTER;
       }
    }
    return output_port;
}
/* send the packet from the router to the next stop (can be a router or a terminal) */
void 
router_packet_send( router_state * s, 
		    tw_bf * bf, 
		     terminal_message * msg, tw_lp * lp)
{
   tw_stime ts;
   tw_event *e;
   terminal_message *m;

   int next_stop = -1, output_port = -1, output_chan = -1;
   float bandwidth = LOCAL_BANDWIDTH;
   int path = ROUTING;
   int minimal_out_port = -1, nonmin_out_port = -1;

   bf->c3 = 0;
   bf->c2 = 0;

   next_stop = get_next_stop(s, bf, msg, lp, path);
   output_port = get_output_port(s, bf, msg, lp, next_stop); 
   output_chan = output_port * NUM_VC;

   // Even numbered channels for minimal routing
   // Odd numbered channels for nonminimal routing
   int i, global=0;
   int buf_size = LOCAL_VC_SIZE;

   // Allocate output Virtual Channel
  if(output_port >= NUM_ROUTER && output_port < NUM_ROUTER + GLOBAL_CHANNELS)
  {
	 //delay = GLOBAL_DELAY;
	 //printf("\n Output port selected is %d ", output_port);
	 bandwidth = GLOBAL_BANDWIDTH;
	 global = 1;
	 buf_size = GLOBAL_VC_SIZE;
  }

  if(output_port >= NUM_ROUTER+GLOBAL_CHANNELS)
	buf_size = TERMINAL_VC_SIZE;

/*   if(s->vc_occupancy[output_chan] >= ROUTER_THRESHOLD)
            fprintf( dragonfly_event_log, " \n Dragonfly router buffer size %d router ID %d time stamp %lf ", s->vc_occupancy[output_chan], (int)lp->gid, tw_now(lp));
*/
   if(s->vc_occupancy[output_chan] >= buf_size)
    {
       if(msg->last_hop == TERMINAL)
	 {
	    printf("\n %lf Router %d buffers overflowed from incoming terminals channel %d occupancy %d ", tw_now(lp),(int)lp->gid, output_chan, s->vc_occupancy[output_chan]);
	    bf->c2 = 0;
	 }
      else
       {
          bf->c3 = 1;

          msg->wait_type = R_SEND;
          msg->vc_index = output_chan;
          router_reschedule_event(s, bf, msg, lp);
       }
       return;
    }

#if DEBUG
if( msg->packet_ID == TRACK && next_stop != msg->dest_terminal_id && msg->chunk_id == num_chunks-1)
  {
   printf("\n (%lf) [Router %d] Packet %lld being sent to intermediate group router %d Final destination router %d Output Channel Index %d Saved vc %d msg_intm_id %d \n", 
              tw_now(lp), (int)lp->gid, msg->packet_ID, next_stop, 
	      getRouterID(msg->dest_terminal_id), output_chan, msg->saved_vc, msg->intm_group_id);
  }
#endif
 // If source router doesn't have global channel and buffer space is available, then assign to appropriate intra-group virtual channel 
  msg->saved_available_time = s->next_output_available_time[output_port];
  ts = ((1/bandwidth) * CHUNK_SIZE) + tw_rand_exponential(lp->rng, (double)CHUNK_SIZE/200);

  s->next_output_available_time[output_port] = ROSS_MAX(s->next_output_available_time[output_port], tw_now(lp));
  s->next_output_available_time[output_port] += ts;
  e = tw_event_new(next_stop, s->next_output_available_time[output_port] - tw_now(lp), lp);

  m = tw_event_data(e);

  if(global)
    m->last_hop=GLOBAL;
  else
    m->last_hop = LOCAL;

  m->saved_vc = output_chan;
  msg->old_vc = output_chan;
  m->intm_lp_id = lp->gid;

  s->vc_occupancy[output_chan]++;

  // Carry on the message information
  m->dest_terminal_id = msg->dest_terminal_id;
  m->packet_ID = msg->packet_ID;
  m->travel_start_time = msg->travel_start_time;
  m->src_terminal_id = msg->src_terminal_id;
  m->my_N_hop = msg->my_N_hop;
  m->intm_group_id = msg->intm_group_id;
  m->wait_type = -1;
  m->chunk_id = msg->chunk_id;


  if(next_stop == msg->dest_terminal_id)
  {
    m->type = T_ARRIVE;

    if(s->vc_occupancy[output_chan] >= TERMINAL_VC_SIZE * num_chunks)
      s->output_vc_state[output_chan] = VC_CREDIT;
  }
  else
  {
    m->type = R_ARRIVE;

   if( global )
   {
     if(s->vc_occupancy[output_chan] >= GLOBAL_VC_SIZE * num_chunks )
       s->output_vc_state[output_chan] = VC_CREDIT;
   }
  else
    {
     if( s->vc_occupancy[output_chan] >= LOCAL_VC_SIZE * num_chunks )
	s->output_vc_state[output_chan] = VC_CREDIT;
    }
  }
  tw_event_send(e);
}

// Packet arrives at the router (can be an intermediate or final router)
void 
router_packet_receive( router_state * s, 
			tw_bf * bf, 
			terminal_message * msg, 
			tw_lp * lp )
{
    tw_event *e, * buf_e;
    terminal_message *m;
    tw_stime ts;

    msg->my_N_hop++;

    ts = 0.1 + tw_rand_exponential(lp->rng, (double)MEAN_INTERVAL/200);

    e = tw_event_new(lp->gid, ts, lp);
 
    m = tw_event_data(e);
    m->saved_vc = msg->saved_vc;
    m->intm_lp_id = msg->intm_lp_id;

   // Carry on the message information
    m->dest_terminal_id = msg->dest_terminal_id;
    m->src_terminal_id = msg->src_terminal_id;
    m->packet_ID = msg->packet_ID;
    m->chunk_id = msg->chunk_id;
    m->travel_start_time = msg->travel_start_time;
    m->my_N_hop = msg->my_N_hop;
    m->last_hop = msg->last_hop;
    m->intm_group_id = msg->intm_group_id;
    m->type = R_SEND;
    m->wait_type = -1;

#if DEBUG
if(msg->packet_ID == TRACK && msg->chunk_id == num_chunks-1)
	printf("\n Router %d packet received %lld source terminal id %d dest terminal ID %d ", (int)lp->gid, m->packet_ID, m->src_terminal_id/4, m->dest_terminal_id/4);
#endif
    router_credit_send(s, bf, msg, lp);
    tw_event_send(e);  
}
/////////////////////////////////////////// Router related functions /////////////////////////////////
void router_setup(router_state * r, tw_lp * lp)
{
   r->router_id=((int)lp->gid);
   r->group_id=lp->gid/NUM_ROUTER;

   int i, j;
   int offset=(lp->gid%NUM_ROUTER) * (GLOBAL_CHANNELS/2) +1;
  
//   r->next_credit_available_time = 0;
   for(i=0; i < RADIX; i++)
    {
	//r->next_input_available_time[i]=0;
	r->next_output_available_time[i]=0;
        r->next_credit_available_time[i]=0;

       // Set credit & router occupancy
       r->vc_occupancy[i]=0;

       // Set virtual channel state to idle
       //r->input_vc_state[i] = VC_IDLE;
       r->output_vc_state[i]= VC_IDLE;
    }

   //round the number of global channels to the nearest even number
   for(i=0; i<GLOBAL_CHANNELS; i++)
    {
      if(i%2!=0)
          {
             r->global_channel[i]=(lp->gid + (offset*NUM_ROUTER))%total_routers;
             offset++;
          }
          else
           {
             r->global_channel[i]=lp->gid-((offset)*NUM_ROUTER);
           }
        if(r->global_channel[i]<0)
         {
           r->global_channel[i]=total_routers+r->global_channel[i]; 
	 }
   
  #if PRINT_ROUTER_TABLE
	//fprintf(dragonfly_event_log, "\n Router %d setup ", lp->gid);


	//fprintf(dragonfly_event_log, "\n Router %d connected to Router %d Group %d to Group %d ", local_router_id, r->global_channel[i], r->group_id, (r->global_channel[i]/NUM_ROUTER));
   #endif
    }
  r->waiting_list = tw_calloc(TW_LOC, "waiting list", sizeof(struct waiting_packet), ROUTER_WAITING_PACK_COUNT);

  for (j = 0; j < ROUTER_WAITING_PACK_COUNT - 1; j++) {
     r->waiting_list[j].next = &r->waiting_list[j + 1];
     // 16-05
     r->waiting_list[j].packet = NULL;
     r->waiting_list[j].chan = -1;
  }
  
  r->waiting_list[j].next = NULL;
  r->waiting_list[j].chan = -1;
  r->head = &r->waiting_list[0];
  r->wait_count = 0;
}	
/*If a packet arrives at a router and the channel is not available, the router maintains a list of waiting packets and the respective channels
for which the packets are waiting */
void
update_router_waiting_list( router_state * s,
			    tw_bf * bf,
                        terminal_message * msg,
                        tw_lp * lp )
{
   bf->c3 = 0;
   int loc = s->wait_count, chan;
   
   if(loc >= ROUTER_WAITING_PACK_COUNT)
    {
//       In the unusual case where all waiting packets are full, the packets will be dropped
//       printf(" Reached maximum count of linked list %d ", s->wait_count);
       bf->c3 = 1;
       return;
    }

//   if(msg->wait_type == R_ARRIVE)
//	chan = msg->input_chan;
//     else
//         if(msg->wait_type == R_SEND)
	   chan = msg->vc_index;
//          else
//  	      printf("\n ---- Invalid wait type in the queue %d ", msg->wait_type);

   s->waiting_list[loc].chan = chan;
   s->waiting_list[loc].packet = msg;
   
   s->wait_count++;
}

void 
waiting_router_packet_free(router_state * s, int loc)
{
  int i, max_count = s->wait_count -1;

  for(i = loc; i < max_count; i++) {
       s->waiting_list[i].chan = s->waiting_list[i + 1].chan;
       s->waiting_list[i].packet = s->waiting_list[i + 1].packet;
  }
  s->waiting_list[max_count].packet = NULL;
  s->waiting_list[max_count].chan = -1;

  s->wait_count--;
}


/*Whenever an input or output channel is set to idle, the reschedule event is triggered to send the next packet */
void
schedule_router_waiting_msg( router_state * s,
                           tw_bf * bf,
			   terminal_message * msg,
                           tw_lp * lp,
			   int chan)
{
  bf->c3 = 0;
  if( s->wait_count <= 0 )
   {
    return;
   }

  int loc=s->wait_count;
  tw_event * e_h;
  terminal_message * m;
  tw_stime ts;
  int j;

 for(j = 0; j < loc; j++)
  {
    if( s->waiting_list[j].chan == chan )
     {
       // 16-05
       bf->c3 = 1;
       ts = tw_rand_exponential(lp->rng, MEAN_INTERVAL/1000);	
       e_h = tw_event_new( lp->gid, ts, lp );
       m = tw_event_data( e_h );
       memcpy(m, s->waiting_list[j].packet, sizeof(terminal_message));

       //        For reverse computation, also copy data to the msg
       memcpy(msg, s->waiting_list[j].packet, sizeof(terminal_message));                 
       msg->wait_loc = loc;

//       changed from R_ARRIVE to R_SEND 16-05
       m->type = R_SEND;
       msg->type = BUFFER;
       tw_event_send(e_h);       
       waiting_router_packet_free(s, loc);
       break;
     }
  }
}
/* When a credit arrives, the router channel is updated */
void router_buf_update(router_state * s, tw_bf * bf, terminal_message * msg, tw_lp * lp)
{
   // Update the buffer space associated with this router LP 
    int msg_indx = msg->vc_index;

    s->vc_occupancy[msg_indx]--;
    s->output_vc_state[msg_indx] = VC_IDLE;
    schedule_router_waiting_msg(s, bf, msg, lp, msg_indx);
}

void router_event(router_state * s, tw_bf * bf, terminal_message * msg, tw_lp * lp)
{
  *(int *)bf = (int)0;
  switch(msg->type)
   {
	   case R_SEND: // Router has sent a packet to an intra-group router (local channel)
 		 router_packet_send(s, bf, msg, lp);
           break;

	   case R_ARRIVE: // Router has received a packet from an intra-group router (local channel)
	        router_packet_receive(s, bf, msg, lp);
	   break;
	
	   case BUFFER:
	        router_buf_update(s, bf, msg, lp);
	   break;

	   case WAIT:
               update_router_waiting_list( s, bf, msg, lp );
          break;

	   default:
		  printf("\n (%lf) [Router %d] Router Message type not supported %d dest terminal id %d packet ID %d ", tw_now(lp), (int)lp->gid, msg->type, (int)msg->dest_terminal_id, (int)msg->packet_ID);
	   break;
   }	   
}

/////////////////////////////////////////// REVERSE EVENT HANDLERS ///////////////////////////////////
//
//Reverse computation handler for a terminal event
void terminal_rc_event_handler(terminal_state * s, tw_bf * bf, terminal_message * msg, tw_lp * lp)
{
   int index = floor(N_COLLECT_POINTS*(tw_now(lp)/g_tw_ts_end));

   switch(msg->type)
   {
	   case T_GENERATE:
		 {
		 int i;
		 for(i = 0; i < num_chunks; i++)
                  tw_rand_reverse_unif(lp->rng);
		 }
	   break;
	   
	   case T_SEND:
	         {
		   if(bf->c3) 
		    {
			int index = floor(N_COLLECT_POINTS*(tw_now(lp)/g_tw_ts_end));
			N_generated_storage[index]--;	
		        s->packet_counter--;
		    }
	           s->terminal_available_time = msg->saved_available_time;
		   tw_rand_reverse_unif(lp->rng);	
		   int vc = msg->saved_vc;
		   s->vc_occupancy[vc]--;
		   s->output_vc_state[vc] = VC_IDLE;
		 }
	   break;

	   case T_ARRIVE:
	   	 {
		   if(bf->c1)
		     tw_rand_reverse_unif(lp->rng);
		   tw_rand_reverse_unif(lp->rng);
		   s->next_credit_available_time = msg->saved_available_time;
		 }
           break;

	   case WAIT:
		{
		   if(bf->c3)
			return;
		     s->wait_count-=1;
		     int loc = s->wait_count; 
		     s->waiting_list[loc].chan = -1;
		     s->waiting_list[loc].packet = NULL;
		}
	   break;

	   case BUFFER:
	        {
		   int msg_indx = msg->vc_index;
		   s->vc_occupancy[msg_indx]++;
		   
		  if(msg_indx != 0)
		     printf("\n Invalid terminal VC index %d ", msg_indx);
		   if(s->vc_occupancy[msg_indx] == TERMINAL_VC_SIZE * num_chunks)
         	     {
			s->output_vc_state[msg_indx] = VC_CREDIT;
	             }
		   
		   if(bf->c3)
		    {
    		           tw_rand_reverse_unif(lp->rng);
			   int loc = msg->wait_loc, i;
			   if(msg->wait_loc < 0)
			      printf("\n Invalid message wait loc %d ", msg->wait_loc);
			   int max_count = s->wait_count;
	                   for(i = max_count; i > loc ; i--)
        	            {
               		         s->waiting_list[i].chan = s->waiting_list[i-1].chan;
                        	 s->waiting_list[i].packet = s->waiting_list[i-1].packet;
	                    }
        	          s->waiting_list[loc].chan = msg->saved_vc;
                	  s->waiting_list[loc].packet = msg;
                          s->wait_count++;	
		  }
	     }  
	   break;
	
	  case FINISH:
		{
		    N_finished--;
		    int index = floor(N_COLLECT_POINTS*(tw_now(lp)/g_tw_ts_end));
		    N_finished_storage[index]--;
		    total_time -= (tw_now(lp) - msg->travel_start_time);
		    total_hops -= msg->my_N_hop;
		    if(bf->c3)
		         max_latency = msg->saved_available_time;
                    
		}
   }
}
//Reverse computation handler for a router event
void router_rc_event_handler(router_state * s, tw_bf * bf, terminal_message * msg, tw_lp * lp)
{
  switch(msg->type)
    {
            case R_SEND:
		    {
			if(bf->c2)
			   return;

		        tw_rand_reverse_unif(lp->rng);

			if(bf->c3)
			   return;
			    
			int output_chan = msg->old_vc;
			int output_port = output_chan/NUM_VC;

			s->next_output_available_time[output_port] = msg->saved_available_time;
			s->vc_occupancy[output_chan]--;
			s->output_vc_state[output_chan]=VC_IDLE;
			if(bf->c2)
			   tw_rand_reverse_unif(lp->rng);
		    }
	    break;

	    case R_ARRIVE:
	    	    {
			msg->my_N_hop--;
			tw_rand_reverse_unif(lp->rng);
			tw_rand_reverse_unif(lp->rng);
			int output_port = msg->saved_vc/NUM_VC;
			s->next_credit_available_time[output_port] = msg->saved_available_time;
		    }
	    break;

	    case BUFFER:
	    	   {
		      int msg_indx = msg->vc_index;
                      s->vc_occupancy[msg_indx]++;

                      int buf = LOCAL_VC_SIZE;

		      if(msg->last_hop == GLOBAL)
			 buf = GLOBAL_VC_SIZE;
		       else if(msg->last_hop == TERMINAL)
			 buf = TERMINAL_VC_SIZE;
	 
		      if(s->vc_occupancy[msg_indx] >= buf * num_chunks)
                          s->output_vc_state[msg_indx] = VC_CREDIT;

                      if(bf->c3)
                       {
                         int loc = msg->wait_loc, i;
			 tw_rand_reverse_unif(lp->rng);
			 if(msg->wait_loc < 0)
				printf("\n Invalid message wait loc %d ", msg->wait_loc);
                         int max_count = s->wait_count;
                          for(i = max_count; i > loc ; i--)
                          {
                            s->waiting_list[i].chan = s->waiting_list[i-1].chan;
                            s->waiting_list[i].packet = s->waiting_list[i-1].packet;
                          }
//			  changed from saved_vc to vc_index 16-05
                          s->waiting_list[loc].chan = msg->vc_index;
                          s->waiting_list[loc].packet = msg;
                          s->wait_count=s->wait_count+1;
                       }
		   }
	    break;
	  
	   case WAIT:
		  {
			if(bf->c3)
			   return;
			s->wait_count-=1;
			int loc = s->wait_count;
			// 16-05
			s->waiting_list[loc].chan = -1;
			s->waiting_list[loc].packet = NULL;
		  }
    }
}

void mpi_rc_event_handler(process_state * s, tw_bf * bf, terminal_message * msg, tw_lp * lp)
{
  // TO BE DONE
   switch(msg->type)
	{
	    case MPI_SEND:
	            {
		      if(bf->c1)
			return;

		      if(bf->c3)
			 tw_rand_reverse_unif(lp->rng);	
		      
		      if(bf->c2)
			 tw_rand_reverse_unif(lp->rng);

		     int i;
		     for(i = 0; i < num_packets; i++)
			{
			   tw_rand_reverse_unif(lp->rng);
			   s->available_time = msg->saved_available_time;
			   s->message_counter--;
			}
		     tw_rand_reverse_unif(lp->rng);
		    }
	    break;

 	   case MPI_RECV:
		  {

		  }
	   break;
	}
}
////////////////////////////////////////////////////// MAIN ///////////////////////////////////////////////////////
////////////////////////////////////////////////////// LP TYPES /////////////////////////////////////////////////
tw_lptype dragonfly_lps[] =
{
   // Terminal handling functions
   {
    (init_f)terminal_init,
    (pre_run_f) NULL,
    (event_f) terminal_event,
    (revent_f) terminal_rc_event_handler,
    (final_f) final,
    (map_f) mapping,
    sizeof(terminal_state)
    },
   {
     (init_f) router_setup,
     (pre_run_f) NULL,
     (event_f) router_event,
     (revent_f) router_rc_event_handler,
     (final_f) final,
     (map_f) mapping,
     sizeof(router_state),
   },
   {
     (init_f) mpi_init,
     (pre_run_f) NULL,
     (event_f) mpi_event,
     (revent_f) mpi_rc_event_handler,
     (final_f) final,
     (map_f) mapping,
     sizeof(process_state),
   },
   {0},
};

const tw_optdef app_opt [] =
{
   TWOPT_GROUP("Dragonfly Model"),
   TWOPT_UINT("memory", opt_mem, "optimistic memory"),
   TWOPT_UINT("traffic", traffic, "UNIFORM RANDOM=1, DRAGONFLY ZONES=2, TRANSPOSE=3, NEAREST NEIGHBOR=4 "),
   TWOPT_UINT("routing", ROUTING, "MINIMAL=0, NON_MINIMAL=1, ADAPTIVE=2(ADAPTIVE ROUTING QUEUE CONGESTION SENSING FEATURE's DEVELOPMENT IN PROGRESS)"),
   TWOPT_UINT("mem_factor", mem_factor, "mem_factor"),
   TWOPT_STIME("arrive_rate", MEAN_INTERVAL, "packet inter-arrival rate"),
   TWOPT_END()
};

tw_lp * dragonfly_mapping_to_lp(tw_lpid lpid)
{
  int index;

  if(lpid < total_routers)
      index = lpid - g_tw_mynode * nlp_router_per_pe - get_router_rem();
  else 
     if(lpid >= total_routers && lpid < total_routers+total_terminals)
        index = nlp_router_per_pe + (lpid - g_tw_mynode * nlp_terminal_per_pe - get_terminal_rem() - total_routers);
   else
	 index = nlp_router_per_pe + nlp_terminal_per_pe + (lpid - g_tw_mynode * nlp_mpi_procs_per_pe - get_terminal_rem() - total_routers - total_terminals);	

  return g_tw_lp[index];
}

void dragonfly_mapping(void)
{
  tw_lpid kpid;
  tw_pe * pe;

  int nkp_per_pe=16;
  for(kpid = 0; kpid < nkp_per_pe; kpid++)
    tw_kp_onpe(kpid, g_tw_pe[0]);

  int i;
  //printf("\n Node %d router start %d Terminal start %d MPI procs start %d ", g_tw_mynode, 
//									    g_tw_mynode * nlp_router_per_pe + get_router_rem(), 
//									    total_routers + g_tw_mynode * nlp_terminal_per_pe + get_terminal_rem(), 
//									    total_routers + total_terminals + g_tw_mynode * nlp_mpi_procs_per_pe);
  for(i = 0; i < nlp_router_per_pe; i++)
   {
     kpid = i % g_tw_nkp;

     pe = tw_getpe(kpid % g_tw_npe);
     
     tw_lp_onpe(i, pe, g_tw_mynode * nlp_router_per_pe + i + get_router_rem());
     tw_lp_onkp(g_tw_lp[i], g_tw_kp[kpid]);
     tw_lp_settype(i, &dragonfly_lps[1]);

#ifdef DEBUG
    //printf("\n [Node %d] Local Router ID %d Global Router ID %d ", g_tw_mynode, i, g_tw_mynode * nlp_router_per_pe + i + get_router_rem());
#endif
   } 
//apping for terminal LP
  for(i = 0; i < nlp_terminal_per_pe; i++)
   {
      kpid = i % g_tw_nkp;

      pe = tw_getpe(kpid % g_tw_npe);

      tw_lp_onpe(nlp_router_per_pe + i, pe, total_routers + g_tw_mynode * nlp_terminal_per_pe + i + get_terminal_rem());
      tw_lp_onkp(g_tw_lp[nlp_router_per_pe + i], g_tw_kp[kpid]);
      tw_lp_settype(nlp_router_per_pe + i, &dragonfly_lps[0]);

#ifdef DEBUG
//    printf("\n [Node %d] Local Terminal ID %d Global Terminal ID %d ", g_tw_mynode, nlp_router_per_pe + i, total_routers + g_tw_mynode * nlp_terminal_per_pe + i + get_terminal_rem());
#endif
    }
// mapping for MPI process LP
  for(i = 0; i < nlp_mpi_procs_per_pe; i++)
   {
      kpid = i % g_tw_nkp;

      pe = tw_getpe(kpid % g_tw_npe);

      tw_lp_onpe(nlp_router_per_pe + nlp_terminal_per_pe + i, pe, total_routers + total_terminals + g_tw_mynode * nlp_mpi_procs_per_pe + i + get_terminal_rem());
      tw_lp_onkp(g_tw_lp[nlp_router_per_pe + nlp_terminal_per_pe + i], g_tw_kp[kpid]);
      tw_lp_settype(nlp_router_per_pe + nlp_terminal_per_pe + i, &dragonfly_lps[2]);

#ifdef DEBUG
//    printf("\n [Node %d] Local Terminal ID %d Global Terminal ID %d ", g_tw_mynode, nlp_router_per_pe + i, total_routers + g_tw_mynode * nlp_terminal_per_pe + i + get_terminal_rem());
#endif
    }
}

int main(int argc, char **argv)
{
     char log[32];
     tw_opt_add(app_opt);
     tw_init(&argc, &argv);

     // UNIFORM_RANDOM
     // WORST_CASE
     // TRANSPOSE (send packets to the transpose of a 2D matrix)
     minimal_count = 0;
     nonmin_count = 0;
     num_packets = MESSAGE_SIZE / PACKET_SIZE;
     num_chunks = PACKET_SIZE / CHUNK_SIZE;
     max_packets = INJECTION_INTERVAL / MEAN_INTERVAL;

     total_routers=NUM_ROUTER*num_groups;
     total_terminals=NUM_ROUTER*NUM_TERMINALS*num_groups;
     total_mpi_procs = NUM_ROUTER*NUM_TERMINALS*num_groups;

//    Assume a one-to-one mapping of MPI processes to terminals/nodes
     nlp_terminal_per_pe = total_terminals/tw_nnodes()/g_tw_npe;
     nlp_mpi_procs_per_pe = total_mpi_procs/tw_nnodes()/g_tw_npe;

     terminal_rem = total_terminals % (tw_nnodes()/g_tw_npe);

#ifdef LOG_DRAGONFLY
     sprintf( log, "dragonfly-log.%d", g_tw_mynode );
     dragonfly_event_log = fopen( log, "w+");
     if( dragonfly_event_log == NULL )
        tw_error( TW_LOC, "Failed to Open DRAGONFLY Event Log file \n");
#endif
     if(g_tw_mynode < terminal_rem)
     {
       nlp_terminal_per_pe++;
       nlp_mpi_procs_per_pe++;
     }

     nlp_router_per_pe = total_routers/tw_nnodes()/g_tw_npe;

     router_rem = total_routers % (tw_nnodes()/g_tw_npe);

      if(g_tw_mynode < router_rem)
        nlp_router_per_pe++;

     range_start=nlp_router_per_pe + nlp_terminal_per_pe + nlp_mpi_procs_per_pe;

     g_tw_mapping=CUSTOM;
     g_tw_custom_initial_mapping=&dragonfly_mapping;
     g_tw_custom_lp_global_to_local_map=&dragonfly_mapping_to_lp;
     g_tw_events_per_pe = mem_factor * 1024 * (nlp_terminal_per_pe/g_tw_npe + nlp_router_per_pe/g_tw_npe) + opt_mem;

     tw_define_lps(range_start, sizeof(terminal_message), 0);

   
#if DEBUG
     //sprintf( log, "dragonfly-log.%d", g_tw_mynode );
     //dragonfly_event_log=fopen(log, "w+");

     //if(dragonfly_event_log == NULL)
	//tw_error(TW_LOC, "\n Failed to open dragonfly event log file \n");
#endif


#if DEBUG
     if(tw_ismaster())
	{
          printf("\n total_routers %d total_terminals %d g_tw_nlp is %d g_tw_npe %d g_tw_mynode: %d \n ", total_routers, total_terminals, (int)g_tw_nlp, (int)g_tw_npe, (int)g_tw_mynode);

	  printf("\n Arrival rate %f g_tw_mynode %d total %d nlp_terminal_per_pe is %d, nlp_router_per_pe is %d \n ", MEAN_INTERVAL, (int)g_tw_mynode, range_start, nlp_terminal_per_pe, nlp_router_per_pe);
	}
#endif
    packet_offset = (g_tw_ts_end/MEAN_INTERVAL) * num_packets;
    tw_run();

    if(tw_ismaster())
    {
      printf("\nDragonfly Network Model Statistics \n");
      printf("\t%-50s %11lld\n", "Number of nodes", nlp_terminal_per_pe * g_tw_npe * tw_nnodes());
//      printf("\n Slowest packet %lld ", max_packet);
      if(ROUTING == ADAPTIVE)
	      printf("\n ADAPTIVE ROUTING STATS: %d packets routed minimally %d packets routed non-minimally ", minimal_count, nonmin_count);
    }

    unsigned long long total_finished_storage[N_COLLECT_POINTS];
    unsigned long long total_generated_storage[N_COLLECT_POINTS];
    unsigned long long N_total_finish,N_total_hop;

   tw_stime total_time_sum,g_max_latency;

   int i;

   for( i=0; i<N_COLLECT_POINTS; i++ )
    {
     MPI_Reduce( &N_finished_storage[i], &total_finished_storage[i],1,
                 MPI_LONG_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
   
     MPI_Reduce( &N_generated_storage[i], &total_generated_storage[i],1,
                  MPI_LONG_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
     }
     MPI_Reduce( &total_time, &total_time_sum,1,
                    MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
   
     MPI_Reduce( &N_finished, &N_total_finish,1,
                    MPI_LONG_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
   
     MPI_Reduce( &total_hops, &N_total_hop,1,
                    MPI_LONG_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
   
     MPI_Reduce( &max_latency, &g_max_latency,1,
                    MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

    for( i=1; i<N_COLLECT_POINTS; i++ )
       {
         total_finished_storage[i]+=total_finished_storage[i-1];
         total_generated_storage[i]+=total_generated_storage[i-1];
       }
   
     if(tw_ismaster())
      {
           printf("\n ****************** \n");
           printf("\n total finish:         %lld and %lld; \n",
                   total_finished_storage[N_COLLECT_POINTS-1],N_total_finish);
           printf("\n total generate:       %lld; \n",
                  total_generated_storage[N_COLLECT_POINTS-1]);
           printf("\n total hops:           %lf; \n",
                   (double)N_total_hop/total_finished_storage[N_COLLECT_POINTS-1]);
           printf("\n average travel time:  %lf; \n\n",
                   total_time_sum/total_finished_storage[N_COLLECT_POINTS-1]);

/*          for( i=0; i<N_COLLECT_POINTS; i++ )
           {
             printf(" %d ",i*100/N_COLLECT_POINTS);
             printf("finish: %lld; generate: %lld; alive: %lld\n",
                     total_finished_storage[i],
                     total_generated_storage[i],
                     total_generated_storage[i]-total_finished_storage[i]);
           }

	tw_stime bandwidth;
	tw_stime interval = (g_tw_ts_end / N_COLLECT_POINTS);
	interval = interval / (1000.0 * 1000.0 * 1000.0); //convert seconds to ns
	for( i=1; i<N_COLLECT_POINTS; i++ )
	   {
		bandwidth = total_finished_storage[i] - total_finished_storage[i - 1];
		bandwidth = (bandwidth * PACKET_SIZE) / (1024.0 * 1024.0 * 1024.0);
		bandwidth = bandwidth / interval;
		printf("\n Interval %.7lf Bandwidth %lf ", interval, bandwidth);
	   }
            // capture the steady state statistics
          unsigned long long steady_sum=0;
          for( i = N_COLLECT_POINTS/2; i<N_COLLECT_POINTS;i++)
             steady_sum+=total_generated_storage[i]-total_finished_storage[i];
        
          printf("\n Steady state, packet alive: %lld\n",
                   2*steady_sum/N_COLLECT_POINTS);*/
          printf("\nMax latency is %lf\n\n",g_max_latency);

      }
   tw_end();
   return 0;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
