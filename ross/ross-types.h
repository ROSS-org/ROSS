#ifndef INC_ross_types_h
#define INC_ross_types_h

/*
 * ROSS: Rensselaer's Optimistic Simulation System.
 * Copyright (c) 1999-2003 Rensselaer Polytechnic Instutitute.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *
 *      This product includes software developed by David Bauer,
 *      Dr. Christopher D.  Carothers, and Shawn Pearce of the
 *      Department of Computer Science at Rensselaer Polytechnic
 *      Institute.
 *
 * 4. Neither the name of the University nor of the developers may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * 5. The use or inclusion of this software or its documentation in
 *    any commercial product or distribution of this software to any
 *    other party without specific, written prior permission is
 *    prohibited.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS
 * IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *
 */

struct tw_statistics_t;
typedef tw_statistics_t tw_statistics;
struct tw_pq_t;
typedef tw_pq_t tw_pq;
struct tw_lptype_t;
typedef tw_lptype_t tw_lptype;
struct tw_petype_t;
typedef tw_petype_t tw_petype;
struct tw_bf_t;
typedef tw_bf_t tw_bf;
struct tw_lp_state_t;
typedef tw_lp_state_t tw_lp_state;
struct tw_eventq_t;
typedef tw_eventq_t tw_eventq;
struct tw_event_t;
typedef tw_event_t tw_event;
struct tw_lp_t;
typedef tw_lp_t tw_lp;
struct tw_kp_t;
typedef tw_kp_t tw_kp;
struct tw_pe_t;
typedef tw_pe_t tw_pe;
struct tw_log_t;
typedef tw_log_t tw_log;

#ifdef ROSS_MEMORY
struct tw_memoryq_t;
typedef tw_memoryq_t tw_memoryq;
struct tw_memory_t;
typedef tw_memory_t tw_memory;
#endif

enum tw_event_owner_t;
typedef tw_event_owner_t tw_event_owner;
enum tw_lp_map_t;
typedef tw_lp_map_t tw_lp_map;

/*
 * synchronization protocol used
 */ 
enum tw_synch_e
  {
    NO_SYNCH,
    SEQUENTIAL,
    CONSERVATIVE,
    OPTIMISTIC
  };

typedef enum tw_synch_e tw_synch;

enum tw_lp_map_t
{
  LINEAR,
  ROUND_ROBIN,
  CUSTOM
};

/* 
 * tw_kpid -- Kernel Process (KP) id
 * tw_fd   -- used to distinguish between memory and event arrays
 */
typedef tw_peid tw_kpid;
typedef unsigned long tw_fd;
typedef unsigned long long tw_stat;

/*
 * User model implements virtual functions for per PE operations.  Currently,
 * ROSS provides hooks for PE init, finalization and per GVT operations.
 */
typedef void (*pe_init_f) (tw_pe * pe);
typedef void (*pe_gvt_f) (tw_pe * pe);
typedef void (*pe_final_f) (tw_pe * pe);

/* pre_lp_init -- PE initialization routine, before LP init.
 * post_lp_init --  PE initialization routine, after LP init.
 * gvt		-- PE per GVT routine.
 * final	-- PE finilization routine.
 */
struct tw_petype_t
{
  pe_init_f pre_lp_init;
  pe_init_f post_lp_init;
  pe_gvt_f gvt;
  pe_final_f final;
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
typedef void (*event_f) (void *sv, tw_bf * cv, void *msg, tw_lp * me);
typedef void (*revent_f) (void *sv, tw_bf * cv, void *msg, tw_lp * me);
typedef void (*final_f) (void *sv, tw_lp * me);
typedef void (*statecp_f) (void *sv_dest, void *sv_src);

/*
 *  init        -- LP setup routine.
 *  map		-- LP mapping of LP gid -> remote PE routine.
 *  event       -- LP event handler routine.
 *  revent      -- LP RC event handler routine.
 *  final       -- LP final handler routine.
 *  statecp     -- LP SV copy routine.
 *  state_sz    -- Number of bytes that SV is for the LP.
 */
struct tw_lptype_t
{
  init_f init;
  event_f event;
  revent_f revent;
  final_f final;
  map_f map;
  size_t state_sz;
};

struct tw_statistics_t
{
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

  tw_clock s_total;
  tw_clock s_net_read;
  tw_clock s_gvt;
  tw_clock s_fossil_collect;

  tw_clock s_event_abort;
  tw_clock s_event_process;
  tw_clock s_pq;
  tw_clock s_rollback;

  tw_clock s_cancel_q;
};

#ifdef ROSS_MEMORY
struct tw_memoryq_t
{
  tw_memory	*head;
  tw_memory	*tail;

  size_t		 size;
  size_t		 start_size;
  size_t		 d_size;

