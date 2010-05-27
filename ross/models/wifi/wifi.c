#include "wifi.h"

tw_peid wifi_map(tw_lpid gid);

void wifi_ap_init(wifi_ap_state * s, tw_lp * lp);
void wifi_ap_event_handler(wifi_ap_state * s, tw_bf * bf, wifi_message * m, tw_lp * lp);
void wifi_ap_event_handler_rc(wifi_ap_state * s, tw_bf * bf, wifi_message * m, tw_lp * lp);
void wifi_ap_finish(wifi_ap_state * s, tw_lp * lp);

tw_lptype mylps[] = 
{
    {	(init_f) wifi_ap_init,
        (event_f) wifi_ap_event_handler,
        (revent_f) wifi_ap_event_handler_rc,
        (final_f) wifi_ap_finish,
        (map_f) wifi_map,
    	sizeof(wifi_ap_state)},
    {0},
};

tw_peid wifi_map(tw_lpid gid){
	return (tw_peid) gid / g_tw_nlp;
}

void wifi_ap_init(wifi_ap_state * s, tw_lp * lp)
{
  tw_bf init_bf;
  s->failed_packets = 0;
  wifi_ap_event_handler(s, &init_bf, NULL, lp);
}

void wifi_ap_event_handler(wifi_ap_state * s, tw_bf * bf, wifi_message * m, tw_lp * lp) 
{
  double snr;
  unsigned int rng_calls=0;
  snr = tw_rand_normal_sd(lp->rng,1.0,5.0, &rng_calls);
  success_rate = WiFi_80211b_DsssDqpskCck11_SuccessRate(snr,num_of_bits);
  random_test  = tw_rand_normal_sd(lp->rng,0.5,0.1, &rng_calls);
  if(random_test < success_rate) 
    {
      bf->c1 = 1;
      s->failed_packets++;
    }

  // schedule event back to self
  tw_event_send(tw_event_new(lp->gid, tw_rand_exponential(lp->rng, 10.0), lp));
}

void wifi_ap_event_handler_rc(wifi_ap_state * s, tw_bf * bf, wifi_message * m, tw_lp * lp)
{
  // because normal was called twice, we are guarenteed that the rng was call twice
  tw_rand_reverse_unif(lp->rng);
  tw_rand_reverse_unif(lp->rng);
  if( bf->c1 ) 
    {
    s->failed_packets--;
  }
}

void wifi_ap_finish(wifi_ap_state * s, tw_lp * lp)
{
  printf("LP %llu had %d failed packets\n", lp->gid, s->failed_packets);
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
