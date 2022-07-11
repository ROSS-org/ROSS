#ifndef INC_ross_types_h
#define INC_ross_types_h

/** @file ross-types.h
 *  @brief Definition of ROSS basic types
 */

typedef struct tw_statistics tw_statistics;
typedef struct tw_pq tw_pq;
#ifdef ROSS_QUEUE_kp_splay
typedef struct tw_eventpq tw_eventpq;
#endif
typedef struct tw_lptype tw_lptype;
typedef struct tw_petype tw_petype;
typedef struct tw_bf tw_bf;
typedef struct tw_eventq tw_eventq;
typedef struct tw_event tw_event;
typedef struct tw_lp tw_lp;
typedef struct tw_kp tw_kp;
typedef struct tw_pe tw_pe;
typedef struct avlNode *AvlTree;

#define MAX_TIE_CHAIN 100

/**
 * Synchronization protocol used
 */
enum tw_synch_e {
    NO_SYNCH,
    SEQUENTIAL,
    CONSERVATIVE,
    OPTIMISTIC,
    OPTIMISTIC_DEBUG,
    OPTIMISTIC_REALTIME,
};

typedef enum tw_synch_e tw_synch;

enum tw_lp_map {
    LINEAR,
    ROUND_ROBIN,
    CUSTOM
};
typedef enum tw_lp_map tw_lp_map;

/** tw_kpid -- Kernel Process (KP) id*/
typedef tw_peid tw_kpid;
typedef unsigned int tw_eventid;

typedef unsigned long long tw_stat;

typedef void (*pe_init_f) (tw_pe * pe);
typedef void (*pe_gvt_f) (tw_pe * pe);
typedef void (*pe_final_f) (tw_pe * pe);

/** tw_petype @brief Virtual Functions for per PE ops
 *
 * User model implements virtual functions for per PE operations.  Currently,
 * ROSS provides hooks for PE init, finalization and per GVT operations.
 */
struct tw_petype {
    pe_init_f pre_lp_init; /**< @brief PE initialization routine, before LP init */
    pe_init_f post_lp_init;  /**< @brief PE initialization routine, after LP init */
    pe_gvt_f gvt;  /**< @brief PE per GVT routine */
    pe_final_f final;  /**< @brief PE finilization routine */
};

/*
 * User implements virtual functions by giving us function pointers
 * for setting up an LP, handling an event on that LP, reversing the
 * event on the LP and cleaning up the LP for stats computation/collecting
 * results.
 */
typedef void (*init_f) (void *sv, tw_lp * me);
typedef tw_peid (*map_f) (tw_lpid);
typedef tw_lp * (*map_local_f) (tw_lpid);
typedef void (*map_custom_f) (void);
typedef void (*pre_run_f) (void *sv, tw_lp * me);
typedef void (*event_f) (void *sv, tw_bf * cv, void *msg, tw_lp * me);
typedef void (*revent_f) (void *sv, tw_bf * cv, void *msg, tw_lp * me);
typedef void (*commit_f) (void *sv, tw_bf * cv, void *msg, tw_lp * me);
typedef void (*final_f) (void *sv, tw_lp * me);

/**
 * tw_lptype
 * @brief Function Pointers for ROSS Event Handlers
 *
 **/
struct tw_lptype {
    init_f init; /**< @brief LP setup routine */
    pre_run_f pre_run; /**< @brief Second stage LP initialization */
    event_f event; /**< @brief LP event handler routine */
    revent_f revent;  /**< @brief LP Reverse event handler routine */
    commit_f commit;  /**< @brief LP Commit event routine */
    final_f final; /**< @brief Final handler routine */
    map_f map; /**< @brief LP Mapping of LP gid -> remote PE routine */
    size_t state_sz; /**< @brief Number of bytes that SV is for the LP */
};

// Type mapping function: gid -> type index
typedef tw_lpid (*tw_typemap_f) (tw_lpid gid);

/**
 * tw_statistics
 * @brief Statistics tallied over the duration of the simulation.
 * @attention If you change the order of this struct you must ensure that
 * tw_net_statistics() is updated!
 **/
struct tw_statistics {
    double s_max_run_time;

    tw_stat s_net_events;
    tw_stat s_nevent_processed;
    tw_stat s_nevent_abort;
    tw_stat s_e_rbs;

    tw_stat s_rb_total;
    tw_stat s_rb_primary;
    tw_stat s_rb_secondary;
    tw_stat s_fc_attempts;

    tw_stat s_pq_qsize;
    tw_stat s_nsend_network;
    tw_stat s_nread_network;
    tw_stat s_nsend_remote_rb;

    tw_stat s_nsend_loc_remote;
    tw_stat s_nsend_net_remote;
    tw_stat s_ngvts;
    tw_stat s_mem_buffers_used;

