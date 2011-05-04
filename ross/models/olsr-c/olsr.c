#include <ross.h>
#include "olsr.h"

/*
 * Time in this model is microseconds
 */

tw_peid olsr_map(tw_lpid gid);

void olsr_region_init(olsr_region_state * s, tw_lp * lp);
void olsr_region_event_handler(olsr_region_state * s, tw_bf * bf, olsr_message * m, tw_lp * lp);
void olsr_region_event_handler_rc(olsr_region_state * s, tw_bf * bf, olsr_message * m, tw_lp * lp);

void olsr_station_to_mpr(olsr_region_state * s, tw_bf * bf, olsr_message * m, tw_lp * lp);
void olsr_station_to_mpr_rc(olsr_region_state * s, tw_bf * bf, olsr_message * m, tw_lp * lp);

void olsr_departure_from_mpr(olsr_region_state * s, tw_bf * bf, olsr_message * m, tw_lp * lp);
void olsr_departure_from_mpr_rc(olsr_region_state * s, tw_bf * bf, olsr_message * m, tw_lp * lp);

void olsr_change_mpr(olsr_region_state * s, tw_bf * bf, olsr_message * m, tw_lp * lp);
void olsr_change_mpr_rc(olsr_region_state * s, tw_bf * bf, olsr_message * m, tw_lp * lp);

void olsr_change_region(olsr_region_state * s, tw_bf * bf, olsr_message * m, tw_lp * lp);
void olsr_change_region_rc(olsr_region_state * s, tw_bf * bf, olsr_message * m, tw_lp * lp);

void olsr_region_finish(olsr_region_state * s, tw_lp * lp);

tw_lptype mylps[] = 
  {
    {	(init_f) olsr_region_init,
        (event_f) olsr_region_event_handler,
        (revent_f) olsr_region_event_handler_rc,
        (final_f) olsr_region_finish,
        (map_f) olsr_map,
    	sizeof(olsr_region_state)},
    {0},
  };

tw_peid olsr_map(tw_lpid gid) 
{
  //return (tw_peid) gid / g_tw_nlp;
  long lp_x = gid % NUM_REGIONS_X;
  long lp_y = gid / NUM_REGIONS_X;
  long vp_num_x = lp_x/g_regions_per_vp_x;
  long vp_num_y = lp_y/g_regions_per_vp_y;
  long vp_num = vp_num_x + (vp_num_y*NUM_VP_X);  
  tw_peid peid = vp_num/g_vp_per_proc;  
  return peid;
}

tw_lp *olsr_map_to_lp( tw_lpid lpid )
{
  tw_lpid lp_x = lpid % NUM_REGIONS_X; //lpid -> (lp_x,lp_y)
  tw_lpid lp_y = lpid / NUM_REGIONS_X;
  tw_lpid vp_index_x = lp_x % g_regions_per_vp_x;
  tw_lpid vp_index_y = lp_y % g_regions_per_vp_y;
  tw_lpid vp_index = vp_index_x + (vp_index_y * (g_regions_per_vp_x));
  tw_lpid vp_num_x = lp_x/g_regions_per_vp_x;
  tw_lpid vp_num_y = lp_y/g_regions_per_vp_y;
  tw_lpid vp_num = vp_num_x + (vp_num_y*NUM_VP_X);  
  vp_num = vp_num % g_vp_per_proc;
  tw_lpid index = vp_index + vp_num*g_regions_per_vp;

#ifdef ROSS_runtime_check  
  if( index >= g_tw_nlp )
    tw_error(TW_LOC, "index (%llu) beyond g_tw_nlp (%llu) range \n", index, g_tw_nlp);
#endif /* ROSS_runtime_check */
  
  return g_tw_lp[index];

}

