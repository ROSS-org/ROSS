#include "wifi.h"

tw_peid wifi_map(tw_lpid gid);

void wifi_access_point_init(wifi_access_point_state * s, tw_lp * lp);
void wifi_access_point_event_handler(wifi_access_point_state * s, tw_bf * bf, wifi_message * m, tw_lp * lp);
void wifi_access_point_event_handler_rc(wifi_access_point_state * s, tw_bf * bf, wifi_message * m, tw_lp * lp);

void wifi_access_point_arrival(wifi_access_point_state * s, tw_bf * bf, wifi_message * m, tw_lp * lp);
void wifi_station_arrival(wifi_access_point_state * s, tw_bf * bf, wifi_message * m, tw_lp * lp);

void wifi_access_point_arrival_rc(wifi_access_point_state * s, tw_bf * bf, wifi_message * m, tw_lp * lp);
void wifi_station_arrival_rc(wifi_access_point_state * s, tw_bf * bf, wifi_message * m, tw_lp * lp);

void wifi_access_point_finish(wifi_access_point_state * s, tw_lp * lp);

tw_lptype mylps[] = 
{
    {	(init_f) wifi_access_point_init,
        (pre_run_f) NULL,
        (event_f) wifi_access_point_event_handler,
        (revent_f) wifi_access_point_event_handler_rc,
        (final_f) wifi_access_point_finish,
        (map_f) wifi_map,
    	sizeof(wifi_access_point_state)},
    {0},
};

tw_peid wifi_map(tw_lpid gid){
	return (tw_peid) gid / g_tw_nlp;
}

void wifi_access_point_init(wifi_access_point_state * s, tw_lp * lp)
{
  int i;
  tw_bf init_bf;
  wifi_message m;
  s->failed_packets = 0;
  s->total_packets = 0;

  // schedule out initial packet from access point
  for( i=0; i < WIFI_MAX_STATIONS_PER_ACCESS_POINT; i++)
    {
      m.type =  WIFI_PACKET_ARRIVAL_AT_ACCESS_POINT;
      m.station = i;
      wifi_access_point_event_handler(s, &init_bf, &m, lp);
    }
}


void wifi_access_point_arrival(wifi_access_point_state * s, tw_bf * bf, wifi_message * m, tw_lp * lp) 
{
  unsigned int rng_calls=0;
  tw_event *e=NULL;
  wifi_message *m_new=NULL;

  // packets coming from station to access point have less power and so lower snr
  s->total_packets++;

  s->stations[m->station].access_point_snr = tw_rand_normal_sd(lp->rng,1.0,5.0, &rng_calls);
  // New Function - to add prop loss, but not ready yet.
  //s->stations[m->station].access_point_snr = calcRxPower(txPowerDbm, distance, minDistance, lambda, systemLoss);
  s->stations[m->station].access_point_success_rate = 
    WiFi_80211b_DsssDqpskCck11_SuccessRate(s->stations[m->station].access_point_snr, num_of_bits);
  if( tw_rand_normal_sd(lp->rng,0.5,0.1, &rng_calls) < 
      s->stations[m->station].access_point_success_rate) 
    {
      bf->c1 = 1;
      s->failed_packets++; // count all failed arrivals coming to access point
    }
  
  // schedule event back to AP w/ exponential service time
  e = tw_event_new(lp->gid, tw_rand_exponential(lp->rng, 10.0), lp);
  m_new = (wifi_message *) tw_event_data(e);
  m_new->type = WIFI_PACKET_ARRIVAL_AT_STATION;
  m_new->station = m->station;
  tw_event_send(e);

}

void wifi_access_point_arrival_rc(wifi_access_point_state * s, tw_bf * bf, wifi_message * m, tw_lp * lp) 
{
  s->total_packets--;	
  // packets coming from access point have much more power and so better snr
  tw_rand_reverse_unif(lp->rng);
  tw_rand_reverse_unif(lp->rng);

  if( bf->c1 )
    {
		s->failed_packets--;
    }
  

}

