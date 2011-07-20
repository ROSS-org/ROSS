/*
 * BG/P model globals
 * Ning Liu
 */

#include "bgp.h"

/////////////////////

int nlp_DDN;
int nlp_Controller;
int nlp_FS;
int nlp_ION;
int nlp_CN;

int rootCN;

int NumDDN = 2;
int NumControllerPerDDN = 1;
int NumFSPerController = 1;

int N_ION_per_FS = 1;
int N_CN_per_ION = 64;

int N_CN_per_DDN;
int N_ION_per_DDN;
int N_FS_per_DDN;
int N_controller_per_DDN;
int N_DDN_per_PE;

// default packet size
double payload_size = 256;
double PVFS_payload_size = 256*1024;
double ACK_message_size = 20;

// IO request message prep time 
double CN_message_wrap_time = 300000;
double PVFS_handshake_time = 3000;

double CN_packet_service_time = 20;
double ION_packet_service_time = 300;
double FS_packet_service_time = 800;
double CON_packet_service_time = 300;
double DDN_packet_service_time = 300;

double link_transmission_time = 500;

double collective_block_size = 10*1024*1024;

/////////////////////////////////////
// links bandwidth
// all measured in "byte/ns"
double CN_tree_bw = 0.7;
double CN_ION_bw = 0.7; 
double ION_FS_bw= 0.26; 
double FS_CON_bw= 0.56; 
double CON_DDN_bw= 0.6; 

int msg_collective_counter = 0;

int nlp_per_pe;
/////////////////////

// Test RC code in serial mode
int g_test_rc = 0;
int N_nodes = 1;

////////////