tw_lpid olsr_map_to_local_index( tw_lpid lpid )
{
  tw_lpid lp_x = lpid % NUM_REGIONS_X; //lpid -> (lp_x,lp_y)
  tw_lpid lp_y = lpid / NUM_REGIONS_X;
  tw_lpid vp_index_x = lp_x % g_regions_per_vp_x;
  tw_lpid vp_index_y = lp_y % g_regions_per_vp_y;
  tw_lpid vp_index = vp_index_x + (vp_index_y * (g_regions_per_vp_x));
  tw_lpid vp_num_x = lp_x/g_regions_per_vp_x;
  tw_lpid vp_num_y = lp_y/g_regions_per_vp_y;
  tw_lpid vp_num = vp_num_x + (vp_num_y*NUM_VP_X);  
  vp_num = vp_num % g_vp_per_proc;
  tw_lpid index = vp_index + vp_num*g_regions_per_vp;
  
  if( index >= g_tw_nlp )
    tw_error(TW_LOC, "index (%llu) beyond g_tw_nlp (%llu) range \n", index, g_tw_nlp);
  
  return( index );
}

/* Generation Initial Mapping of LPs to PEs, etc */
void olsr_grid_mapping()
{
  tw_lpid         x, y;
  tw_lpid         lpid, kpid;
  tw_lpid         num_regions_per_kp, vp_per_proc;
  tw_lpid         local_lp_count;
  
  num_regions_per_kp = (NUM_REGIONS_X * NUM_REGIONS_Y) / (NUM_VP_X * NUM_VP_Y);
  vp_per_proc = (NUM_VP_X * NUM_VP_Y) / ((tw_nnodes() * g_tw_npe)) ;
  g_tw_nlp = nlp_per_pe;
  g_tw_nkp = vp_per_proc;

  local_lp_count=0;
  for (y = 0; y < NUM_REGIONS_Y; y++)
    {
      for (x = 0; x < NUM_REGIONS_X; x++)
	{
	  lpid = (x + (y * NUM_REGIONS_X));
	  if( g_tw_mynode == olsr_map(lpid) )
	    {
	      
	      kpid = local_lp_count/num_regions_per_kp;
	      local_lp_count++; // MUST COME AFTER!! DO NOT PRE-INCREMENT ELSE KPID is WRONG!!
	      
	      if( kpid >= g_tw_nkp )
		tw_error(TW_LOC, "Attempting to mapping a KPid (%llu) for Global LPid %llu that is beyond g_tw_nkp (%llu)\n",
			 kpid, lpid, g_tw_nkp );
	      
	      tw_lp_onpe(olsr_map_to_local_index(lpid), g_tw_pe[0], lpid);
	      if( g_tw_kp[kpid] == NULL )
		tw_kp_onpe(kpid, g_tw_pe[0]);
	      tw_lp_onkp(g_tw_lp[olsr_map_to_local_index(lpid)], g_tw_kp[kpid]);
	      tw_lp_settype( olsr_map_to_local_index(lpid), &mylps[0]);
	    }
	}
    }
}