    tw_stat s_pe_event_ties;

    double s_min_detected_offset;

    tw_clock s_total;
    tw_clock s_init;
    tw_clock s_net_read;
    tw_clock s_net_other;
    tw_clock s_gvt;
    tw_clock s_fossil_collect;

    tw_clock s_event_abort;
    tw_clock s_event_process;
    tw_clock s_pq;
    tw_clock s_rollback;

    tw_clock s_cancel_q;

    tw_clock s_avl;
    tw_clock s_buddy;
    tw_clock s_lz4;
    tw_clock s_stat_comp;
    tw_clock s_stat_write;

    tw_stat s_events_past_end;

    tw_stat s_alp_nevent_processed;
    tw_stat s_alp_e_rbs;

#ifdef USE_RIO
    tw_clock s_rio_load;
    tw_clock s_rio_lp_init;
#endif
};

struct tw_eventq {
    size_t size;
    tw_event *head;
    tw_event *tail;
};

/**
 * tw_bf
 * @brief Reverse Computation Bitfield
 *
 * Some applications find it handy to have this bitfield when doing
 * reverse computation.  So we follow GTW tradition and provide it.
 */
struct tw_bf {
    unsigned int    c0:1;
    unsigned int    c1:1;
    unsigned int    c2:1;
    unsigned int    c3:1;
    unsigned int    c4:1;
    unsigned int    c5:1;
    unsigned int    c6:1;
    unsigned int    c7:1;
    unsigned int    c8:1;
    unsigned int    c9:1;
    unsigned int    c10:1;
    unsigned int    c11:1;
    unsigned int    c12:1;
    unsigned int    c13:1;
    unsigned int    c14:1;
    unsigned int    c15:1;
    unsigned int    c16:1;
    unsigned int    c17:1;
    unsigned int    c18:1;
    unsigned int    c19:1;
    unsigned int    c20:1;
    unsigned int    c21:1;
    unsigned int    c22:1;
    unsigned int    c23:1;
    unsigned int    c24:1;
    unsigned int    c25:1;
    unsigned int    c26:1;
    unsigned int    c27:1;
    unsigned int    c28:1;
    unsigned int    c29:1;
    unsigned int    c30:1;
    unsigned int    c31:1;
};

enum tw_event_owner {
    TW_pe_event_q = 1,        /**< @brief In a tw_pe.event_q list */
    TW_pe_pq = 2,           /**< @brief In a tw_pe.pq */
    TW_kp_pevent_q = 3,     /**< @brief In a tw_kp.pevent_q */
    TW_pe_anti_msg = 4,     /**< @brief Anti-message */
    TW_net_outq = 5,        /**< @brief Pending network transmission */
    TW_net_asend = 6,       /**< @brief Network transmission in progress */
    TW_net_acancel = 7,     /**< @brief Network transmission in progress */
    TW_pe_sevent_q = 8,     /**< @brief In tw_pe.sevent_q */
    TW_pe_free_q = 9,       /**< @brief In tw_pe.free_q */
#ifdef USE_RIO
    IO_buffer = 10,         /**< @brief RIO captured event */
#endif
};
typedef enum tw_event_owner tw_event_owner;

/**
 * tw_out
 * @brief Rollback-aware output mechanism
 *
 * Regularly requested feature: rollback-aware output.  This will allow us to
 * create an output stream without messages from cancelled events.
 */
typedef struct tw_out {
    struct tw_out *next;
    tw_kp *owner;
    /** The actual message content */
    char message[256 - 2*sizeof(void *)];
} tw_out;

#ifdef USE_RAND_TIEBREAKER
typedef struct tw_causal_origin {
    tw_peid pe_id;
    tw_eventid event_id;
} tw_unique_event_id;

static inline int tw_unique_event_id_eq(tw_unique_event_id e, tw_unique_event_id n)
{
    if (e.pe_id == n.pe_id)
    {
        if (e.event_id == n.event_id)
        {
            return 1;
        }
    }
    return 0;
}



typedef struct tw_event_sig {
    tw_stime recv_ts;
    tw_stime priority;
    tw_stime event_tiebreaker[MAX_TIE_CHAIN];
    unsigned int tie_lineage_length;
} tw_event_sig;

static inline tw_event_sig tw_get_init_sig(tw_stime recv_ts, tw_stime priority, tw_stime event_tiebreaker)
{
    tw_event_sig e;
    memset(&e, 0, sizeof(tw_event_sig));
    e.recv_ts = recv_ts;
    e.priority = priority;
    for (size_t i = 0; i < MAX_TIE_CHAIN; i++) {
        e.event_tiebreaker[i] = event_tiebreaker;
    }
    return e;
}
#endif

/**
 * tw_event:
 * @brief Event Stucture
 *
 * Holds entire event structure, one is created for each and every
 * event in use.
 */
