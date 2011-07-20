/*                                                                              
 *  Blue Gene/P model                                                           
 *  Extern define file
 *  by Ning Liu                                                                 
 */

#ifndef BGP_EXTERN_H
#define BGP_EXTERN_H

#define OPT_MEM 64

#include "bgp.h"

///////////////////////////////////
// configuration
extern int nlp_per_pe;

extern int rootCN;

extern int nlp_DDN;
extern int nlp_Controller;
extern int nlp_FS;
extern int nlp_ION;
extern int nlp_CN;

extern int N_CN_per_DDN;
extern int N_ION_per_DDN;
extern int N_FS_per_DDN;
extern int N_controller_per_DDN;
extern int N_DDN_per_PE;

extern int N_FS_t;
extern int N_ION_per_FS;
extern int N_CN_per_ION;

extern int N_CN_per_FS;
extern int N_Disk_per_FS;

extern int NumDDN;
extern int NumControllerPerDDN;
extern int NumFSPerController;

extern int N_lp_per_FS;

//////////////////////////////////
// hardware parameter
extern double collective_block_size;

extern double payload_size;
extern double PVFS_payload_size;
extern double ACK_message_size;
extern double CN_message_wrap_time;
extern double PVFS_handshake_time; 

extern double CN_packet_service_time;
extern double ION_packet_service_time;
extern double FS_packet_service_time;
extern double CON_packet_service_time;
extern double DDN_packet_service_time;

extern double link_transmission_time;

// all links bandwidth
extern double CN_tree_bw;
extern double CN_ION_bw;
extern double ION_FS_bw;
extern double FS_CON_bw;
extern double CON_DDN_bw;

extern int msg_collective_counter;


// Compute Node Globals
extern void bgp_cn_init(CN_state* , tw_lp* );
extern void bgp_cn_eventHandler(CN_state* , tw_bf* , MsgData * , tw_lp* );
extern void bgp_cn_eventHandler_rc(CN_state* , tw_bf* , MsgData* , tw_lp* );
extern void bgp_cn_finish(CN_state* , tw_lp* );
extern int get_tree_next_hop( int );

// IO node Globals
extern void bgp_ion_init(ION_state* ,  tw_lp* );
extern void bgp_ion_eventHandler(ION_state* , tw_bf* , MsgData* , tw_lp* );
extern void bgp_ion_eventHandler_rc(ION_state* , tw_bf* , MsgData* , tw_lp* );
extern void bgp_ion_finish(ION_state* , tw_lp* );


// File Server Globals
extern void bgp_fs_init(FS_state* ,  tw_lp* );
extern void bgp_fs_eventHandler(FS_state* , tw_bf* , MsgData* , tw_lp* );
extern void bgp_fs_eventHandler_rc(FS_state* , tw_bf* , MsgData* , tw_lp* );
extern void bgp_fs_finish(FS_state* , tw_lp* );

// Controller Globals
extern void bgp_controller_init( CON_state* ,  tw_lp* );
extern void bgp_controller_eventHandler(CON_state* , tw_bf* , MsgData* , tw_lp* );
extern void bgp_controller_eventHandler_rc(CON_state*, tw_bf*, MsgData*, tw_lp* );
extern void bgp_controller_finish(CON_state* , tw_lp* );

// DDN Globals
extern void bgp_ddn_init(DDN_state* ,  tw_lp* );
extern void bgp_ddn_eventHandler(DDN_state* , tw_bf* , MsgData* , tw_lp* );
extern void bgp_ddn_eventHandler_rc(DDN_state* , tw_bf* , MsgData* , tw_lp* );
extern void bgp_ddn_finish(DDN_state* , tw_lp* );



#endif
