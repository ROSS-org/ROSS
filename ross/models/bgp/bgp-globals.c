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
int NumControllerPerDDN = 2;
int NumFSPerController = 1;

int N_ION_per_FS = 2;
int N_CN_per_ION = 64;

int N_CN_per_DDN;
int N_ION_per_DDN;
int N_FS_per_DDN;
int N_controller_per_DDN;
int N_DDN_per_PE;


double CN_packet_service_time = 300;
double ION_packet_service_time = 300;
double FS_packet_service_time = 800;
double Disk_packet_service_time = 300;

double link_transmission_time = 500;

int msg_collective_counter = 0;

int nlp_per_pe;
/////////////////////

// Test RC code in serial mode
int g_test_rc = 0;
int N_nodes = 1;

////////////