struct tw_event {
    tw_event *next;
    tw_event *prev;
#if defined(ROSS_QUEUE_splay) || defined(ROSS_QUEUE_kp_splay)
    tw_event *up;                   /**< @brief Up pointer for storing membufs in splay tree */
#endif
#ifdef ROSS_QUEUE_heap
    unsigned long heap_index;       /**< @brief Index for storing membufs in heap queue */
#endif

    tw_event *cancel_next;          /**< @brief Next event in the cancel queue for the dest_pe */
    tw_event *caused_by_me;         /**< @brief Start of event list caused by this event */
    tw_event *cause_next;           /**< @brief Next in parent's caused_by_me chain */

    tw_eventid   event_id;          /**< @brief Unique id assigned by src_lp->pe if remote. */

#ifdef USE_RAND_TIEBREAKER
    tw_event_sig sig;
#endif

    /** Status of the event's queue location(s). */
    struct {
        unsigned char owner;        /**< @brief Owner of the next/prev pointers; see tw_event_owner */
        unsigned char cancel_q;     /**< @brief Actively on a dest_lp->pe's cancel_q */
        unsigned char cancel_asend;
        unsigned char remote;       /**< @brief Indicates union addr is in 'remote' storage */
    } state;

    tw_bf        cv;                /**< @brief Used by app during reverse computation. */
    void *delta_buddy;              /**< @brief Delta memory from buddy allocator. */
    size_t      delta_size;         /**< @brief Size of delta. */

    unsigned int critical_path;     /**< @brief Critical path of this event */

    tw_lp       *dest_lp;           /**< @brief Destination LP ID */
    tw_lp       *src_lp;            /**< @brief Sending LP ID */
    tw_stime     recv_ts;           /**< @brief Actual time to be received */

    tw_peid      send_pe;
    tw_lpid      send_lp;           /**< @brief sending LP ID for data collection uses */
    tw_lpid      dest_lpid;
    tw_stime     send_ts;

    tw_out *out_msgs;               /**< @brief Output messages */
};

/**
 * tw_lp @brief LP State Structure
 *
 * Holds our state for the LP, including the lptype and a pointer
 * to the user's current state.  The lptype is copied into the tw_lp
 * in order to save the extra memory load that would otherwise be
 * required (if we stored a pointer).
 *
 * Specific PE's service specific LPs, each PE has a linked list of
 * the LPs it services, this list is made through the pe_next field
 * of the tw_lp structure.
 */
struct tw_lp {
    tw_lpid id; /**< @brief local LP id */
    tw_lpid gid; /**< @brief global LP id */

    tw_pe *pe;

    /*
    * pe_next  -- Next LP in the PE's service list.  ????
    */
    tw_kp *kp; /**< @brief kp -- Kernel process that we belong to (must match pe). */

    void *cur_state; /**< @brief Current application LP data */
    tw_lptype  *type; /**< @brief Type of this LP, including service callbacks */
    tw_rng_stream *rng; /**< @brief  RNG stream array for this LP */
    tw_rng_stream *core_rng; /**< @brief RNG stream array for ROSS non-model operation - possible alternative to a model_rng pointer array*/

    unsigned int critical_path; /**< @brief Critical path value for this LP */

    /* ROSS instrumentation structs */
    struct st_model_types *model_types;
    struct st_lp_stats *lp_stats;
    struct st_lp_stats *last_stats[3];

    /* tw_suspend variables */
    tw_event    *suspend_event;
#ifdef USE_RAND_TIEBREAKER
    tw_event_sig suspend_sig;
#else
    tw_stime     suspend_time;
#endif
    unsigned int suspend_error_number;
    unsigned int suspend_do_orig_event_rc;
    unsigned int suspend_flag;
};

/**
 * tw_kp KP State Structure
 *
 * Holds our state for the Kernel Process (KP), which consists only of
 * processed event list for a collection of LPs.
 */
struct tw_kp {
    tw_kpid id;     /**< @brief ID number, otherwise its not available to the app */
    tw_pe *pe;      /**< @brief PE that services this KP */
    tw_kp *next;    /**< @brief Next KP in the PE's service list */
    tw_out *output; /**< @brief Output messages */
    int lp_count;

#ifdef ROSS_QUEUE_kp_splay
    tw_eventpq *pq;

    tw_kp *prev;
    tw_kp *up;
#endif

#ifdef AVL_TREE
    /* AVL tree root */
    AvlTree avl_tree;
#endif

    tw_eventq pevent_q; /**< @brief Events processed by LPs bound to this KP */
#ifdef USE_RAND_TIEBREAKER
    tw_event_sig last_sig; /**< @brief Event signature of the current event being processed */
#else
    tw_stime last_time; /**< @brief Time of the current event being processed */
#endif
    tw_stat s_nevent_processed; /**< @brief Number of events processed */

