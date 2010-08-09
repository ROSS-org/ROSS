#ifndef _SRW_H
#define _SRW_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file srw.h
 * @brief SRW header
 *
 * Header file for the SRW waveform.  Work in progress.
 */

#define SRW_COMM_MEAN       150.0 ///< Rate of communication
#define SRW_MOVE_MEAN        50.0 ///< Rate of movement
#define SRW_GPS_RATE         10.0 ///< Seconds in between GPS tracking


#define SRW_MAX_GROUP_SIZE     20 ///< Max radios per master (initial)
#define SRW_MAX_GROUP_OVERSIZE 30 ///< Hard limit on max radios per master

typedef enum {
  COMMUNICATION,
  MOVEMENT,
  GPS
} srw_event_type;

/**
 * struct to keep track of individual SRW nodes within the group.
 */
typedef struct {
  long node_id;    ///< Node ID for this node
  double lng;      ///< Longitude for this node only 
  double lat;      ///< Latitude for this node only
  int movements;   ///< Number of movements for this node only
  int comm_fail;   ///< Failed communications for this node only
  int comm_try;    ///< Attempted communications for this node only
} srw_node_info;

/**
 * We should always keep in mind that this is basically the state of the
 * master, so we should keep track of things relevant from the perspective
 * of a master.
 */
typedef struct {
  int num_radios; ///< Number of radios INCLUDING the master

  int movements;  ///< Number of movements INCLUDING the master

  int comm_fail;  ///< Failed communications
  int comm_try;   ///< Attempted communications

  /// Array of srw_node_info to track entire group
  srw_node_info nodes[SRW_MAX_GROUP_OVERSIZE];
} srw_state;

/**
 */
typedef struct {
  srw_event_type type; ///< What type of message is this?
  long node_id;        ///< Node ID responsible for this event
  double lng;          ///< Longitude for this node only
  double lat;          ///< Latitude for this node only
} srw_msg_data;

#ifdef __cplusplus
}
#endif

#endif /* _SRW_H */
