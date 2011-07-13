/*
 *
 * Ning Liu
 */

#include "bgp.h"

/////////////////////
int TreeMatrix[32];

//
int nlp_DDN;
int nlp_Controller;
int nlp_FS;
int nlp_ION;
int nlp_CN;

//
int rootCN;

int NumDDN = 2;
int NumControllerPerDDN = 2;
int NumFSPerController = 1;

int N_CN_per_DDN;
int N_ION_per_DDN;
int N_FS_per_DDN;
int N_controller_per_DDN;
int N_DDN_per_PE;

int N_ION_per_FS = 2;
int N_CN_per_ION = 64;

double CN_packet_service_time = 300;
double ION_packet_service_time = 300;
double FS_packet_service_time = 800;
double Disk_packet_service_time = 300;

double link_transmission_time = 500;

int msg_collective_counter = 0;

/////////////////////

double total_time = 0;
double max_latency = 0;

unsigned long long       N_finished = 0;
unsigned long long       N_dropped = 0;            

unsigned long long       N_finished_storage[N_COLLECT_POINTS];
unsigned long long       N_dropped_storage[N_COLLECT_POINTS];
unsigned long long       N_generated_storage[N_COLLECT_POINTS];

unsigned long long       total_queue_length = 0;
unsigned long long       queueing_times_sum = 0;
unsigned long long       total_hops = 0;

int       half_length[N_dims];

int       nlp_per_pe;
int       opt_mem = 3000;
//static int       buffer_size = 10;                    

tw_stime g_tw_last_event_ts = -1.0;
tw_lpid  g_tw_last_event_lpid = 0;
//enum EventT g_tw_last_event_type=0;                             
     
FILE *g_event_trace_file=NULL;
int g_enable_event_trace=1;

// Test RC code in serial mode
int g_test_rc = 0;
int N_nodes = 1;

////////////
int Number_Compute_Nodes;
int Number_IO_Nodes;
int Number_File_Servers;
int Number_Disks;

