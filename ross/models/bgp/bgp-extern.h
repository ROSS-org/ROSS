/*                                                                              
 *  Blue Gene/P model                                                           
 *  Extern define file
 *  by Ning Liu                                                                 
 */

#ifndef BGP_EXTERN_H
#define BGP_EXTERN_H

#define OPT_MEM 64

#include "bgp.h"

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

// Inheriate from torus model
extern void torus_init(CN_state* , tw_lp* );
extern void torus_setup(CN_state* , tw_lp* );
extern void packet_send(CN_state* , tw_bf* , MsgData* , tw_lp* );
extern void packet_process(CN_state* , tw_bf* , MsgData* , tw_lp* );
extern void packet_generate(CN_state* , tw_bf* , MsgData* , tw_lp* );
extern void packet_arrive( CN_state* , tw_bf* , MsgData* , tw_lp* );
extern void dimension_order_routing(CN_state* , MsgData* , tw_lpid* );


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

extern double CN_packet_service_time;
extern double ION_packet_service_time;
extern double FS_packet_service_time;
extern double Disk_packet_service_time;

extern double link_transmission_time;

extern int msg_collective_counter;

#endif
