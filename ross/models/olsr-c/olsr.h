#ifndef INC_OLSR_H
#define INC_OLSR_H

#include <ross.h>

#include "prop-loss.h"
#include "coding-error.h"
#include "mobility.h"


// These define the regions space -- should be square
#define NUM_REGIONS_X 32
#define NUM_REGIONS_Y 32
#define NUM_VP_X 8
#define NUM_VP_Y 8

#define OLSR_MAX_STATIONS_PER_REGION 8
#define MAX_X_DIST 1
#define MAX_Y_DIST 1
#define MAX_Z_DIST 1
#define STD_DEV 0.1
#define OLSR_STATIONS_PER_MPR 8

#define WIFIB_BW 20

#define OLSR_MPRS_PER_REGIONLP 4

#define LAMBDA 0.125 //Insert reasoning behind this here, it is in my notebook.

/* default to 11Mbits/sec data rate */
/* 1 unit of time is 1 usecond      */ 
/* max total size of packet with header is 2346 */

#define DATA_PACKET_TIME (((2346 * 8) /  (11.0 * 1024.0 * 1024.0)) * 1000000.0)

typedef struct olsr_region_state olsr_region_state;
typedef struct olsr_station_state olsr_station_state;
typedef struct olsr_message olsr_message;
typedef struct olsr_mpr olsr_mpr;

typedef enum {
        OLSR_STATION_TO_MPR,
	OLSR_MPR_TO_MPR,
	OLSR_CHANGE_MPR,
	OLSR_CHANGE_CELL
} olsr_message_type;

typedef enum {
	OLSR_HELLO,
	OLSR_DATA_PACKET
} olsr_packet_type;

struct olsr_mpr_state {
  olsr_mpr mpr[OLSR_MPRS_PER_REGIONLP];
  tw_grid_pt location;
}

struct olsr_station_state {
  unsigned int failed_packets;
  double region_snr;
  double station_success_rate;
  double region_success_rate;
  double tx_power;
  double data_packet_time;
  tw_grid_pt location;
};

struct olsr_mpr {
  unsigned int failed_packets;  
  tw_grid_pt location;
  olsr_station_state stations[OLSR_STATIONS_PER_MPR];
};

struct olsr_message {
  olsr_message_type type;
  unsigned int from_station;
  unsigned int to_station;
  unsigned int station;
  unsigned int mpr;      
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


static unsigned int g_regions_per_vp_x=NUM_REGIONS_X/NUM_VP_X;
static unsigned int g_regions_per_vp_y= NUM_REGIONS_Y/NUM_VP_Y;
static unsigned int g_regions_per_vp=(NUM_REGIONS_X/NUM_VP_X)*(NUM_REGIONS_Y/NUM_VP_Y);
static unsigned int g_vp_per_proc=0;

// rate for timestamp exponential distribution
static tw_stime mean = 1.0;

static char run_id[1024] = "undefined";

#endif
