#include <ross.h>
#include "olsr.h"

/*
 * Time in this model is microseconds
 */

tw_peid olsr_map(tw_lpid gid);

void olsr_region_init(olsr_region_state * s, tw_lp * lp);
void olsr_region_event_handler(olsr_region_state * s, tw_bf * bf, olsr_message * m, tw_lp * lp);
void olsr_region_event_handler_rc(olsr_region_state * s, tw_bf * bf, olsr_message * m, tw_lp * lp);

void olsr_region_arrival(olsr_region_state * s, tw_bf * bf, olsr_message * m, tw_lp * lp);
void olsr_station_arrival(olsr_region_state * s, tw_bf * bf, olsr_message * m, tw_lp * lp);

void olsr_region_arrival_rc(olsr_region_state * s, tw_bf * bf, olsr_message * m, tw_lp * lp);
void olsr_station_arrival_rc(olsr_region_state * s, tw_bf * bf, olsr_message * m, tw_lp * lp);

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
  int i;
  unsigned int rng_calls;
  tw_bf init_bf;
  olsr_message m;
  s->failed_packets = 0;
  
  // schedule out initial packet from access point
  for( i=0; i < OLSR_MAX_STATIONS_PER_REGION; i++) 
    {
    m.type = OLSR_DATA_PACKET;
    m.station = i;
    s->stations[i].location.x = tw_rand_normal_sd(lp->rng, MAX_X_DIST, STD_DEV, &rng_calls);
    s->stations[i].location.y = tw_rand_normal_sd(lp->rng, MAX_Y_DIST, STD_DEV, &rng_calls);
    s->stations[i].location.z = 0;
    s->stations[i].tx_power = 10.0;
    s->stations[i].data_packet_time = DATA_PACKET_TIME + tw_rand_unif( lp->rng );
    olsr_region_event_handler(s, &init_bf, &m, lp);
    }
}

void olsr_region_arrival(olsr_region_state * s, tw_bf * bf, olsr_message * m, tw_lp * lp) 
{
  unsigned int rng_calls=0;
  tw_event *e=NULL;
  olsr_message *m_new=NULL;
  rf_signal rf;
  double dist = calculateGridDistance(s->stations[m->from_station].location, 
				      s->stations[m->to_station].location);
  
  rf.signal = calcRxPower(s->stations[m->from_station].tx_power, dist, LAMBDA);
  printf("Region %d: Signal %lf, Power %lf, Distance %lf, Lambda %lf \n",
	 lp->gid, 
	 rf.signal, 
	 s->stations[m->from_station].tx_power, 
	 dist, 
	 LAMBDA);
  rf.bandwidth = WIFIB_BW;
  rf.noiseFigure = 1;
  rf.noiseInterference = 1;
  
  
  // packets coming from station to access point have less power and so lower snr
  s->stations[m->station].region_snr = calculateSnr(rf);
  s->stations[m->station].region_success_rate = 
    OLSR_80211b_DsssDqpskCck11_SuccessRate(s->stations[m->station].region_snr, num_of_bits);

  printf("Region %d: Success Rate %lf, SNR %lf, Bits %d \n",
	 lp->gid, 
	 s->stations[m->station].region_success_rate, 
	 s->stations[m->station].region_snr, 
	 num_of_bits);

  if( tw_rand_unif( lp->rng ) < s->stations[m->station].region_success_rate ) 
    {
      bf->c1 = 1;
      s->failed_packets++; // count all failed arrivals coming to access point
    }

  // Need to send packet out at tw_now + data packet time to self for the MPR and then that goes out 
  // between two MPRs.
  
  // schedule event back to AP w/ exponential service time
  e = tw_event_new(lp->gid, tw_rand_exponential(lp->rng, 10.0), lp);
  m_new = (olsr_message *) tw_event_data(e);
  m_new->type = OLSR_PACKET_ARRIVAL_AT_STATION;
  m_new->station = m->station;
  tw_event_send(e);
}

void olsr_region_arrival_rc(olsr_region_state * s, tw_bf * bf, olsr_message * m, tw_lp * lp) 
{
  // packets coming from access point have much more power and so better snr
  tw_rand_reverse_unif(lp->rng);
  tw_rand_reverse_unif(lp->rng);

  if( bf->c1 ) {
	s->failed_packets++;
  }
}

