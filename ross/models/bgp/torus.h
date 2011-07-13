#ifndef INC_torus_h
#define INC_torus_h

#include <ross.h>

// unit time can be nano second
// assume 175MB/s bandwidth, it takes 5.7 ns to transfer 1Byte
// assume average packet size 1kB, exponential dirstribution
// assume average processing time 1 us = 1000 ns, exponential dirstribution 
//#define HOP 8
//#define SETUP 4000
#define PACKET_SIZE 1750.0
#define MEAN_PROCESS 200.0
#define LINK_DELAY 100.0
//#define MEAN_INTERVAL 100000.0
static double MEAN_INTERVAL;

#define N_PACKETS_PER_NODE 8

//#define ARRIVAL_RATE 0.05
static double ARRIVAL_RATE = 0.0000001; 

//static int       dim_length[] = {8,8,8,8,8,8};
//static int       dim_length[] = {32,32};
//static int       dim_length[] = {64,64,64,64};
//static int       dim_length[] = {2,2,2,2,2,2,2,2,2,2};
//static int       dim_length[] = {8,8,8,8,8,8,8,8};
static int       dim_length[] = {4,4,4};
#define N_dims 3

typedef enum nodes_event_t nodes_event_t;
typedef struct nodes_state nodes_state;
typedef struct nodes_message nodes_message;

// important parameters for the torus simulation
// build your own torus
// static int       N_dims = 4;

// Debug
#define TRACK -1
#define N_COLLECT_POINTS 20

// Test RC code in serial mode
int g_test_rc = 0;

// Total number of nodes in torus, calculate in main
static int       N_nodes = 1;

enum nodes_event_t
{
  GENERATE,
  ARRIVAL, 
  SEND,
  PROCESS
};

struct nodes_state
{
  unsigned long long packet_counter;            
  tw_stime next_available_time;                 
  tw_stime next_link_available_time[2][N_dims]; 
  int dim_position[N_dims];
  int neighbour_minus_lpID[N_dims];
  int neighbour_plus_lpID[N_dims];
  int node_queue_length[2][N_dims];
  int N_wait_to_be_processed;
  int source_dim;
  int direction;
  int generate_counter;
};

struct nodes_message
{
  tw_stime transmission_time;
  tw_stime travel_start_time;
  tw_stime saved_available_time;
  tw_stime saved_link_available_time[2][N_dims];
  unsigned long long packet_ID;
  nodes_event_t	 type;
  int saved_source_dim;
  int saved_direction;
  int dest[N_dims];
  tw_lpid dest_lp;
  tw_lpid src_lp;
  int my_N_queue;
  int my_N_hop;
  int queueing_times;
  int source_dim;
  int source_direction;
  int next_stop;
};

tw_stime         average_travel_time = 0;
tw_stime         total_time = 0;
tw_stime         max_latency = 0;

static unsigned long long       N_finished = 0;
//static unsigned long long       N_dropped = 0;

static unsigned long long       N_finished_storage[N_COLLECT_POINTS];
static unsigned long long       N_dropped_storage[N_COLLECT_POINTS];
static unsigned long long       N_generated_storage[N_COLLECT_POINTS];

static unsigned long long       total_queue_length = 0;
static unsigned long long       queueing_times_sum = 0;
static unsigned long long       total_hops = 0;


static int       half_length[N_dims];

static int	 nlp_per_pe;
static int	 opt_mem = 3000;
//static int       buffer_size = 10;

tw_stime g_tw_last_event_ts = -1.0;
tw_lpid  g_tw_last_event_lpid = 0;
enum nodes_event_t g_tw_last_event_type=0;
FILE *g_event_trace_file=NULL;
int g_enable_event_trace=1;
#endif
