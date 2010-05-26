#include "wifi.h"

/*#ifdef ENABLE_GSL
#include "gsl-prob-wifi.h"
#elif defined(ENABLE_ESSL)
#include "essl-prop-wifi.h"
#else
#include "none-prob-wifi.h"
#endif*/

#include "none-prob-wifi.h"

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

void wifi_ap_init(wifi_ap_state * s, tw_lp * lp){
  	tw_bf init_bf;
  	wifi_ap_event_handler(s, &init_bf, NULL, lp);
}

void wifi_ap_event_handler(wifi_ap_state * s, tw_bf * bf, wifi_message * m, tw_lp * lp) {
	double snr;
	snr = tw_rand_normal_sd(lp->rng,1,5);
	success_rate = WiFi_80211b_DsssDqpskCck11_SuccessRate(snr,num_of_bits);
	random_test  = tw_rand_normal_sd(lp->rng,0.5,0.1);
	if(random_test < success_rate) {
		failed_packets++;
	}
}

void wifi_ap_event_handler_rc(wifi_ap_state * s, tw_bf * bf, wifi_message * m, tw_lp * lp){
    tw_rand_reverse_unif(lp->rng);
    tw_rand_reverse_unif(lp->rng);
	if(random_test < success_rate) {
		failed_packets--;
	}
}

void wifi_ap_finish(wifi_ap_state * s, tw_lp * lp){
	printf("%d\n", failed_packets);
}


const tw_optdef app_opt[] ={
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
main(int argc, char **argv, char **env){
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
        tw_lp_settype(i, &mylps[1]);
    
	tw_run();
	tw_end();

	return 0;
}
