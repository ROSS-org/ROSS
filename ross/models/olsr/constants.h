/** @file constants.h
	@brief File of OLSR Constants

	Constants used by OLSR as defined by RFC-3626

*/

#define C 0.0625

#define HELLO_INTERVAL	 2 /* sec */
#define REFRESH_INTERVAL 2 /* sec */
#define TC_INTERVAL 	 5 /* sec */
#define MID_INTERVAL	 TC_INTERVAL /* sec */
#define HNA_INTERVAL	 TC_INTERVAL /* sec */

#define NEIGHB_HOLD_TIME 3 * REFRESH_INTERVAL /* sec */
#define TOP_HOLD_TIME    3 * TC_INTERVAL /* sec */
#define DUP_HOLD_TIME	 30 /* sec */
#define MID_HOLD_TIME    3 * MID_INTERVAL
#define HNA_HOLD_TIME	 3 * HNA_INTERVAL


/** Message Types **/
#define HELLO_MESSAGE 	 1
#define TC_MESSAGE		 2
#define MD_MESSAGE		 3
#define HNA_MESSAGE		 4

/** Link Types **/
#define UNSPEC_LINK		 0
#define ASM_LINK		 1
#define SYM_LINK		 2
#define LOST_LINK		 3

/** Neighbor Types **/
#define NOT_NEIGH		 0
#define SYM_NEIGH		 1
#define MPR_NEIGH		 2

/** Link Hysteresis **/
#define HYST_THRESHOLD_HIGH 0.8
#define HYST_THRESHOLD_LOW  0.3
#define HYST_SCALING		0.5

/** Willingness **/
#define WILL_NEVER		 0
#define WILL_LOW		 1
#define WILL_DEFAULT	 3
#define WILL_HIGH		 6
#define WILL_ALWAYS		 7

/** Misc Constants **/
#define TC_REDUNDANCY    0
#define MPR_COVERAGE     1
#define MAXJITTER		 HELLO_INVERVAL / 4