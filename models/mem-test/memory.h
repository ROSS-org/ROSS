#ifndef INC_mem_h
#define INC_mem_h

#include <ross.h>

	/*
	 * Model Types
	 */

#define Q_SIZE 1000

FWD(struct, mem_state);
FWD(struct, mem_message);
FWD(struct, mem_statistics);
FWD(struct, mem_packet);

DEF(struct, mem_statistics)
{
	tw_stat s_rb;
	tw_stat s_sent;
	tw_stat s_recv;

	tw_stat s_mem_alloc;
	tw_stat s_mem_free;

	tw_stat s_mem_alloc_rc;
	tw_stat s_mem_free_rc;

	tw_stat s_mem_get;
	tw_stat s_mem_get_rc;
};

DEF(struct, mem_state)
{
	mem_statistics	 stats;
	long int	 dummy_state;
};

DEF(struct, mem_message)
{
	long int	 dummy_data;
};

DEF(struct, mem_packet)
{
	char bytes[512];
};

	/*
	 * Model Globals
	 */

static unsigned int offset_lpid = 0;
static tw_stime mult = 1.4;
static tw_stime percent_remote = 0.25;
static unsigned int ttl_lps = 0;
static unsigned int nlp_per_pe = 8;
static int g_mem_start_events = 1;
static int optimistic_memory = 100;

// rate for timestamp exponential distribution
static tw_stime mean = 1.0;

static char run_id[1024] = "undefined";

static unsigned int nbufs = 2;
static mem_statistics g_stats;
static char test_string [255] = "This is a test!";
static tw_fd	my_fd;

#endif
