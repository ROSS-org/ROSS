/**
 * RAID File Server Farm Simulation
 *
 * FILE: raid-extern.h
 * AUTHOR: Matt Perry
 * DATE: Apr 27 2011
 */

#ifndef RAID_EXTERN_H
#define RAID_EXTERN_H

#include "raid.h"

/* raid.c */
extern void message_error( Event event_type );

/* raid-server.c */
extern void raid_server_calculate_bandwidth( ServerState* s );
extern long raid_server_num_blocks( ServerState* s, tw_lp* lp );
extern void raid_server_init( ServerState* s, tw_lp* lp );
extern void raid_server_eventhandler( ServerState* s, tw_bf* cv, MsgData* m, tw_lp* lp );
extern void raid_server_eventhandler_rc( ServerState* s, tw_bf* cv, MsgData* m, tw_lp* lp );
extern void raid_server_finish( ServerState* s, tw_lp* lp );

/* raid-controller.c */
extern void raid_controller_gen_send( tw_lpid dest, Event event_type, tw_stime event_time, tw_lp* lp );
extern tw_lpid raid_controller_find_server( tw_lp* lp );
extern void raid_controller_init( ContState* s, tw_lp* lp );
extern void raid_controller_eventhandler( ContState* s, tw_bf* cv, MsgData* m, tw_lp* lp );
extern void raid_controller_eventhandler_rc( ContState* s, tw_bf* cv, MsgData* m, tw_lp* lp );
extern void raid_controller_finish( ContState* s, tw_lp* lp );

/* raid-disk.c */
extern void raid_disk_gen_send( tw_lpid dest, Event event_type, tw_stime event_time, tw_lp* lp );
extern tw_stime raid_disk_fail_time( tw_lp* lp );
extern tw_lpid raid_disk_find_controller( tw_lp* lp );
extern void raid_disk_init( DiskState* s, tw_lp* lp );
extern void raid_disk_eventhandler( DiskState* s, tw_bf* cv, MsgData* m, tw_lp* lp );
extern void raid_disk_eventhandler_rc( DiskState* s, tw_bf* cv, MsgData* m, tw_lp* lp );
extern void raid_disk_finish( DiskState* s, tw_lp* lp );

/* raid-io.c */
extern void raid_io_gen_send( tw_lpid dest, Event event_type, tw_stime event_time, tw_lp* lp );
extern tw_lpid raid_io_find_server( tw_lp* lp );
extern void raid_io_init( IOState* s, tw_lp* lp );
extern void raid_io_eventhandler( IOState* s, tw_bf* cv, MsgData* m, tw_lp* lp );
extern void raid_io_eventhandler_rc( IOState* s, tw_bf* cv, MsgData* m, tw_lp* lp );
extern void raid_io_finish( IOState* s, tw_lp* lp );

/* raid-globals.c */
extern int g_disk_distro;
extern int g_ttl_fs;
extern int g_nfs_per_pe;
extern tw_lpid g_nlp_per_pe;
extern int g_raid_start_events;
extern int g_add_mem;
extern Stats g_stats;

#endif