void olsr_region_init(olsr_region_state * s, tw_lp * lp) 
{
  int i, j;
    double distance = 0.0;
  double min_distance = REGION_SIZE;
  unsigned int my_mpr=0;
  tw_event *e = NULL;
  olsr_message *m=NULL;
  rf_signal rf;

  // Init the MPRs
  for( i=0; i < OLSR_MAX_MPRS_PER_REGION; i++) 
    {
      s->mpr[i].failed_packets = 0;
      s->mpr[i].sent_packets = 0;
      s->mpr[i].waiting_packets = 0;
      rf.signal = calcRxPower(s->station[i].tx_power, .2*REGION_SIZE, LAMBDA);
      rf.bandwidth = WIFIB_BW;
      rf.noiseFigure = 1;
      rf.noiseInterference = 1;
      s->mpr[i].snr = calculateSnr(rf);
      s->mpr[i].success_rate = 
	OLSR_80211b_DsssDqpskCck11_SuccessRate(s->mpr[i].snr, (DATA_PACKET_SIZE * 8));
      s->mpr[i].tx_power = OLSR_MPR_POWER;

      switch( i )
	{
	case OLSR_NORTH: 
	  s->mpr[i].location.x = REGION_SIZE/2.0;
	  s->mpr[i].location.y = REGION_SIZE * 0.9;
	  break;
	case OLSR_SOUTH: 
	  s->mpr[i].location.x = REGION_SIZE/2.0;
	  s->mpr[i].location.y = REGION_SIZE * 0.1;
	  break;
	case OLSR_EAST: 
	  s->mpr[i].location.x = REGION_SIZE * 0.9;
	  s->mpr[i].location.y = REGION_SIZE/2.0;
	  break;
	case OLSR_WEST: 
	  s->mpr[i].location.x = REGION_SIZE * 0.1;
	  s->mpr[i].location.y = REGION_SIZE/2.0;
	  break;
	default:
	  tw_error(TW_LOC, "Bad OLSR Direction %d", i );
	}
      // a mpr does not have an owning mpr so just init to max
      s->mpr[i].my_mpr = OLSR_MAX_MPRS_PER_REGION; 
    }

  // Init the station -- only init 1/2 of max
  for( i=0; i < OLSR_MAX_STATIONS_PER_REGION/2; i++) 
    {
      s->station[i].failed_packets = 0;
      s->station[i].sent_packets = 0;
      s->station[i].waiting_packets = 0;
      s->station[i].tx_power = OLSR_MPR_POWER;
      s->station[i].location.x = tw_rand_unif( lp->rng ) * REGION_SIZE;
      s->station[i].location.y = tw_rand_unif( lp->rng ) * REGION_SIZE;
      
      // find closest MPR to each station
      for( j = 0; j < OLSR_MAX_MPRS_PER_REGION; j++ )
	{
	  distance = calculateGridDistance(s->mpr[j].location, s->station[i].location);
	  if( distance < min_distance )
	    {
	      distance = min_distance;
	      my_mpr = j;
	    }
	}

      s->station[i].my_mpr = my_mpr;

      rf.signal = calcRxPower(s->station[i].tx_power, min_distance, LAMBDA);
      rf.bandwidth = WIFIB_BW;
      rf.noiseFigure = 1;
      rf.noiseInterference = 1;
      s->station[i].snr = calculateSnr(rf);
      s->station[i].success_rate = 
	OLSR_80211b_DsssDqpskCck11_SuccessRate(s->station[i].snr, (DATA_PACKET_SIZE * 8));

      /*
       * Schedule each station's to MPR packet
       */
      e = tw_event_new(lp->gid, tw_rand_exponential(lp->rng, MEAN_TIME_BETWEEN_DATA_PACKETS), lp);
      m = (olsr_message *) tw_event_data(e);
      m->type = OLSR_STATION_TO_MPR;
      m->mpr = my_mpr;
      m->station = i;
      tw_event_send(e);
    }

  s->num_mprs = OLSR_MAX_MPRS_PER_REGION;
  s->num_stations = OLSR_MAX_STATIONS_PER_REGION/2;
  s->region_location.x = lp->gid / NUM_REGIONS_X;
  s->region_location.y = lp->gid % NUM_REGIONS_X;
}

double olsr_hello_time(olsr_region_state * s, tw_lp * lp) 
{

  /* based on HICSS Paper 2002, hello costs are (log N)^2 since it cost log N to
     compute the first set of MPRs and to find a good near optimal set it will 
     cost log N times.
  */ 
  double time = log((s->num_mprs+s->num_stations))/log(2);
  time = time * time * HELLO_PACKET_TIME;
  return time;
} 

void olsr_station_to_mpr(olsr_region_state * s, tw_bf * bf, olsr_message * m, tw_lp * lp) 
{
  tw_event *e=NULL;
  olsr_message *m_new=NULL;
  tw_stime packet_time = 0.0;
  
  if( tw_rand_unif( lp->rng ) > s->station[m->station].success_rate ) 
    {
      bf->c1 = 1;
      s->station[m->station].failed_packets++;
    }

  s->station[m->station].sent_packets++;

  // Schedule packet betwen station and MPR
  packet_time = DATA_PACKET_TIME + tw_rand_unif( lp->rng );
  e = tw_event_new(lp->gid, packet_time, lp);
  m_new = (olsr_message *) tw_event_data(e);
  m_new->type = OLSR_ARRIVAL_TO_MPR;
  m_new->station = m->station;
  m_new->mpr = m->mpr;
  m_new->max_hop_count = tw_rand_integer( lp->rng, 1, OLSR_MAX_HOPS );
  m_new->hop_count = 1;
  tw_event_send(e);

  // Schedule next packet send
  e = tw_event_new(lp->gid, tw_rand_exponential(lp->rng, MEAN_TIME_BETWEEN_DATA_PACKETS), lp);
  m_new = (olsr_message *) tw_event_data(e);
  m_new->type = OLSR_STATION_TO_MPR;
  m_new->station = m->station;
  m_new->mpr = m->mpr;
  m_new->max_hop_count = 0;
  m_new->hop_count = 0;
  tw_event_send(e);

}

