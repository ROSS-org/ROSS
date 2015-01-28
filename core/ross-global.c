#include <ross.h>

	/*
	 * LP data structures are allocated dynamically when the
	 * process starts up based on the number it requires.
	 *
	 * g_tw_nlp         -- Number of LPs on this processor
	 * g_tw_lp_offset   -- global id of g_tw_lp[0] (on this processor)
	 * g_tw_nkp         -- Number of KPs on this processor
                            IF this is 1, then it gets over written as nkp_per_pe * g_tw_npe
                            thus it is total KPs in simulation, not on this processor
	 * g_tw_lp          -- Public LP object array (on this processor)
	 * g_tw_kp          -- Public KP object array (on this processor)
	 * g_tw_fossil_attempts  -- Number of times fossil_collect is called
         * g_tw_nRNG_per_lp -- Number of RNG per LP
	 */

tw_synch     g_tw_synchronization_protocol=NO_SYNCH;
map_local_f  g_tw_custom_lp_global_to_local_map=NULL;
map_custom_f g_tw_custom_initial_mapping=NULL;
tw_lp_map    g_tw_mapping=LINEAR;

tw_lpid         g_tw_nlp = 0;
tw_lpid		g_tw_lp_offset = 0;
tw_kpid         g_tw_nkp = 1;
tw_lp		**g_tw_lp = NULL;
tw_kp		**g_tw_kp = NULL;
int             g_tw_fossil_attempts = 0;
unsigned int    g_tw_nRNG_per_lp = 1;
tw_lpid         g_tw_rng_default = 1;
tw_seed        *g_tw_rng_seed = NULL;
unsigned int	g_tw_sim_started = 0;
size_t g_tw_msg_sz;
size_t g_tw_delta_sz = 0;
uint32_t g_tw_buddy_alloc = 0; /**< Allocation for buddy system */
buddy_list_bucket_t *g_tw_buddy_master = 0;
uint32_t g_tw_avl_node_count = 18;

#if ROSS_MEMORY
unsigned int	g_tw_memory_nqueues = 64;
#else
unsigned int	g_tw_memory_nqueues = 0;
#endif

size_t		g_tw_memory_sz = 0;
size_t		g_tw_event_msg_sz = 0;

        /*
         * Minimum lookahead for a model -- model defined when
         * using the Simple Synchronization Protocol (conservative)
         */
tw_stime g_tw_lookahead=0.005;

        /*
         * Minimum detected timestamp offset used by the simulation at
         * runtime, can be used to help tune conservative protocol runs.
         */
tw_stime g_tw_min_detected_offset=DBL_MAX;

	/**
	 * Number of messages to process at once out of the PQ before
	 * returning back to handling things like GVT, message recption,
	 * etc.  AKA the "batch" parameter to ROSS.
	 */
unsigned int g_tw_mblock = 16;
unsigned int g_tw_gvt_interval = 16;
tw_stime     g_tw_ts_end = 100000.0;

	/*
	 * g_tw_npe             -- Number of PEs on this processor (usually one)
	 * g_tw_pe              -- Public PE object array (on this processor)
	 * g_tw_events_per_pe   -- Number of events to place in for each PE.
	 *                         MUST be > 1 because of abort buffer.
	 */
tw_peid		g_tw_npe = 1;
tw_pe		**g_tw_pe;
unsigned int    g_tw_events_per_pe = 2048;
/** Number of extra events allocated per PE.  Command-line customizable. */
unsigned int    g_tw_events_per_pe_extra = 0;

unsigned int	g_tw_gvt_threshold = 1000;
unsigned int	g_tw_gvt_done = 0;

	/*
	 * Network variables:
	 * g_tw_masternode -- pointer to GVT net node, for GVT comp
	 */
unsigned int	g_tw_net_device_size = 0;
tw_node		g_tw_mynode = -1;
tw_node		g_tw_masternode = -1;

FILE		*g_tw_csv = NULL;


/*
 *
 */

tw_stime g_tw_clock_rate=1000000000.0; // Default to 1 GHz

// LP Type Mapping
tw_lptype * g_tw_lp_types = NULL;
tw_typemap_f g_tw_lp_typemap = &map_onetype;
