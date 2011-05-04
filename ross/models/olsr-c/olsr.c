#include <ross.h>
#include "olsr.h"

/*
 * Time in this model is microseconds
 */

tw_peid olsr_map(tw_lpid gid);

void olsr_region_init(olsr_mpr_state * s, tw_lp * lp);
void olsr_region_event_handler(olsr_mpr_state * s, tw_bf * bf, olsr_message * m, tw_lp * lp);
void olsr_region_event_handler_rc(olsr_mpr_state * s, tw_bf * bf, olsr_message * m, tw_lp * lp);

void olsr_station_to_mpr(olsr_mpr_state * s, tw_bf * bf, olsr_message * m, tw_lp * lp);
void olsr_mpr_to_mpr(olsr_mpr_state * s, tw_bf * bf, olsr_message * m, tw_lp * lp);

void olsr_station_to_mpr_rc(olsr_mpr_state * s, tw_bf * bf, olsr_message * m, tw_lp * lp);
void olsr_mpr_to_mpr_rc(olsr_mpr_state * s, tw_bf * bf, olsr_message * m, tw_lp * lp);

void olsr_change_mpr(olsr_mpr_state * s, tw_bf * bf, olsr_message * m, tw_lp * lp);
void olsr_change_region(olsr_mpr_state * s, tw_bf * bf, olsr_message * m, tw_lp * lp);

void olsr_region_finish(olsr_mpr_state * s, tw_lp * lp);

