#ifndef INC_OLSR_H
#define INC_OLSR_H

#include <ross.h>

#include "prop-loss.h"
#include "coding-error.h"

#define OLSR_MAX_STATIONS_PER_region 8

typedef struct olsr_region_state olsr_region_state;
typedef struct olsr_station_state olsr_station_state;
typedef struct olsr_message olsr_message;

enum olsr_message_type_e
{
	OLSR_HELLO_PACKET,
    OLSR_DATA_PACKET
};

typedef enum olsr_message_type_e olsr_message_type;

struct olsr_station_state
{
  unsigned int failed_packets;
 /* double signal;
  double bandwidth;
  double noiseFigure;
  double noiseInterference;
  double station_snr;*/
  double region_snr;
  double station_success_rate;
  double region_success_rate;
};

struct olsr_region_state
{
  unsigned int failed_packets;
  olsr_station_state stations[OLSR_MAX_STATIONS_PER_region];
};

struct olsr_message
{
  olsr_message_type type;
  unsigned int station;
};

double success_rate;
tw_stime lookahead = 1.0;
static unsigned int offset_lpid = 0;
static tw_stime mult = 3.0;
static unsigned int ttl_lps = 0;
static unsigned int nlp_per_pe = 8;
static int g_olsr_start_events = 1;
static int optimistic_memory = 100;
static int num_of_bits = 1500;

// rate for timestamp exponential distribution
static tw_stime mean = 1.0;

static char run_id[1024] = "undefined";

#endif