void olsr_station_arrival(olsr_region_state * s, tw_bf * bf, olsr_message * m, tw_lp * lp) 
{
  unsigned int rng_calls=0;
  tw_event *e=NULL;
  olsr_message *m_new=NULL;
  rf_signal rf;
	
  double dist = calculateGridDistance(s->stations[m->from_station].location, s->stations[m->to_station].location);
	
  rf.signal = calcRxPower(s->stations[m->from_station].tx_power, dist, LAMBDA);
  rf.bandwidth = WIFIB_BW;
  rf.noiseFigure = 1;
  rf.noiseInterference = 1;

  // packets coming from station to access point have less power and so lower snr
  s->stations[m->station].region_snr = calculateSnr(rf);
  s->stations[m->station].region_success_rate = 
    OLSR_80211b_DsssDqpskCck11_SuccessRate(s->stations[m->station].region_snr, num_of_bits);
  if( tw_rand_normal_sd(lp->rng,0.5,0.1, &rng_calls) < 
      s->stations[m->station].region_success_rate) 
    {
      bf->c1 = 1;
      s->failed_packets++; // count all failed arrivals coming to access point
    }
  // schedule event back to AP w/ exponential service time
  e = tw_event_new(lp->gid, tw_rand_exponential(lp->rng, 10.0), lp);
  m_new = (olsr_message *) tw_event_data(e);
  m_new->type = OLSR_PACKET_ARRIVAL_AT_REGION;
  m_new->station = m->station;
  tw_event_send(e);
}

void olsr_station_arrival_rc(olsr_region_state * s, tw_bf * bf, olsr_message * m, tw_lp * lp) 
{
  tw_rand_reverse_unif(lp->rng);
  tw_rand_reverse_unif(lp->rng);

  if( bf->c1 )
    {
      s->stations[m->station].failed_packets--;
    }
}

void olsr_move(olsr_region_state * s, tw_bf * bf, olsr_message * m, tw_lp * lp) 
{
	unsigned int rng_calls;
	double x_move = tw_rand_normal_sd(lp->rng, 0, -0.02, &rng_calls);
	double y_move = tw_rand_normal_sd(lp->rng, 0, -0.02, &rng_calls);
	s->stations[m->station].location.x += x_move; 
	s->stations[m->station].location.y += y_move;
}

void olsr_move_rc(olsr_region_state * s, tw_bf * bf, olsr_message * m, tw_lp * lp) 
{
	tw_rand_reverse_unif(lp->rng);
	tw_rand_reverse_unif(lp->rng);
	
}

void olsr_region_event_handler(olsr_region_state * s, tw_bf * bf, olsr_message * m, tw_lp * lp) 
{
  switch( m->type ) 
    {	
    case OLSR_STATION_MOVE:
      olsr_move(s, bf, m, lp);
      break;
      
    case OLSR_PACKET_ARRIVAL_AT_REGION:
      olsr_region_arrival(s, bf, m, lp);
      break;
      
    case OLSR_PACKET_ARRIVAL_AT_STATION:
      olsr_station_arrival(s, bf, m, lp);
      break;
      
    default:
      tw_error(TW_LOC, "Undefined type, corrupted message \n");
      break;
    }
}

void olsr_region_event_handler_rc(olsr_region_state * s, tw_bf * bf, olsr_message * m, tw_lp * lp) {
  switch( m->type ) 
    {
    case OLSR_STATION_MOVE:
      olsr_move_rc(s, bf, m, lp);
      break;
      
    case OLSR_PACKET_ARRIVAL_AT_REGION:
      olsr_region_arrival_rc(s, bf, m, lp);
      break;
      
    case OLSR_PACKET_ARRIVAL_AT_STATION:
      olsr_station_arrival_rc(s, bf, m, lp);
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
  printf("LP %llu had %d failed Station to Access Point packets\n", lp->gid, s->failed_packets);

  for( i=0; i < OLSR_MAX_STATIONS_PER_REGION; i++)
    {
      station_failed_packets += s->stations[i].failed_packets;
    }
  printf("LP %llu had %llu failed Access Point to Station packets\n", lp->gid, 
	 station_failed_packets);
}

const tw_optdef app_opt[] =
  {
    TWOPT_GROUP("802.11b Model"),
    TWOPT_UINT("nlp", nlp_per_pe, "number of LPs per processor"),
    TWOPT_STIME("mean", mean, "exponential distribution mean for timestamps"),
    TWOPT_STIME("mult", mult, "multiplier for event memory allocation"),
    TWOPT_STIME("lookahead", lookahead, "lookahead for events"),
    TWOPT_UINT("start-events", g_olsr_start_events, "number of initial messages per LP"),
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
  
  g_tw_events_per_pe = (mult * nlp_per_pe * g_olsr_start_events)+ optimistic_memory;
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
  tw_end();

	return 0;
}
