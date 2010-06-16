#ifndef INC_tmr_h
#define INC_tmr_h

#include <ross.h>

	/*
	 * PHOLD Types
	 */

typedef struct tmr_state tmr_state;
typedef struct tmr_message tmr_message;

struct tmr_state
{
	tw_event	*timer;
};

struct tmr_message
{
	tw_event	*old_timer;
	tw_stime	 old_time;
};

	/*
	 * PHOLD Globals
	 */
static unsigned int verbose = 0;
static FILE *f = NULL;
static unsigned int offset_lpid = 0;
static tw_stime mult = 1.4;
static tw_stime percent_remote = 0.25;
static unsigned int ttl_lps = 0;
static unsigned int nlp_per_pe = 8;
static int g_tmr_start_events = 1;
static int optimistic_memory = 100;

// rate for timestamp exponential distribution
static tw_stime mean = 1.0;

static char run_id[1024] = "undefined";

#endif