  tw_stime	 grow;
};

/*
 * tw_memory
 * 
 * This is a memory buffer which applications can use in any way they
 * see fit.  ROSS provides API methods for handling memory buffers in the event of
 * a rollback and manages the memory in an efficient way, ie, like events.
 */
struct tw_memory_t
{
  /*
   * next		-- Next pointer for all queues except the LP RC queue
   * prev		-- Prev pointer for all queues except the LP RC queue
   *
   * up		-- Up pointer for storing membufs in splay tree
   * heap_index	-- index for storing membufs in heap queue
   *
   * ts		-- time at which this event can be fossil collected
   * fd		-- source memory queue index
   * nrefs	-- number of references to this membuf (for forwarding)
   */
  tw_memory	*next;
  tw_memory	*prev;

  //tw_memory	*volatile up;
  //int		 heap_index;

  tw_stime         ts;
  tw_fd		 fd;
  unsigned int	 nrefs;
};
#endif

struct tw_eventq_t
{
  size_t size;
  tw_event *head;
  tw_event *tail;
};

/*
 * Some applications find it handy to have this bitfield when doing
 * reverse computation.  So we follow GTW tradition and provide it.
 */
struct tw_bf_t
{
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

/*
 * tw_lp_state:
 *
 * Used to make a list of LP state vectors.  The entire state
 * is usually going to always be larger than this object, but
 * the minimum size is this object.  When the state is a
 * tw_lp_state it does NOT hold any application data, so we
 * can overwrite it with our own safely.
 */
struct tw_lp_state_t
{
  tw_lp_state    *next;
};


enum tw_event_owner_t
{
  TW_pe_event_q = 1,	/* In a tw_pe.event_q list */
    TW_pe_pq = 2,		/* In a tw_pe.pq */
    TW_kp_pevent_q = 3,     /* In a tw_kp.pevent_q */
    TW_pe_anti_msg = 4,     /* Anti-message */
    TW_net_outq = 5,        /* Pending network transmission */
    TW_net_asend = 6,       /* Network transmission in progress */
    TW_net_acancel = 7,     /* Network transmission in progress */
    TW_pe_sevent_q = 8,     /* In tw_pe.sevent_q */
    TW_pe_free_q = 9        /* In tw_pe.free_q */
    };

/*
 * tw_event:
 *
 * Holds entire event structure, one is created for each and every
 * event in use.
 */
struct tw_event_t
{
  tw_event *next;
  tw_event *prev;
#ifdef ROSS_QUEUE_splay
  tw_event *up;
#endif
#ifdef ROSS_QUEUE_heap
  unsigned long heap_index;
#endif

  /* cancel_next  -- Next event in the cancel queue for the dest_pe.
   * caused_by_me -- Start of event list caused by this event.
   * cause_next   -- Next in parent's caused_by_me chain.
   */
  tw_event *cancel_next;
  tw_event *caused_by_me;
  tw_event *cause_next;

  tw_eventid	 event_id;

  /* Status of the event's queue location(s). */
  struct
  {
    BIT_GROUP(

	      /* Owner of the next/prev pointers; see tw_event_owner */
	      BIT_GROUP_ITEM(owner, 4)

	      /* Actively on a dest_lp->pe's cancel_q */
	      BIT_GROUP_ITEM(cancel_q, 1)
	      BIT_GROUP_ITEM(cancel_asend, 1)

	      /* Indicates union addr is in 'remote' storage */
	      BIT_GROUP_ITEM(remote, 1)
	      BIT_GROUP_ITEM(__pad, 1)
	      )
  } state;

  /* cv -- Used by app during reverse computation.
   * lp_state -- dest_lp->state BEFORE this event.
   */
  tw_bf		 cv;
  //void		*lp_state;

  /* dest_lp -- Destination LP object.
   * src_lp -- Sending LP.
   * recv_ts -- Actual time to be received.
   * event_id -- Unique id assigned by src_lp->pe if remote.
   */
  tw_lp		*dest_lp;
  tw_lp		*src_lp;
  tw_stime	 recv_ts;

  tw_peid		 send_pe;

#ifdef ROSS_MEMORY
  tw_memory	*memory;
#endif
};

/*
 * tw_lp:
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
struct tw_lp_t
{
  // local LP id
  tw_lpid id;

  // global LP id
  tw_lpid gid;

  tw_pe *pe;

  /* kp -- Kernel process that we belong to (must match pe).
   * pe_next  -- Next LP in the PE's service list.
   */
  tw_kp *kp;

  /* cur_state	-- Current application LP data.
   * state_qh	-- Head of [free] state queue (for state saving).
   * rng		-- RNG stream array for this LP
   * type		-- Type of this LP, including service callbacks.
   */
  void		*cur_state;
  tw_lp_state	*state_qh;
  tw_lptype	 type;
  tw_rng_stream	*rng;
};

/*
 * tw_kp:
 *
 * Holds our state for the Kernel Process (KP), which consists only of
 * processed event list for a collection of LPs.  
 */
struct tw_kp_t
{
  /* id -- ID number, otherwise its not available to the app.
   * pe -- PE that services this KP.
   * next -- Next KP in the PE's service list.
   */
  tw_kpid id;
  tw_pe *pe;
  tw_kp *next;