void olsr_station_to_mpr_rc(olsr_region_state * s, tw_bf * bf, olsr_message * m, tw_lp * lp) 
{
  tw_rand_reverse_unif(lp->rng);
  tw_rand_reverse_unif(lp->rng);
  tw_rand_reverse_unif(lp->rng);

  s->station[m->station].sent_packets--;

  if( bf->c1 ) 
    {
      s->station[m->station].failed_packets--;
    }
}

void olsr_arrival_to_mpr(olsr_region_state * s, tw_bf * bf, olsr_message * m, tw_lp * lp) 
{
  tw_event *e = NULL;
  olsr_message *new_m=NULL;
  tw_stime ts;

  // increment waiting packets
  s->mpr[m->mpr].waiting_packets++;
  
  // check if packet is delivered to region
  if( m->hop_count == m->max_hop_count)
    {
      bf->c1 = 1;
      s->mpr[m->mpr].packets_delivered++;
      return;
    }

  // Schedule packets departure MPR to MPR events
  ts = tw_rand_exponential( lp->rng, OLSR_MPR_PACKET_SERVICE_TIME) * s->mpr[m->mpr].waiting_packets;
  e = tw_event_new( lp->gid, ts, lp );
  new_m = (olsr_message *) tw_event_data(e);
  new_m->type = OLSR_DEPARTURE_FROM_MPR;
  new_m->mpr = m->mpr;
  new_m->station = m->station;
  new_m->max_hop_count = m->max_hop_count;
  new_m->hop_count = m->hop_count;
}

