#include "olsr.h"

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

tw_peid olsr_map(tw_lpid gid) {
	return (tw_peid) gid / g_tw_nlp;
}

void olsr_region_init(olsr_region_state * s, tw_lp * lp)
{
  int i;
  tw_bf init_bf;
  olsr_message m;
  s->failed_packets = 0;

  // schedule out initial packet from access point
  for( i=0; i < OLSR_MAX_STATIONS_PER_region; i++)
    {
	  m.type = OLSR_DATA_PACKET;
      m.station = i;
      olsr_region_event_handler(s, &init_bf, &m, lp);
    }
}


void olsr_region_arrival(olsr_region_state * s, tw_bf * bf, olsr_message * m, tw_lp * lp) 
{
  unsigned int rng_calls=0;
  tw_event *e=NULL;
  olsr_message *m_new=NULL;

  // packets coming from station to access point have less power and so lower snr
  s->stations[m->station].region_snr = tw_rand_normal_sd(lp->rng,1.0,5.0, &rng_calls);
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

  // packets coming from access point have much more power and so better snr
  s->stations[m->station].station_snr = calculateSnr(m.rf)
  s->stations[m->station].station_success_rate = 
    OLSR_80211b_DsssDqpskCck11_SuccessRate(s->stations[m->station].station_snr, num_of_bits);
  if( tw_rand_normal_sd(lp->rng,0.5,0.1, &rng_calls) < 
      s->stations[m->station].station_success_rate) 
    {
      bf->c1 = 1;
      s->stations[m->station].failed_packets++;
    }
  
  // schedule event back to AP w/ exponential service time
  e = tw_event_new(lp->gid, tw_rand_exponential(lp->rng, 10.0), lp);
  m_new = (olsr_message *) tw_event_data(e);
  m_new->type = OLSR_PACKET_ARRIVAL_AT_region;
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


void olsr_region_event_handler(olsr_region_state * s, tw_bf * bf, olsr_message * m, tw_lp * lp) 
{
  switch( m->type )
    {
    case OLSR_PACKET_ARRIVAL_AT_region:
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

void olsr_region_event_handler_rc(olsr_region_state * s, tw_bf * bf, olsr_message * m, tw_lp * lp)
{
  switch( m->type )
    {
    case OLSR_PACKET_ARRIVAL_AT_region:
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

  for( i=0; i < OLSR_MAX_STATIONS_PER_region; i++)
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
  int i;
  lookahead = 1.0;
  tw_opt_add(app_opt);
  tw_init(&argc, &argv);
  
  g_tw_memory_nqueues = 16;
  
  offset_lpid = g_tw_mynode * nlp_per_pe;
  ttl_lps = tw_nnodes() * g_tw_npe * nlp_per_pe;
  
  g_tw_events_per_pe = (mult * nlp_per_pe * g_olsr_start_events)+ optimistic_memory;
  g_tw_lookahead = lookahead;
  
  tw_define_lps(nlp_per_pe, sizeof(olsr_message), 0);
  
  tw_lp_settype(0, &mylps[0]);
  
  for(i = 1; i < g_tw_nlp; i++)
    tw_lp_settype(i, &mylps[0]);
  
  tw_run();
  tw_end();

	return 0;
}
