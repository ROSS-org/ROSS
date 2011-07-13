/*                                                                              
 *  Blue Gene/P model                                                           
 *  Header file                                                                 
 *  by Ning Liu                                                                 
 */
#ifndef BGP_TYPES_H
#define BGP_TYPES_H

#include "bgp.h"

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

// Debug                                                                        
#define TRACK -1
#define N_COLLECT_POINTS 20

// Test RC code in serial mode                                                  
extern int g_test_rc;

// Total number of nodes in torus, calculate in main                            
extern int N_nodes;

// build your own torus                                                         
// static int       N_dims = 4;     


/* type define */
typedef struct compute_node_state CN_state;
typedef struct io_node_state ION_state;

typedef struct file_server_state FS_state;
//typedef struct disk_state Disk_state;

typedef struct controller_state CON_state;
typedef struct ddn_state DDN_state;

typedef struct nodes_message MsgData;
typedef enum   event_t EventType;
typedef enum   block_t BlockType;
typedef enum   message_t MsgType;

enum event_t
{
  GENERATE,
  ARRIVAL,
  SEND,
  PROCESS
};

enum message_t
{
  DATA,
  ACK,
  CONT
};

enum block_t
{
  ComputeNode,
  IONode,
  FileServer,
  Disk
};

struct compute_node_state
{
  /////////////////////////////
  int upID;
  int * dwonID;
  ///////////////////////////////

  int CN_ID_in_tree;
  int FS_group_id;
  int ION_group_id;

  int tree_next_hop_id;
  int tree_previous_hop_id;
  int * torus_neighbour_minus_id;
  int * torus_neighbour_plus_id;

  ////////////////////////////////
  unsigned long long packet_counter;

  tw_stime next_available_time;
  tw_stime nextLinkAvailableTime;

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

struct io_node_state
{  
  int file_server_id;
  int root_CN_id;

  tw_stime next_available_time;
  tw_stime nextLinkAvailableTime;

  int collective_round_counter;
  
};

struct file_server_state
{
  int * previous_ION_id;
  int controller_id;
  
  tw_stime next_available_time;
  tw_stime nextLinkAvailableTime;
};

struct controller_state
{
  int * previous_FS_id;
  int ddn_id;
  tw_stime next_available_time;
  tw_stime nextLinkAvailableTime;
};

struct ddn_state
{
  int * previous_CON_id;
  int file_server_id;
  tw_stime next_available_time;
  tw_stime nextLinkAvailableTime;
};

struct nodes_message
{
  ///////////////////////////////
  int collective_msg_tag;
  int msg_size;
  BlockType MsgSrc;
  MsgType message_type;

  ///////////////////////////////
  //double     message_size;
  //tw_stime   travel_start_time;

  tw_stime transmission_time;
  tw_stime travel_start_time;
  tw_stime saved_available_time;
  tw_stime saved_link_available_time[2][N_dims];
  unsigned long long packet_ID;
  EventType  type;
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


///////////////

extern double         average_travel_time;
extern double         total_time;
extern double         max_latency;

extern unsigned long long       N_finished;
extern unsigned long long       N_dropped;                                

extern unsigned long long       N_finished_storage[N_COLLECT_POINTS];
extern unsigned long long       N_dropped_storage[N_COLLECT_POINTS];
extern unsigned long long       N_generated_storage[N_COLLECT_POINTS];

extern unsigned long long       total_queue_length;
extern unsigned long long       queueing_times_sum;
extern unsigned long long       total_hops;

extern int       half_length[N_dims];

extern int       nlp_per_pe;
extern int       opt_mem;
extern int       buffer_size;

extern tw_stime g_tw_last_event_ts;
extern tw_lpid  g_tw_last_event_lpid;
//enum EventT g_tw_last_event_type=0;
extern FILE *g_event_trace_file;
extern int g_enable_event_trace;

#endif
