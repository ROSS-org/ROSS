#ifndef INC_disksim_h
#define INC_disksim_h

#include <ross.h>

#define MAX_HOURS 43800 // hours in 5 years
#define MTTF 1000000 // 1M hours

typedef struct disksim_state disksim_state;
typedef struct disksim_ras_state disksim_ras_state;
typedef struct disksim_message disksim_message;

struct disksim_state
{
	unsigned int number_of_failures;
};

struct disksim_ras_state
{
   unsigned int disk_failure_per_hour[MAX_HOURS+2];
};

struct disksim_message
{
	tw_lpid disk_that_failed;
        tw_stime time_of_failure;
};

tw_stime lookahead = 1.0;
static unsigned int offset_lpid = 0;
static tw_stime mult = 1.4;
static unsigned int ttl_lps = 0;
static unsigned int nlp_per_pe = 8;
static int g_disksim_start_events = 16;
static int optimistic_memory = 100;

static int disksim_distribution=2;

// rate for timestamp exponential distribution
static tw_stime mean = 1.0;

static char run_id[1024] = "undefined";

#endif
