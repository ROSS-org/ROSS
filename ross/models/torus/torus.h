#ifndef INC_torus_h
#define INC_torus_h

#include <ross.h>
#include <assert.h>

/* unit time of delay is nano second
 assume 374MB/s bandwidth for BG/P, it takes 64 ns to transfer 32Byte
 1.8 GB/s for BG/Q (2.0 GB/s is available but 1.8 GB/s is available
 to the user */
#define REPORT_BANDWIDTH 0
#define MEAN_PROCESS 1.0
#define N_dims 5
#define N_dims_sim 5

// Total available tokens on a VC = VC buffer size / token size
#define PACKET_SIZE 512
#define NUM_VC 1
/*  static int       dim_length[] = {8,4,4,4,4,4,2}; // 7-D torus */
    static int dim_length_sim[] = {4,4,4,4,2};//512 node case
    static int dim_length[] = {4,4,4,4,2};
//    static int dim_length[] = {4, 4, 2, 2, 2, 2, 2}; /* same as original dimension length */
/*  static int dim_length[] = {16, 8, 8, 8, 2};
    static int dim_length[] = {10, 10, 10, 8, 4, 4, 2}; // 256K 7D
    static int dim_length[] = {20, 20, 16, 10, 4}; // 256K 5D
    static int dim_length[] = {16, 16, 16, 10, 4, 4, 2}; //1.3 million 7D 
    static int dim_length[] = {10, 8, 8, 8, 4, 4, 4, 2, 2}; // 1.3 million 9D      
    static int dim_length[] = {10, 8, 8, 8, 4, 4, 4, 2, 2}; // 1.3 million 9D      
    static int dim_length_sim[] = {32, 32, 32, 20, 2}; //1.3 million 5D */

/* For reporting statistics, one can configure the number of sample points during
 * which the stats will be reported in the simulation */
#define N_COLLECT_POINTS 100
#define TRACK_LP -1
#define DEBUG 1

/* For packets waiting to be injected in the network when the buffers are already full. 
 * We probably need to come up with a better strategy here like slow down the injection
 * rate when the buffers are already full instead of making the packets wait in the queue. */
#define WAITING_PACK_COUNT 1 << 15

typedef enum nodes_event_t nodes_event_t;
typedef struct nodes_state nodes_state;
typedef struct mpi_process mpi_process;
typedef struct nodes_message nodes_message;
typedef struct waiting_packet waiting_packet;

// Total number of nodes in torus, calculate in main
static int N_nodes = 1;
static int N_mpi_procs = 1;
//static int N_COLLECT_POINTS=200;
enum nodes_event_t
{
  GENERATE = 1,
  ARRIVAL, 
  SEND,
  PROCESS,
  CREDIT,
  WAIT,
  MPI_SEND,
  MPI_RECV
};

enum traffic
{
  UNIFORM_RANDOM=1,
  NEAREST_NEIGHBOR,
  DIAGONAL
};

struct mpi_process
{
 unsigned long long message_counter;
 tw_stime available_time;

 /*For matrix transpose traffic, we have a row and col value for each MPI process so that the message can be sent to the corresponding transpose of 
 the MPI process */
 int row, col;
};

struct nodes_state
{
  unsigned long long packet_counter;            
  /* Time when the VC will be available for sending packets on this node */
  tw_stime next_link_available_time[2*N_dims][NUM_VC]; 
  /* Time when the next VC will be available for sending credits on this node*/
  tw_stime next_credit_available_time[2*N_dims][NUM_VC];
  /* Buffer occupancy of the current VC */
  int buffer[2*N_dims][NUM_VC]; 
  /* torus dimension coordinates of this node */
  int dim_position[N_dims];
  /* torus dimension coordinates of the simulated torus dimension by this node (For TOPC paper)*/
  int dim_position_sim[N_dims_sim];
  /* torus neighbor coordinates for this node */ 
  int neighbour_minus_lpID[N_dims];
  /* torus plus neighbor coordinates for this node */
  int neighbour_plus_lpID[N_dims];
  
