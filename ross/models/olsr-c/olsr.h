#ifndef INC_OLSR_H
#define INC_OLSR_H

#include <ross.h>

#include "prop-loss.h"
#include "coding-error.h"
#include "mobility.h"


// These define the regions space -- should be square
#define NUM_REGIONS_X 64
#define NUM_REGIONS_Y 64
#define NUM_VP_X 64 
#define NUM_VP_Y 64

#define OLSR_MAX_STATIONS_PER_REGION 256
#define OLSR_MAX_MPRS_PER_REGION 4

#define MAX_X_DIST 1
#define MAX_Y_DIST 1
#define MAX_Z_DIST 1
#define STD_DEV 0.1


#define WIFIB_BW 20

#define LAMBDA 0.125 //Insert reasoning behind this here, it is in my notebook.

/* default to 11Mbits/sec data rate */
/* 1 unit of time is 1 usecond      */ 
/* max total size of packet with header is 2346 */

#define DATA_PACKET_SIZE 2346
#define HELLO_PACKET_SIZE 34

#define DATA_PACKET_TIME (((DATA_PACKET_SIZE * 8) /  (11.0 * 1024.0 * 1024.0)) * 1000000.0)
#define HELLO_PACKET_TIME (((HELLO_PACKET_SIZE * 8) /  (11.0 * 1024.0 * 1024.0)) * 1000000.0)
#define MEAN_TIME_BETWEEN_DATA_PACKETS 100000.0 // 100K useconds for each station
#define MEAN_TIME_BETWEEN_MOVES 10000000.0 // 10 seconds between each stations region moves

#define REGION_SIZE 100 // 100 meters by default -- each region is 100x100

#define OLSR_MPR_POWER 80     // 30 dbm
#define OLSR_STATION_POWER 80 // 10 dbm
#define OLSR_MPR_PACKET_SERVICE_TIME 100 // 100 useconds
#define OLSR_MAX_HOPS 16

#define WIFI_CW_MIN 16
#define WIFI_CW_MAX 1024
#define WIFI_SLOT_TIME 20 // 20 useconds

typedef struct olsr_region_state olsr_region_state;
typedef struct olsr_mpr_station_state olsr_mpr_station_state;
typedef struct olsr_message olsr_message;
typedef struct olsr_mpr olsr_mpr;

typedef enum {
        OLSR_STATION_TO_MPR,
        OLSR_ARRIVAL_TO_MPR,
	OLSR_DEPARTURE_FROM_MPR,
	OLSR_CHANGE_MPR,
	OLSR_CHANGE_REGION
} olsr_message_type;

typedef enum {
	OLSR_HELLO,
	OLSR_DATA_PACKET
} olsr_packet_type;

typedef enum {
  OLSR_NORTH, 
  OLSR_SOUTH, 
  OLSR_EAST, 
  OLSR_WEST} olsr_direction_type;

typedef enum
  {
    STATION_SLOT_FREE,
    STATION_SLOT_BUSY
  } station_status_type;

struct olsr_mpr_station_state 
{
  unsigned int failed_packets;
  unsigned int sent_packets;
  unsigned int waiting_packets;
  unsigned int packets_delivered;
  double snr;
  double success_rate;
  //  double region_success_rate; What is this for ??
  double tx_power;
  // double data_packet_time; What is this for ?
  tw_grid_pt location;
  unsigned int my_mpr;
  tw_stime next_move_time;
};

struct olsr_region_state 
{
  olsr_mpr_station_state mpr[OLSR_MAX_MPRS_PER_REGION];
  olsr_mpr_station_state station[OLSR_MAX_STATIONS_PER_REGION];
  station_status_type station_status[OLSR_MAX_STATIONS_PER_REGION];
  unsigned int num_mprs;
  unsigned int num_stations;
  unsigned int slot_busy;
  unsigned int station_drops;
  tw_integer_grid_pt region_location;
  
};

struct olsr_message 
{
  olsr_message_type type;
  //  unsigned int from_station;
  //  unsigned int to_station;
  unsigned int station;
  unsigned int mpr;      
  unsigned int max_hop_count;
  unsigned int hop_count;
  unsigned int contention_window;
  tw_stime     next_move_time;
  double       success_rate;
};

double success_rate;
tw_stime lookahead = 1.0;
static unsigned int offset_lpid = 0;
static tw_stime mult = 3.0;
static unsigned int ttl_lps = 0;
static unsigned int nlp_per_pe = 8;
static int optimistic_memory = 100;

static uint64_t dropped_packets;


static unsigned int g_regions_per_vp_x=NUM_REGIONS_X/NUM_VP_X;
static unsigned int g_regions_per_vp_y= NUM_REGIONS_Y/NUM_VP_Y;
static unsigned int g_regions_per_vp=(NUM_REGIONS_X/NUM_VP_X)*(NUM_REGIONS_Y/NUM_VP_Y);
static unsigned int g_vp_per_proc=0;

static char run_id[1024] = "OLSR Model";

#endif