void olsr_departure_from_mpr(olsr_region_state * s, tw_bf * bf, olsr_message * m, tw_lp * lp) 
{
  tw_event *e=NULL;
  olsr_message *m_new=NULL;
  rf_signal rf;
  double success_rate;
  int direction;
  int next_x;
  int next_y;
  int next_mpr = -1;
  tw_lpid destlp;
  double distance;
  tw_stime packet_time;

  direction = tw_rand_integer( lp->rng, 0, OLSR_WEST );

  switch(m->mpr)
    {
    case OLSR_NORTH:
      switch(direction)
	{
	case OLSR_NORTH:
	  next_x = s->region_location.x;
          next_y = (s->region_location.y + 1) % NUM_REGIONS_Y;
	  next_mpr = OLSR_SOUTH;
	  break;

	case OLSR_SOUTH:
	case OLSR_EAST:
	case OLSR_WEST:
	  next_x = s->region_location.x;
          next_y = s->region_location.y;
	  next_mpr = direction;
	  break;
	default:
	  tw_error(TW_LOC, "Bad Direction %d \n", direction );
	}
      break;

    case OLSR_SOUTH:
      switch(direction)
	{
	case OLSR_SOUTH:
	  next_x = s->region_location.x;
          next_y = (s->region_location.y + NUM_REGIONS_Y - 1) % NUM_REGIONS_Y;
	  next_mpr = OLSR_NORTH;
	  break;

	case OLSR_NORTH:
	case OLSR_EAST:
	case OLSR_WEST:
	  next_x = s->region_location.x;
          next_y = s->region_location.y;
	  next_mpr = direction;
	  break;
	default:
	  tw_error(TW_LOC, "Bad Direction %d \n", direction );
	}
      break;

    case OLSR_EAST:
      switch(direction)
	{
	case OLSR_EAST:
	  next_x = (s->region_location.x + 1) % NUM_REGIONS_X;
          next_y = s->region_location.y;
	  next_mpr = OLSR_WEST;
	  break;

	case OLSR_NORTH:
	case OLSR_SOUTH:
	case OLSR_WEST:
	  next_x = s->region_location.x;
          next_y = s->region_location.y;
	  next_mpr = direction;
	  break;
	default:
	  tw_error(TW_LOC, "Bad Direction %d \n", direction);
	}
      break;

    case OLSR_WEST:
      switch(direction)
	{
	case OLSR_WEST:
	  next_x = (s->region_location.x + NUM_REGIONS_X - 1) % NUM_REGIONS_X;
          next_y = s->region_location.y;
	  next_mpr = OLSR_WEST;
	  break;

	case OLSR_NORTH:
	case OLSR_SOUTH:
	case OLSR_EAST:
	  next_x = s->region_location.x;
          next_y = s->region_location.y;
	  next_mpr = direction;
	  break;
	default:
	  tw_error(TW_LOC, "Bad Direction %d \n", direction);
	}
      break;

      break;
    default:
      tw_error(TW_LOC, "Bad Direction %d \n", m->mpr );
    }

  // Check to see if message to self LP or remote LP
  if( next_x == s->region_location.x && 
      next_y == s->region_location.y)
    {
      // schedule MPR Arrvial to Self
      distance = calculateGridDistance(s->mpr[m->mpr].location, s->mpr[direction].location);
      rf.signal = calcRxPower(s->station[m->station].tx_power, distance, LAMBDA);
      rf.bandwidth = WIFIB_BW;
      rf.noiseFigure = 1;
      rf.noiseInterference = 1;
      success_rate = OLSR_80211b_DsssDqpskCck11_SuccessRate( calculateSnr(rf), (DATA_PACKET_SIZE * 8));
      destlp = lp->gid;
    }
  else
    {
      success_rate = s->mpr[m->mpr].success_rate;
      destlp = next_x * NUM_REGIONS_X + next_y;
    }

  if( tw_rand_unif( lp->rng) > success_rate )
    {
      bf->c1 = 1;
      s->mpr[m->mpr].failed_packets++;
      return;
    }

  // Schedule Departure if network allowed

  packet_time = DATA_PACKET_TIME + tw_rand_unif( lp->rng );
  e = tw_event_new(lp->gid, packet_time, lp);
  m_new = (olsr_message *) tw_event_data(e);
  m_new->type = OLSR_ARRIVAL_TO_MPR;
  m_new->station = m->station;
  m_new->mpr = m->mpr;
  m_new->max_hop_count = m->max_hop_count;
  m_new->hop_count = m->hop_count + 1;
  tw_event_send(e);
}

void olsr_departure_from_mpr_rc(olsr_region_state * s, tw_bf * bf, olsr_message * m, tw_lp * lp) 
{
  tw_rand_reverse_unif(lp->rng);
  tw_rand_reverse_unif(lp->rng);
}

//HELP: Should we keep these? I think we should, to add small purturbations to distance.
void olsr_move(olsr_region_state * s, tw_bf * bf, olsr_message * m, tw_lp * lp) 
{
  /* unsigned int rng_calls; */
  /* double x_move = tw_rand_normal_sd(lp->rng, 0, -0.02, &rng_calls); */
  /* double y_move = tw_rand_normal_sd(lp->rng, 0, -0.02, &rng_calls); */
  /* s->mpr[m->mpr].stations[m->station].location.x += x_move;  */
  /* s->mpr[m->mpr].stations[m->station].location.y += y_move; */
}

void olsr_move_rc(olsr_region_state * s, tw_bf * bf, olsr_message * m, tw_lp * lp) 
{
  tw_rand_reverse_unif(lp->rng);
  tw_rand_reverse_unif(lp->rng);
}

void olsr_change_mpr(olsr_region_state * s, tw_bf * bf, olsr_message * m, tw_lp * lp) 
{

}

void olsr_change_mpr_rc(olsr_region_state * s, tw_bf * bf, olsr_message * m, tw_lp * lp) 
{

}

void olsr_change_region(olsr_region_state * s, tw_bf * bf, olsr_message * m, tw_lp * lp) 
{

}

void olsr_change_region_rc(olsr_region_state * s, tw_bf * bf, olsr_message * m, tw_lp * lp) 
{

}