  // For simulation purposes: Making the same nearest neighbor traffic
  // across all torus dimensions
  /* neighbors of the simulated torus dimension for this node */
  int neighbour_minus_lpID_sim[N_dims_sim];
  int neighbour_plus_lpID_sim[N_dims_sim];   

  int source_dim;
  int direction;

  //first element of linked list
  struct waiting_packet * waiting_list;
  struct waiting_packet * head;
  long wait_count;
};

struct nodes_message
{
  /* time when the flit/packet starts travelling */
  tw_stime travel_start_time;
  /* saved time for reverse computation */
  tw_stime saved_available_time;

  /* packet ID of the flit */
  unsigned long long packet_ID;
  /* event time: mpi_send, mpi_recv, packet_generate etc. */
  nodes_event_t	 type;

  /* originating torus node dimension and direction */
  int source_dim;
  int source_direction;
  
  /* originating torus node dimension and direction (for reverse computation) */
  int saved_src_dim;
  int saved_src_dir;

  /* For waiting messages/packets/flits that don't get a slot in the buffer*/
  int wait_dir;
  int wait_dim;

  /* destination torus coordinates for this packet/message/flit */
  int dest[N_dims];

  /* destination LP ID for this packet/message/flit */
  tw_lpid dest_lp;
  /* sender LP ID for this packet/message/flit */
  tw_lpid sender_lp;

  /* number of hops travelled by this message */
  int my_N_hop;
  
  /* next stop of this message/packet/flit */
  tw_lpid next_stop;
  /* size of the message/packet/flit in bytes */
  int packet_size;
  /* flit/chunk ID of the packet */
  short chunk_id;
  /* for packets waiting to be injected into the network */
  int wait_loc;
  int wait_type;
};

/* Linked list for storing waiting packets in the queue */
struct waiting_packet
{
   int dim;
   int dir;
   nodes_message * packet;
   struct waiting_packet * next;
};

/* overall simulation statistics, average travel time, total time and maximum packet
 * latency per PE */
tw_stime         average_travel_time = 0;
tw_stime         total_time = 0;
tw_stime         max_latency = 0;

/* number of finished packets and messages per PE */
static unsigned long long       N_finished_packets = 0;
static unsigned long long N_finished_msgs = 0;

/* number of finished packets, generated packets, size of buffer queues
 * and average number of hops travelled by the packets at different 
 * points during the simulation per PE */
static unsigned long long       N_finished_storage[N_COLLECT_POINTS];
static unsigned long long       N_generated_storage[N_COLLECT_POINTS];
static unsigned long long 	N_queue_depth[N_COLLECT_POINTS];
static unsigned long long	N_num_hops[N_COLLECT_POINTS];
static unsigned long long       total_hops = 0;

/* for calculating torus dimensions of real and simulated torus coordinates of
 * a node*/
static int       half_length[N_dims];
static int	 half_length_sim[N_dims_sim];

/* ROSS simulation statistics */
static int	 nlp_nodes_per_pe;
static int 	 nlp_mpi_procs_per_pe;
static int total_lps;

// run time arguments
static int	 opt_mem = 3000;
static long mpi_message_size = 32;
static int num_mpi_msgs = 50;
static int mem_factor = 16;
static double MEAN_INTERVAL=200.0;
static int TRAFFIC = UNIFORM_RANDOM;
static int injection_limit = 10;
static double injection_interval = 20000;
static int vc_size = 16384;
static double link_bandwidth = 2.0;
static char traffic_str[512] = "uniform";

/* number of packets in a message and number of chunks/flits in a packet */
int num_packets;
int num_chunks;
int packet_offset = 0;
const int chunk_size = 32;
int num_buf_slots;

/* for ROSS mapping purposes */
int node_rem = 0;

int num_rows, num_cols;
/* for calculating torus dimensions */
int factor[N_dims];
int factor_sim[N_dims_sim];

/* calculating delays using the link bandwidth */
float head_delay=0.0;
float credit_delay = 0.0;

// debug
int credit_sent = 0;
int packet_sent = 0;
#endif
