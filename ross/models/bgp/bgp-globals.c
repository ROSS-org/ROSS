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

int NumDDN = 16;
int NumControllerPerDDN = 2;
int NumFSPerController = 4;

int N_ION_per_FS = 5;
int N_CN_per_ION = 64;

int N_CN_per_DDN;
int N_ION_per_DDN;
int N_FS_per_DDN;
int N_controller_per_DDN;
int N_DDN_per_PE;

// default packet size
double payload_size = 128;
double ACK_message_size = 20;

double CN_message_wrap_time = 10;

double CN_packet_service_time = 300;
double ION_packet_service_time = 300;
double FS_packet_service_time = 800;
double CON_packet_service_time = 300;
double DDN_packet_service_time = 300;

double link_transmission_time = 500;

double collective_block_size = 4*1024*16;

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


