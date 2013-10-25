#ifndef INC_dragonfly_h
#define INC_dragonfly_h

#include <ross.h>

// dragonfly basic configuration parameters
#define GLOBAL_CHANNELS 4
#define NUM_ROUTER 8
#define NUM_TERMINALS 4

#define MESSAGE_SIZE 512.0
#define PACKET_SIZE 512.0
#define CHUNK_SIZE 32.0

// delay parameters
//#define TERMINAL_DELAY 1.0
//#define LOCAL_DELAY 1.0
//#define GLOBAL_DELAY 10.0

#define GLOBAL_BANDWIDTH 4.7 
#define LOCAL_BANDWIDTH 5.25
#define NODE_BANDWIDTH 5.25
#define CREDIT_SIZE 8
#define INJECTION_INTERVAL 3000
//16-05
#define RESCHEDULE_DELAY 1.0
// time to process a packet at destination terminal
#define MEAN_PROCESS 1.0
#define NUM_VC 1

#define N_COLLECT_POINTS 100

// virtual channel information
#define LOCAL_VC_SIZE 32768
#define TERMINAL_THRESHOLD 256
#define ROUTER_THRESHOLD 256
#define GLOBAL_VC_SIZE 65536
#define TERMINAL_VC_SIZE 16384

// radix of each router
#define RADIX (NUM_VC * NUM_ROUTER)+ (NUM_VC*GLOBAL_CHANNELS) + (NUM_VC * NUM_TERMINALS)

// debugging parameters
#define DEBUG 1
#define TRACK 601
#define PRINT_ROUTER_TABLE 1

#define NUM_ROWS NUM_ROUTER*NUM_TERMINALS
#define NUM_COLS (NUM_ROUTER*NUM_TERMINALS)+1
#define TERMINAL_WAITING_PACK_COUNT 1 << 16
#define ROUTER_WAITING_PACK_COUNT 1 << 18

extern FILE *dragonfly_event_log;

// arrival rate
static double MEAN_INTERVAL=200.0;

typedef enum event_t event_t;
typedef struct terminal_state terminal_state;
typedef struct terminal_message terminal_message;
typedef struct buf_space_message buf_space_message;
typedef struct router_state router_state;
typedef struct waiting_packet waiting_packet;
typedef struct process_state process_state;

struct terminal_state
{
   // Dragonfly specific parameters
   unsigned int router_id;
   unsigned int terminal_id;

   // Each terminal will have an input and output channel with the router
   int vc_occupancy[NUM_VC];
   int output_vc_state[NUM_VC];
   tw_stime terminal_available_time;
   tw_stime next_credit_available_time;
   
   //first element of linked list
   struct waiting_packet * waiting_list;

  // pointer to the linked list
   struct waiting_packet * head;   
   int wait_count;
   int packet_counter;
// Terminal generate, sends and arrival T_SEND, T_ARRIVAL, T_GENERATE
// Router-Router Intra-group sends and receives RR_LSEND, RR_LARRIVE
// Router-Router Inter-group sends and receives RR_GSEND, RR_GARRIVE
};

enum event_t
{
  RESCHEDULE,
  T_GENERATE,
  T_ARRIVE,
  T_SEND,

  R_SEND,
  R_ARRIVE,

  BUFFER,
  WAIT,
  FINISH,

  MPI_SEND,
  MPI_RECV
};

enum vc_status
{
   VC_IDLE,
   VC_ACTIVE,
   VC_ALLOC,
   VC_CREDIT
};

enum last_hop
{
   GLOBAL,
   LOCAL,
   TERMINAL
};

enum ROUTING_ALGO
{
   MINIMAL,
   NON_MINIMAL,
   ADAPTIVE
};

enum TRAFFIC_PATTERN
{
  UNIFORM_RANDOM=1,
  WORST_CASE,
  TRANSPOSE,
  NEAREST_NEIGHBOR,
  BISECTION,
  NEAREST_ROUTER
};

struct terminal_message
{
  tw_stime travel_start_time;
  unsigned long long packet_ID;
  event_t  type;
  
  unsigned int dest_terminal_id;
  unsigned int src_terminal_id;
  
  short my_N_hop;

  // Intermediate LP ID from which this message is coming
  unsigned int intm_lp_id;
  short old_vc;
  short saved_vc;

  short last_hop;

  // For buffer message
   short vc_index;
   int input_chan;
   int output_chan;
   
   tw_stime saved_available_time;
   tw_stime saved_credit_time;

   int intm_group_id;
   short wait_type;
   short wait_loc;
   short chunk_id;
//   short route;
};

struct router_state
{
   unsigned int router_id;
   unsigned int group_id;
  
   int global_channel[GLOBAL_CHANNELS]; 

   // 0--NUM_ROUTER-1 local router indices (router-router intragroup channels)
   // NUM_ROUTER -- NUM_ROUTER+GLOBAL_CHANNELS-1 global channel indices (router-router inter-group channels)
   // NUM_ROUTER+GLOBAL_CHANNELS -- RADIX-1 terminal indices (router-terminal channels)
   tw_stime next_output_available_time[RADIX];
//   tw_stime next_input_available_time[RADIX];

//   tw_stime next_available_time;
   tw_stime next_credit_available_time[RADIX];
//   tw_stime next_credit_available_time[RADIX];

//   unsigned int credit_occupancy[RADIX];   
   int vc_occupancy[RADIX];

//   unsigned int input_vc_state[RADIX];
   unsigned int output_vc_state[RADIX];

   //first element of linked list
   struct waiting_packet * waiting_list;

  // pointer to the linked list
   struct waiting_packet * head;
   int wait_count;
};

struct process_state
{
   int message_counter;
   tw_stime available_time;
  
   unsigned int router_id; 
   unsigned int group_id;
//   For matrix transpose traffic
   int row, col;
};

struct waiting_packet
{
   terminal_message * packet;
   struct waiting_packet * next;
   int chan;
};

static int       nlp_terminal_per_pe;
static int       nlp_router_per_pe;
static int 	 nlp_mpi_procs_per_pe;
static int opt_mem = 10000;
static int mem_factor = 32;
static int max_packets = 0;

static int ROUTING= MINIMAL;
static int traffic= NEAREST_NEIGHBOR;
int minimal_count, nonmin_count;

int adaptive_threshold;
int head_delay;
int num_packets;
int num_chunks;
int packet_offset;

tw_stime         average_travel_time = 0;
tw_stime         total_time = 0;
tw_stime         max_latency = 0;

int range_start;
int num_vc;
int terminal_rem=0, router_rem=0;
int num_terminal=0, num_router=0;
unsigned long num_groups = NUM_ROUTER*GLOBAL_CHANNELS+1;
int total_routers, total_terminals, total_mpi_procs;
unsigned long long max_packet;

static unsigned long long       total_hops = 0;
static unsigned long long       N_finished = 0;
static unsigned long long       N_finished_storage[N_COLLECT_POINTS];
static unsigned long long       N_generated_storage[N_COLLECT_POINTS];

void dragonfly_mapping(void);
tw_lp * dragonfly_mapping_to_lp(tw_lpid lpid);
#endif
