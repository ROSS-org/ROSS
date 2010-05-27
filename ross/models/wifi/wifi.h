#ifndef INC_WIFI_H
#define INC_WIFI_H

#include <ross.h>

#ifdef ENABLE_GSL
	#include "gsl-prob-wifi.h"
#elif defined(ENABLE_ESSL)
	#include "essl-prop-wifi.h"
#else
	#include "none-prob-wifi.h"
#endif

FWD(struct,  wifi_ap_state);
FWD(struct,   wifi_message);

DEF(struct, wifi_ap_state){
	unsigned int failed_packets;
};

DEF(struct, wifi_message){
	
};

double success_rate;
double random_test;
tw_stime lookahead = 1.0;
static unsigned int offset_lpid = 0;
static tw_stime mult = 3.0;
static unsigned int ttl_lps = 0;
static unsigned int nlp_per_pe = 8;
static int g_wifi_start_events = 1;
static int optimistic_memory = 100;
static int num_of_antennas = 100;
static int num_of_bits = 1500;

// rate for timestamp exponential distribution
static tw_stime mean = 1.0;

static char run_id[1024] = "undefined";

#endif