  /* pevent_q -- Events processed by LPs bound to this KP
   * last_time -- Time of the current event being processed.
   */
  tw_eventq pevent_q;
  tw_stime last_time;

  /* s_nevent_processed -- Number of events processed.
   * s_e_rbs -- Number of events rolled back by this LP.
   * s_rb_total -- Number of total rollbacks by this LP.
   * s_rb_secondary -- Number of secondary rollbacks by this LP.
   */
  tw_stat s_nevent_processed;

#if 0
  tw_stat s_e_rbs;
  tw_stat s_rb_total;
  tw_stat s_rb_secondary;
#endif

  long s_e_rbs;
  long s_rb_total;
  long s_rb_secondary;

  long long test;

  /*
   * queues -- TW processed memory buffer queues
   */
#ifdef ROSS_MEMORY
  tw_memoryq	*pmemory_q;
#endif
};

/*
 * tw_pe
 *
 * Holds the entire PE state.  
 */
struct tw_pe_t
{
  tw_peid id;
  tw_node	node;

  /* type -- Model defined PE type routines.
   */
  tw_petype type;

  /* event_q -- Linked list of events sent to this PE.
   * cancel_q -- List of canceled events.
   * event_q_lck -- processor specific lock for this PE's event_q.
   * cancel_q_lck -- processor specific lock for this PE's cancel_q.
   * pq -- Priority queue used to sort events.
   * pe_next -- Single linked list of PE structs.
   * rollback_q -- List of KPs actively rolling back.
   */
  tw_eventq event_q;
  tw_event *cancel_q;
  tw_pq *pq;
  tw_kp *kp_list;
  tw_pe **pe_next;

  /* free_q -- Linked list of free tw_events.
   * abort_event -- Placeholder event for when free_q is empty.
   * cur_event -- Current event being processed.
   * sevent_q -- events already sent over the network.
   * memory_q -- array of free tw_memory buffers linked lists
   */
  tw_eventq free_q;
  tw_event *abort_event;
  tw_event *cur_event;
  tw_eventq sevent_q;
#ifdef ROSS_MEMORY
  tw_memoryq *memory_q;
#endif

  /* clock_offset -- Initial clock value for this PE.
   * clock_time -- Most recent clock value for this PE.
   */
  tw_clock clock_offset;
  tw_clock clock_time;

  /* cev_abort	-- Current event being processed must be aborted.
   * master	-- Master across all compute nodes.
   * local_master -- Master for this node.
   * gvt_status	-- bits available for gvt computation.
   */
  BIT_GROUP(
	    BIT_GROUP_ITEM(__pad, 1)
	    BIT_GROUP_ITEM(cev_abort, 1)
	    BIT_GROUP_ITEM(master, 1)
	    BIT_GROUP_ITEM(local_master, 1)
	    BIT_GROUP_ITEM(gvt_status, 4)
	    )

    /* trans_msg_ts -- Last transient messages' time stamp.
     * GVT -- global virtual time
     * LVT -- local (to PE) virtual time
     */
    tw_stime trans_msg_ts;
  tw_stime GVT;
  tw_stime GVT_prev;
  tw_stime LVT;

#ifdef ROSS_GVT_mpi_allreduce
  tw_stat s_nwhite_sent;
  tw_stat s_nwhite_recv;
#endif

  /* start_time -- When this PE first started execution.
   * end_time -- When this PE finished its execution.
   */
  tw_wtime start_time;
  tw_wtime end_time;

  /* stats	-- per PE counters
   */
  tw_statistics		stats;

#ifndef ROSS_NETWORK_none
  /*
   * hash_t  -- array of incoming events from remote pes
   *            Note: only necessary for distributed DSR
   * seq_num  -- array of remote send counters for hashing on
   *                 size == g_tw_npe
   */
  void           *hash_t;
#ifdef ROSS_NETWORK_mpi
  tw_eventid	 seq_num;
#else
  tw_eventid	*seq_num;
#endif
#endif

  /*
   * rng  -- pointer to the random number generator on this PE
   */
  tw_rng  *rng;
};

struct tw_log_t
{
  struct
  {
    unsigned int
    rollback_primary:1,
      rollback_secondary:1,
      rollback_abort:1,
      send_event:1,
      recv_event:1,
      eventq_delete:1,
      send_cancel:1,
      recv_cancel:1,
      pq_enq:1,
      pq_deq:1,
      pq_delete:1,
      freeq_enq:1,
      freeq_deq:1,
      processed_enq:1,
      processed_deq:1;
  }
  state;

  tw_event        event;
  tw_event       *e;
  tw_log         *next;
  unsigned long long log_sz;
};

#define TW_MHZ 1000000
#endif