void olsr_region_event_handler(olsr_region_state * s, tw_bf * bf, olsr_message * m, tw_lp * lp) 
{
  switch( m->type ) 
    {
    case OLSR_STATION_TO_MPR:
      olsr_station_to_mpr(s, bf, m, lp); 
      break;
       
    case OLSR_ARRIVAL_TO_MPR:
      olsr_arrival_to_mpr(s, bf, m, lp); 
      break;
      
    case OLSR_DEPARTURE_FROM_MPR:
      olsr_departure_from_mpr(s, bf, m, lp);
      break;
      
    case OLSR_CHANGE_MPR:
      olsr_change_mpr(s, bf, m, lp);
      break;
      
    case OLSR_CHANGE_REGION:
      olsr_change_region(s, bf, m, lp);
      break; 
      
    default:
      tw_error(TW_LOC, "Undefined type, corrupted message \n");
      break;
    }
}

void olsr_region_event_handler_rc(olsr_region_state * s, tw_bf * bf, olsr_message * m, tw_lp * lp) 
{
  switch( m->type ) 
    {
    case OLSR_STATION_TO_MPR:
      olsr_station_to_mpr_rc(s, bf, m, lp); 
      break;
      
    case OLSR_DEPARTURE_FROM_MPR:
      olsr_departure_from_mpr_rc(s, bf, m, lp);
      break;
      
    case OLSR_CHANGE_MPR:
      olsr_change_mpr_rc(s, bf, m, lp);
      break;
      
    case OLSR_CHANGE_REGION:
      olsr_change_region_rc(s, bf, m, lp);
      break; 
      
    default:
      tw_error(TW_LOC, "Undefined type, corrupted message \n");
      break;
    }
}

void olsr_region_finish(olsr_region_state * s, tw_lp * lp)
{
  int i;
  unsigned long long station_failed_packets=0;
  
  for( i=0; i < OLSR_MAX_STATIONS_PER_REGION; i++)
    {
      station_failed_packets += s->station[i].failed_packets;
    }
  printf("LP %lld had %lld failed Access Point to Station packets\n", lp->gid, 
	 station_failed_packets);
}

const tw_optdef app_opt[] =
  {
    TWOPT_GROUP("802.11b OLSR Model"),
    TWOPT_UINT("memory", optimistic_memory, "additional memory buffers"),
    TWOPT_CHAR("run", run_id, "user supplied run name"),
    TWOPT_END()
  };



int
main(int argc, char **argv, char **env)
{
  tw_lpid         num_regions_per_kp, vp_per_proc;
  
  lookahead = 1.0;
  tw_opt_add(app_opt);
  tw_init(&argc, &argv);

  nlp_per_pe = (NUM_REGIONS_X * NUM_REGIONS_Y) / (tw_nnodes() * g_tw_npe);  
  g_tw_memory_nqueues = 16;
  
  offset_lpid = g_tw_mynode * nlp_per_pe;
  ttl_lps = tw_nnodes() * g_tw_npe * nlp_per_pe;
  
  g_tw_events_per_pe = (mult * nlp_per_pe * OLSR_MAX_STATIONS_PER_REGION) + optimistic_memory;
  g_tw_lookahead = lookahead;

  num_regions_per_kp = (NUM_REGIONS_X * NUM_REGIONS_Y) / (NUM_VP_X * NUM_VP_Y);
  vp_per_proc = (NUM_VP_X * NUM_VP_Y) / ((tw_nnodes() * g_tw_npe)) ;
  g_vp_per_proc = vp_per_proc;
  g_tw_nlp = nlp_per_pe;
  g_tw_nkp = vp_per_proc;

  g_tw_mapping = CUSTOM;
  g_tw_custom_initial_mapping = &olsr_grid_mapping;
  g_tw_custom_lp_global_to_local_map = &olsr_map_to_lp;
  
  tw_define_lps(nlp_per_pe, sizeof(olsr_message), 0);
  
  tw_run();

  //TODO:  Add MPI_Reduces Here to collect
  // Total sent packets
  // Total failed packets
  // Effective Bandwidth
  // Average failed packets
  // Time spent in Hello
  // 

  tw_end();

  return 0;
}