void wifi_station_arrival(wifi_access_point_state * s, tw_bf * bf, wifi_message * m, tw_lp * lp) 
{
  unsigned int rng_calls=0;
  tw_event *e=NULL;
  wifi_message *m_new=NULL;

  // packets coming from access point have much more power and so better snr
  s->stations[m->station].total_packets++;
  s->stations[m->station].station_snr = tw_rand_normal_sd(lp->rng,4.0,8.0, &rng_calls);
  // New Function - to add prop loss, but not ready yet.
  //s->stations[m->station].station_snr = calcRxPower (txPowerDbm, distance, minDistance, lambda, systemLoss);
  s->stations[m->station].station_success_rate = 
    WiFi_80211b_DsssDqpskCck11_SuccessRate(s->stations[m->station].station_snr, num_of_bits);
  if( tw_rand_normal_sd(lp->rng,0.5,0.1, &rng_calls) < 
      s->stations[m->station].station_success_rate) 
    {
      bf->c1 = 1;
      s->stations[m->station].failed_packets++;
    }
  
  // schedule event back to AP w/ exponential service time
  e = tw_event_new(lp->gid, tw_rand_exponential(lp->rng, 10.0), lp);
  m_new = (wifi_message *) tw_event_data(e);
  m_new->type = WIFI_PACKET_ARRIVAL_AT_ACCESS_POINT;
  m_new->station = m->station;
  tw_event_send(e);
}

void wifi_station_arrival_rc(wifi_access_point_state * s, tw_bf * bf, wifi_message * m, tw_lp * lp) 
{
  tw_rand_reverse_unif(lp->rng);
  tw_rand_reverse_unif(lp->rng);

  if( bf->c1 )
    {
      s->stations[m->station].failed_packets--;
    }
}


void wifi_access_point_event_handler(wifi_access_point_state * s, tw_bf * bf, wifi_message * m, tw_lp * lp) 
{
  switch( m->type )
    {
    case WIFI_PACKET_ARRIVAL_AT_ACCESS_POINT:
      wifi_access_point_arrival(s, bf, m, lp);
      break;

    case WIFI_PACKET_ARRIVAL_AT_STATION:
      wifi_station_arrival(s, bf, m, lp);
      break;

    default:
      tw_error(TW_LOC, "Undefined type, corrupted message \n");
      break;
    }
}

void wifi_access_point_event_handler_rc(wifi_access_point_state * s, tw_bf * bf, wifi_message * m, tw_lp * lp)
{
  switch( m->type )
    {
    case WIFI_PACKET_ARRIVAL_AT_ACCESS_POINT:
      wifi_access_point_arrival_rc(s, bf, m, lp);
      break;

    case WIFI_PACKET_ARRIVAL_AT_STATION:
      wifi_station_arrival_rc(s, bf, m, lp);
      break;

    default:
      tw_error(TW_LOC, "Undefined type, corrupted message \n");
      break;
    }
}

void wifi_access_point_finish(wifi_access_point_state * s, tw_lp * lp)
{
  int i;
  unsigned long long station_failed_packets=0;
  unsigned long long station_total_packets=0;
  printf("LP %d had %d / %d failed Station to Access Point packets\n", lp->gid, s->failed_packets, s->total_packets);

  for( i=0; i < WIFI_MAX_STATIONS_PER_ACCESS_POINT; i++)
    {
      station_failed_packets += s->stations[i].failed_packets;
	  station_total_packets += s->stations[i].total_packets;
    }

  printf("LP %d had %llu / %llu failed Access Point to Station packets\n", lp->gid, station_failed_packets, station_total_packets);
}


const tw_optdef app_opt[] =
  {
    TWOPT_GROUP("802.11b Model"),
    TWOPT_UINT("nlp", nlp_per_pe, "number of LPs per processor"),
    TWOPT_STIME("mean", mean, "exponential distribution mean for timestamps"),
    TWOPT_STIME("mult", mult, "multiplier for event memory allocation"),
    TWOPT_STIME("lookahead", lookahead, "lookahead for events"),
    TWOPT_UINT("start-events", g_wifi_start_events, "number of initial messages per LP"),
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
  
  g_tw_events_per_pe = (mult * nlp_per_pe * g_wifi_start_events)+ optimistic_memory;
  g_tw_lookahead = lookahead;
  
  tw_define_lps(nlp_per_pe, sizeof(wifi_message), 0);
  
  tw_lp_settype(0, &mylps[0]);
  
  for(i = 1; i < g_tw_nlp; i++)
    tw_lp_settype(i, &mylps[0]);
  
  tw_run();
  tw_end();

	return 0;
}
