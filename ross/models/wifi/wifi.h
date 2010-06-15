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

#define WIFI_MAX_STATIONS_PER_ACCESS_POINT 8

struct wifi_access_point_state_t;
typedef wifi_access_point_state_t wifi_access_point_state;
struct wifi_station_state_t;
typedef wifi_station_state_t wifi_station_state;
struct wifi_message_t;
typedef wifi_message_t wifi_message;

enum wifi_message_type_e
{
    WIFI_PACKET_ARRIVAL_AT_ACCESS_POINT,
    WIFI_PACKET_ARRIVAL_AT_STATION
};

typedef enum wifi_message_type_e wifi_message_type;

struct wifi_station_state_t
{
  unsigned int failed_packets;
  double station_snr;
  double access_point_snr;
  double station_success_rate;
  double access_point_success_rate;
};

struct wifi_access_point_state_t
{
  unsigned int failed_packets;
  wifi_station_state stations[WIFI_MAX_STATIONS_PER_ACCESS_POINT];
};

struct wifi_message_t
{
  wifi_message_type type;
  unsigned int station;
};

double success_rate;
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
