#ifndef _SRW_H
#define _SRW_H

/**
 * @file srw.h
 * @brief SRW header
 *
 * Header file for the SRW waveform.  Work in progress.
 */

#define SRW_COMM_MEAN      150.0  // Rate of communication
#define SRW_MOVE_MEAN      50.0   // Rate of movement
#define SRW_GPS_RATE       10.0   // Seconds in between GPS tracking



#define SRW_MAX_GROUP_SIZE 20   // Max radios per master (initial)

typedef enum {
  COMMUNICATION,
  MOVEMENT,
  GPS
} srw_event_type;

/**
 * We should always keep in mind that this is basically the state of the
 * master, so we should keep track of things relevant from the perspective
 * of a master.
 */
typedef struct {
  int num_radios; /**< Number of radios INCLUDING the master */

  int movements;

  int comm_fail;  /**< Failed communications */
  int comm_try;   /**< Attempted communications; comm_try - comm_fail =
		     successful communication attempts */
} srw_state;

/**
 */
typedef struct {
  srw_event_type type;
  double lng, lat;
} srw_msg_data;

#endif /* _SRW_H */
