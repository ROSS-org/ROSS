/**
 * RAID File Server Farm Simulation
 *
 * FILE: raid-types.h
 * AUTHOR: Matt Perry
 * DATE: Apr 27 2011
 */

#ifndef RAID_TYPES_H
#define RAID_TYPES_H

#include "raid.h"

/* Disk */
#define MAX_HOURS 157680000 // 5 years in seconds
#define MTTF 3600000000     // 3.6B seconds (1M Hours)

/* LP */
#define FS_PER_IO 1         // 1 IO forwarding node per file server
#define CONT_PER_FS 4       // 4 controllers per file server
#define DISK_PER_RC 8       // 8 disks per raid controller
#define LPS_PER_FS ( 1 + FS_PER_IO + CONT_PER_FS + ( DISK_PER_RC * CONT_PER_FS ) )

/* Random */
#define STD_DEV .10         // Std deviation of random times (10% of mean)

/* IO */
#define IDLE_TIME 10        // Avg time per idle phase (IO)
#define BUSY_TIME 100       // Avg time per busy phase (IO)
#define BLOCK_SIZE 4        // Block size in KB

/* Bandwidth */
#define CONT_FS_BW 8388608  // Controller Bandwidth in KB
#define THROTTLE .25        // Percent to throttle bandwidth

/* Controller */
#define REBUILD_TIME 3600   // Time to rebuild a volume (1hr)

/* Disk */
#define REPLACE_TIME 600    // Time to physically replace disk (10m)

/* typedef */
typedef struct _io_state IOState;
typedef struct _server_state ServerState;
typedef struct _controller_state ContState;
typedef struct _disk_state DiskState;
typedef struct _rc RC;
typedef struct _message_data MsgData;
typedef struct _aggregate_stats Stats;
typedef enum _event Event;
typedef enum _io_mode IOMode;
typedef enum _controller_condition ContCondition;

/* event types */
enum _event
{
    IO_IDLE,
    IO_BUSY,
    REBUILD_START,
    REBUILD_FINISH,
    RAID_FAILURE,
    DISK_FAILURE,
    DISK_REPLACED,
};

/* raid-io.c */
enum _io_mode
{
    IDLE,
    BUSY
};

struct _io_state
{
    tw_lpid m_server;
    IOMode mode;
    tw_stime ttl_idle;
    tw_stime ttl_busy;
};

/* raid-server.c */
struct _server_state
{
    IOMode mode;
    tw_stime mode_change_timestamp;
    double bandwidth;
    long double num_blocks_wr;
    int num_controllers_good_health;
    int num_controllers_rebuilding;
    int num_controllers_failure;
};

/* raid-controller.c */
enum _controller_condition
{
    HEALTHY,
    WAIT,
    REBUILD,
    FAILED
};

struct _controller_state
{
    tw_lpid m_server;
    ContCondition condition;
    int num_rebuilds;
};

/* raid-disk.c */
struct _disk_state
{
    tw_lpid m_controller;
    int num_failures;
};

/* message */
struct _rc
{
    tw_stime io_time;
    tw_stime server_timestamp;
    long server_blocks;
    ContCondition controller_condition;
    unsigned int rng_calls;
};

struct _message_data
{
    Event event_type;
    RC rc;
};

/* stats */
struct _aggregate_stats
{
    tw_stime ttl_busy_time;
    tw_stime ttl_idle_time;
    long double ttl_blocks_wr;
    int ttl_controller_failures;
    int ttl_controller_rebuilds;
    int ttl_disk_failures;
};


#endif
