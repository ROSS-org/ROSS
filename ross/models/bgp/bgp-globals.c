/*
 * BG/P model globals
 * Ning Liu
 */

#include "bgp.h"

int N_ION_active = 8; // default, we can set it in command line
int N_FS_active = 123;
int opt_mem = 1024;
int burst_buffer_on = 0;

int computation_time = 5000;
int N_checkpoint = 2;

double CN_ION_meta_payload = 20;
double CN_out_bw = 0.7;
double CN_in_bw = 0.7;

double ION_CONT_msg_prep_time = 1024;
double ION_FS_meta_payload = 4*1024;
double ION_CN_out_bw = 0.7;
double ION_CN_in_bw = 0.7;
double ION_FS_out_bw = 0.28;
double ION_FS_in_bw = 0.23;

double FS_ION_in_bw = 0.4;
double FS_ION_out_bw = 0.42;
double FS_DDN_in_bw = 0.6;
double FS_DDN_out_bw = 0.6;
double FS_CONT_msg_prep_time = 128;
double DDN_FS_out_bw = 0.6;
double DDN_FS_in_bw = 0.6;

// used in write
//double handshake_payload_size = 371610;
double handshake_payload_size = 2716100;

double lookup_meta_size = 4*1024;
double close_meta_size = 4*1024;
double FS_DDN_meta_payload = 64*1024;
double FS_DDN_read_meta = 1024;
double CONT_CONT_msg_prep_time = 1024;
double DDN_ACK_size = 1024;
double CONT_FS_in_bw = 0.6;
double CN_CONT_msg_prep_time = 64;

long long stripe_size = 4*1024*1024;
double PVFS_payload_size = 4*1024*1024;
double payload_size = 4*1000*1000;
/////////////////////

double CONT_FS_msg_prep_time = 224;

int N_PE;

double meta_payload_size = 20;
double create_payload_size = 4*1024;

double ION_out_bw = 0.27;
double FS_in_bw = 0.31;
double ION_in_bw = 0.27;
double FS_out_bw = 0.3;
//////////////////////
int nlp_DDN;
int nlp_Controller;
int nlp_FS;
int nlp_ION;
int nlp_CN;

int rootCN;

int NumDDN = 128;
int NumControllerPerDDN = 1;
int NumFSPerController = 1;

int N_ION_per_FS = 5;
int N_CN_per_ION = 256;

int N_CN_per_DDN;
int N_ION_per_DDN;
int N_FS_per_DDN;
int N_controller_per_DDN;
int N_DDN_per_PE;

// default packet size


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
double FS_CON_bw= 0.6; 
double CON_DDN_bw= 0.6; 

int msg_collective_counter = 0;

int nlp_per_pe;
/////////////////////

// Test RC code in serial mode
int g_test_rc = 0;
int N_nodes = 1;

////////////