    long s_e_rbs; /**< @brief Number of events rolled back by this LP */
    long s_rb_total; /**< @brief Number of total rollbacks by this LP */
    long s_rb_secondary; /**< @brief Number of secondary rollbacks by this LP */
    struct st_kp_stats *kp_stats;
    struct st_kp_stats *last_stats[3];

};

/**
 * tw_pe @brief Holds the entire PE state
 *
 */
struct tw_pe {
    tw_peid    id;
    tw_petype  type; /**< @brief Model defined PE type routines */

    tw_eventq event_q; /**< @brief Linked list of events sent to this PE */
    tw_event *cancel_q; /**< @brief List of canceled events */
    tw_pq *pq; /**< @brief Priority queue used to sort events */

    tw_eventq free_q; /**< @brief Linked list of free tw_events */
    tw_event *abort_event; /**< @brief Placeholder event for when free_q is empty */
    tw_event *cur_event; /**< @brief Current event being processed */
    tw_eventq sevent_q; /**< @brief events already sent over the network */

    unsigned char *delta_buffer[3]; /**< @brief buffers used for delta encoding */

#ifdef AVL_TREE
    /* AVL node head pointer and size */
    AvlTree avl_list_head;
    unsigned avl_tree_size;
#endif

    tw_clock clock_offset; /**< @brief Initial clock value for this PE */
    tw_clock clock_time; /**< @brief  Most recent clock value for this PE */

    unsigned char cev_abort; /**< @brief Current event being processed must be aborted */
    unsigned char gvt_status; /**< @brief Bits available for gvt computation */

#ifdef USE_RAND_TIEBREAKER
    tw_event_sig trans_msg_sig; /**< @brief Last transient messages' time signature */
    tw_event_sig GVT_sig; /**< @brief Global Virtual Time Signature */
    tw_event_sig GVT_prev_sig;
    tw_event_sig LVT_sig; /**< @brief Local (to PE) Virtual Time Signature */
#else
    tw_stime trans_msg_ts; /**< @brief Last transient messages' time stamp */
    tw_stime GVT; /**< @brief Global Virtual Time */
    tw_stime GVT_prev;
    tw_stime LVT; /**< @brief Local (to PE) Virtual Time */
#endif

#ifdef ROSS_GVT_mpi_allreduce
    long long s_nwhite_sent;
    long long s_nwhite_recv;
#endif

    tw_wtime start_time; /**< @brief When this PE first started execution */
    tw_wtime end_time; /**< @brief When this PE finished its execution */

    tw_statistics stats; /**< @brief per PE counters */

#ifndef ROSS_NETWORK_none
    void           *hash_t; /**< @brief Array of incoming events from remote pes, Note: only necessary for distributed DSR*/
#ifdef ROSS_NETWORK_mpi
    tw_eventid     seq_num; /**< @brief Array of remote send counters for hashing on, size == g_tw_npe */
#else
    tw_eventid    *seq_num; /**< @brief Array of remote send counters for hashing on, size == g_tw_npe */
#endif
#endif

    tw_rng  *rng; /**< @brief Pointer to the random number generator on this PE */
    tw_rng  *core_rng; /**< @brief Pointer to the core random number generator on this PE */
};

#ifdef USE_RAND_TIEBREAKER
static inline int min_int(int x, int y)
{
  return (x < y) ? x : y;
}

//compares the 'new' event to the signature. If the new event is to occur
//n_sig later (larger) than e_sig signature, return -1
//n_sig before (smaller) than e_sig signature, return 1
//at the signature - return 0
static inline int tw_event_sig_compare(tw_event_sig e_sig, tw_event_sig n_sig)
{
    int time_compare = TW_STIME_CMP(e_sig.recv_ts, n_sig.recv_ts);
    if (time_compare != 0)
        return time_compare;
    else {
        //then we compare the user defined priority first
        int prio_compare = TW_STIME_CMP(e_sig.priority, n_sig.priority);
        if (prio_compare != 0)
            return prio_compare;
        else {
            //if tie with user pririty then we use tiebreaker
            int min_len = min_int(e_sig.tie_lineage_length, n_sig.tie_lineage_length);
            for(int i = 0; i < min_len; i++) //lexicographical ordering
            {
                if (e_sig.event_tiebreaker[i] < n_sig.event_tiebreaker[i])
                    return -1;
                else if (e_sig.event_tiebreaker[i] > n_sig.event_tiebreaker[i])
                    return 1;
            }
            if (e_sig.tie_lineage_length == n_sig.tie_lineage_length) //total tie
                return 0;
            else if (e_sig.tie_lineage_length > n_sig.tie_lineage_length) //give priority to one with shorter lineage
                return 1;
            else
                return -1;
            }
        }
}
#endif

#endif