tw_lptype mylps[] = 
{
    {	(init_f) olsr_region_init,
        (event_f) olsr_region_event_handler,
        (revent_f) olsr_region_event_handler_rc,
        (final_f) olsr_region_finish,
        (map_f) olsr_map,
    	sizeof(olsr_mpr_state)},
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

void olsr_region_init(olsr_mpr_state * s, tw_lp * lp) 
{
  int i, j;
  unsigned int rng_calls;
  tw_bf init_bf;
  olsr_message m;
  s->failed_packets = 0;
  
for( j=0; j < OLSR_MPRS_PER_REGIONLP; j++) {
    for( i=0; i < OLSR_STATIONS_PER_MPR; i++) {
      m.type = OLSR_DATA_PACKET;
      m.station = i;
      m.mpr = j;
      s->mpr[j].stations[i].location.x = tw_rand_normal_sd(lp->rng, MAX_X_DIST, STD_DEV, &rng_calls);
      s->mpr[j].stations[i].location.y = tw_rand_normal_sd(lp->rng, MAX_Y_DIST, STD_DEV, &rng_calls);
      s->mpr[j].stations[i].location.z = 0;
      s->mpr[j].stations[i].tx_power = 10.0;
      s->mpr[j].stations[i].data_packet_time = DATA_PACKET_TIME + tw_rand_unif( lp->rng );
      olsr_region_event_handler(s, &init_bf, &m, lp);
    }
  }
}



//TODO: Factor out all this sucess/fail code.  It's the same in every function.

double olsr_hello_time(olsr_mpr_state * s, tw_lp * lp) {
  double time = 0.0;
  double dist = calculateGridDistance(s->mpr[m->mpr].stations[m->from_station].location, 
				        s->mpr[m->mpr].location);

 rf.signal = calcRxPower(s->stations[m->from_station].tx_power, dist, LAMBDA);
  printf("Region %d: Signal %lf, Power %lf, Distance %lf, Lambda %lf \n",
	 lp->gid, 
	 rf.signal, 
	 s->stations[m->station].tx_power, 
	 dist, 
	 LAMBDA);
  rf.bandwidth = WIFIB_BW;
  rf.noiseFigure = 1;
  rf.noiseInterference = 1;
  
  s->mpr[m->mpr].stations[m->station].region_snr = calculateSnr(rf);
  s->mpr[m->mpr].stations[m->station].region_success_rate = 
    OLSR_80211b_DsssDqpskCck11_SuccessRate(s->stations[m->station].region_snr, num_of_bits);

  printf("Region %d: Success Rate %lf, SNR %lf, Bits %d \n",
	 lp->gid, 
	 s->mpr[m->mpr].stations[m->station].region_success_rate, 
	 s->mpr[m->mpr].stations[m->station].region_snr, 
	 num_of_bits);

  unsigned int failing = 1;

  while(failing) {
     if( tw_rand_unif( lp->rng ) < s->mpr[m->mpr].stations[m->station].region_success_rate )
       failing = 0;
     else
       time += DATA_PACKET_TIME + tw_rand_unif( lp->rng );
  }
  return time;
} 


void olsr_station_to_mpr(olsr_mpr_state * s, tw_bf * bf, olsr_message * m, tw_lp * lp) 
{
  unsigned int rng_calls=0;
  tw_event *e=NULL;
  olsr_message *m_new=NULL;
  rf_signal rf;
  double dist = calculateGridDistance(s->mpr[m->mpr].stations[m->from_station].location, 
				      s->mpr[m->mpr].stations[m->to_station].location);
  
  rf.signal = calcRxPower(s->stations[m->from_station].tx_power, dist, LAMBDA);
  printf("Region %d: Signal %lf, Power %lf, Distance %lf, Lambda %lf \n",
	 lp->gid, 
	 rf.signal, 
	 s->stations[m->station].tx_power, 
	 dist, 
	 LAMBDA);
  rf.bandwidth = WIFIB_BW;
  rf.noiseFigure = 1;
  rf.noiseInterference = 1;
 
  s->mpr[m->mpr].stations[m->station].region_snr = calculateSnr(rf);
  s->mpr[m->mpr].stations[m->station].region_success_rate = 
    OLSR_80211b_DsssDqpskCck11_SuccessRate(s->stations[m->station].region_snr, num_of_bits);

  printf("Region %d: Success Rate %lf, SNR %lf, Bits %d \n",
	 lp->gid, 
	 s->mpr[m->mpr].stations[m->station].region_success_rate, 
	 s->mpr[m->mpr].stations[m->station].region_snr, 
	 num_of_bits);

  if( tw_rand_unif( lp->rng ) < s->mpr[m->mpr].stations[m->station].region_success_rate ) 
    {
      bf->c1 = 1;
      s->failed_packets++;
    }

  // Need to send packet out at tw_now + data packet time to self for the MPR and then that goes out 
  // between two MPRs.
  
  //TODO: Randomly decided which kind of event to send here 

  // schedule event back to AP w/ exponential service time
  //HELP:  Should the inter-message time be some multiple of data packet time, or is something like 10k ok?
  e = tw_event_new(lp->gid, tw_rand_exponential(lp->rng, 10000.0), lp);
  m_new = (olsr_message *) tw_event_data(e);
  m_new->type = OLSR_MPR_TO_MPR;
  m_new->from = m->mpr;
  m_new->mpr = tw_rand_unif( lp->rng ) % 4; //send to N, S, E, or W MPR
  tw_event_send(e);
}

void olsr_station_to_mpr_rc(olsr_mpr_state * s, tw_bf * bf, olsr_message * m, tw_lp * lp) 
{
  tw_rand_reverse_unif(lp->rng);
  tw_rand_reverse_unif(lp->rng);

  if( bf->c1 ) {
	s->failed_packets++;
  }
}

void olsr_mpr_to_mpr(olsr_mpr_state * s, tw_bf * bf, olsr_message * m, tw_lp * lp) 
{
  unsigned int rng_calls=0;
  tw_event *e=NULL;
  olsr_message *m_new=NULL;
  rf_signal rf;
	
  unsigned int to_x = tw_rand_unif( lp->rng ) % NUM_REGION_X;
  unsigned int to_y = tw_rand_unif( lp->rng ) % NUM_REGION_Y;

  //HELP: HOW to I calculate Next MPR or even the target MPR from X,Y for upcoming current part?

  rf_signal rf;
  double dist = calculateGridDistance(s->mpr[m->mpr].location,
				      s->mpr[].location);
  
  rf.signal = calcRxPower(s->stations[m->from_station].tx_power, dist, LAMBDA);
  printf("Region %d: Signal %lf, Power %lf, Distance %lf, Lambda %lf \n",
	 lp->gid, 
	 rf.signal, 
	 s->mpr[m->mpr].stations[m->station].tx_power, 
	 dist, 
	 LAMBDA);
  rf.bandwidth = WIFIB_BW;
  rf.noiseFigure = 1;
  rf.noiseInterference = 1;
  
  
  // packets coming from station to access point have less power and so lower snr
  s->mpr[m->mpr].stations[m->station].region_snr = calculateSnr(rf);
  s->mpr[m->mpr].stations[m->station].region_success_rate = 
    OLSR_80211b_DsssDqpskCck11_SuccessRate(s->stations[m->station].region_snr, num_of_bits);

  printf("Region %d: Success Rate %lf, SNR %lf, Bits %d \n",
	 lp->gid, 
	 s->mpr[m->mpr].stations[m->station].region_success_rate, 
	 s->mpr[m->mpr].stations[m->station].region_snr, 
	 num_of_bits);

  if( tw_rand_unif( lp->rng ) < s->stations[m->station].region_success_rate ) 
    {
      bf->c1 = 1;
      s->failed_packets++;
    }

  // Need to send packet out at tw_now + data packet time to self for the MPR and then that goes out 
  // between two MPRs.
  
  //TODO: Randomly decided which kind of event to send here 

  //TODO: RANDOMLY SELECT EVENT TO SEND
  //HELP: Do event RNGs have to be unrolled?
  e = tw_event_new(lp->gid, tw_rand_exponential(lp->rng, 10000.0), lp);
  m_new = (olsr_message *) tw_event_data(e);
  m_new->type = OLSR_STATION_TO_MPR;
  m_new->from_station = m->station;
  m_new->to_station = tw_rand_unif( lp->rng ) % OLSR_STATIONS_PER_MPR; 
  tw_event_send(e);
}

void olsr_mpr_to_mpr_rc(olsr_mpr_state * s, tw_bf * bf, olsr_message * m, tw_lp * lp) 
{
  tw_rand_reverse_unif(lp->rng);
  tw_rand_reverse_unif(lp->rng);
}

//HELP: Should we keep these? I think we should, to add small purturbations to distance.
void olsr_move(olsr_mpr_state * s, tw_bf * bf, olsr_message * m, tw_lp * lp) 
{
	unsigned int rng_calls;
	double x_move = tw_rand_normal_sd(lp->rng, 0, -0.02, &rng_calls);
	double y_move = tw_rand_normal_sd(lp->rng, 0, -0.02, &rng_calls);
	s->mpr[m->mpr].stations[m->station].location.x += x_move; 
	s->mpr[m->mpr].stations[m->station].location.y += y_move;
}

void olsr_move_rc(olsr_mpr_state * s, tw_bf * bf, olsr_message * m, tw_lp * lp) 
{
	tw_rand_reverse_unif(lp->rng);
	tw_rand_reverse_unif(lp->rng);
}

void olsr_change_mpr(olsr_mpr_state * s, tw_bf * bf, olsr_message * m, tw_lp * lp) {
  s->mpr[m->mpr].stations[m->station].mpr = tw_rand_unif( lp->rng ) % 4;


}

void olsr_change_mpr_region(olsr_mpr_state * s, tw_bf * bf, olsr_message * m, tw_lp * lp) {

}

void olsr_region_event_handler(olsr_mpr_state * s, tw_bf * bf, olsr_message * m, tw_lp * lp) {
  switch( m->type ) {
     case OLSR_STATION_TO_MPR:
       olsr_station_to_mpr(s, bf, m, lp); 
     break;
     
     case OLSR_MPR_TO_MPR:
       olsr_mpr_to_mpr(s, bf, m, lp);
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

void olsr_region_event_handler_rc(olsr_mpr_state * s, tw_bf * bf, olsr_message * m, tw_lp * lp) {
switch( m->type ) {
     case OLSR_STATION_TO_MPR:
       olsr_station_to_mpr_rc(s, bf, m, lp); 
     break;
     
     case OLSR_MPR_TO_MPR:
       olsr_mpr_to_mpr_rc(s, bf, m, lp);
     break;
     
     case OLSR_CHANGE_MPR:
       olsr_change_mpr_rc(s, bf, m, lp);
     break;
     
     case OLSR_CHANGE_CELL:
       olsr_change_cell_rc(s, bf, m, lp);
     break; 

     default:
       tw_error(TW_LOC, "Undefined type, corrupted message \n");
     break;
  }
}

void olsr_region_finish(olsr_mpr_state * s, tw_lp * lp)
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
    TWOPT_GROUP("802.11b OLSR Model"),
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
