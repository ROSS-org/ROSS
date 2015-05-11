#ifndef INC_ross_extern_h
#define	INC_ross_extern_h

extern void	tw_rand_init_streams(tw_lp * lp, unsigned int nstreams);

/*
 * tw-stats.c
 */
extern void tw_stats(tw_pe * me);

/*
 * ross-global.c
 */
extern tw_synch g_tw_synchronization_protocol;
extern map_local_f g_tw_custom_lp_global_to_local_map;
extern map_custom_f g_tw_custom_initial_mapping;
extern tw_lp_map g_tw_mapping;
extern tw_lpid  g_tw_nlp;
extern tw_lpid	g_tw_lp_offset;
extern tw_kpid  g_tw_nkp;
extern tw_lp	**g_tw_lp;
extern tw_kp	**g_tw_kp;
extern int      g_tw_fossil_attempts;
extern unsigned int	g_tw_nRNG_per_lp;
extern tw_lpid		g_tw_rng_default;
extern tw_seed		*g_tw_rng_seed;
extern unsigned int	g_tw_mblock;
extern unsigned int g_tw_gvt_interval;
extern tw_stime		g_tw_ts_end;
extern unsigned int	g_tw_sim_started;
extern size_t		g_tw_msg_sz;
extern size_t		g_tw_event_msg_sz;
extern size_t       g_tw_delta_sz;
extern uint32_t     g_tw_buddy_alloc;
extern buddy_list_bucket_t *g_tw_buddy_master;
extern uint32_t		g_tw_avl_node_count;

extern unsigned int	g_tw_memory_nqueues;
extern size_t		g_tw_memory_sz;

extern tw_stime         g_tw_lookahead;
extern tw_stime         g_tw_min_detected_offset;

extern tw_peid  g_tw_npe;
extern tw_pe **g_tw_pe;
extern unsigned int      g_tw_events_per_pe;
extern unsigned int      g_tw_events_per_pe_extra;

extern unsigned int	    g_tw_gvt_threshold;
extern unsigned int	    g_tw_gvt_done;

extern unsigned int	g_tw_net_device_size;
extern tw_node g_tw_mynode;
extern tw_node g_tw_masternode;

extern FILE		*g_tw_csv;

extern tw_lptype * g_tw_lp_types;
extern tw_typemap_f g_tw_lp_typemap;

        /*
	 * Cycle Counter variables
	 */

extern tw_clock g_tw_cycles_gvt;
extern tw_clock g_tw_cycles_ev_abort;
extern tw_clock g_tw_cycles_ev_proc;
extern tw_clock g_tw_cycles_ev_queue;
extern tw_clock g_tw_cycles_rbs;
extern tw_clock g_tw_cycles_cancel;

/*
 * clock-*
 */
extern const tw_optdef *tw_clock_setup();
extern void tw_clock_init(tw_pe * me);
extern tw_clock tw_clock_now(tw_pe * me);
extern tw_clock tw_clock_read();
extern tw_stime g_tw_clock_rate;

/*
 * tw-event.c
 */
extern void		 tw_event_send(tw_event * event);
extern void		 tw_event_rollback(tw_event * event);

/*
 * ross-inline.h
 */
static inline void  tw_event_free(tw_pe *, tw_event *);
static inline void  tw_free_output_messages(tw_event *e, int print_message);

/*
 * tw-lp.c
 */
extern tw_lp	*tw_lp_next_onpe(tw_lp * last, tw_pe * pe);
extern void		 tw_lp_settype(tw_lpid lp, tw_lptype * type);
extern void		 tw_lp_onpe(tw_lpid index, tw_pe * pe, tw_lpid id);
extern void		 tw_lp_onkp(tw_lp * lp, tw_kp * kp);
extern void		 tw_init_lps(tw_pe * me);
extern void tw_pre_run_lps(tw_pe * me);
extern void tw_lp_setup_types();

/*
 * tw-kp.c
 */
extern void     tw_kp_onpe(tw_kpid id, tw_pe * pe);
extern void     kp_fossil_remote(tw_kp * kp);
extern tw_kp*   tw_kp_next_onpe(tw_kp * last, tw_pe * pe);
extern void     tw_init_kps(tw_pe * me);
extern tw_out*  tw_kp_grab_output_buffer(tw_kp *kp);
extern void     tw_kp_put_back_output_buffer(tw_out *out);

extern void	 tw_kp_rollback_event(tw_event *event);
extern void	 tw_kp_rollback_to(tw_kp * kp, tw_stime to);
extern void	 tw_kp_fossil_memoryq(tw_kp * me, tw_fd);
extern void	 tw_kp_fossil_memory(tw_kp * me);

/*
 * tw-pe.c
 */
extern tw_pe		*tw_pe_next(tw_pe * last);
extern void		 tw_pe_settype(tw_pe *, const tw_petype * type);
extern void		 tw_pe_create(tw_peid npe);
extern void		 tw_pe_init(tw_peid id, tw_peid global);
extern void		 tw_pe_fossil_collect(tw_pe * me);
extern tw_fd		 tw_pe_memory_init(tw_pe * pe, size_t n_mem,
					   size_t d_sz, tw_stime mult);

/*
 * tw-setup.c
 */
extern void tw_init(int *argc, char ***argv);
extern void tw_define_lps(tw_lpid nlp, size_t msg_sz, tw_seed * seed);
extern void tw_run(void);
extern void tw_end(void);
extern tw_lpid map_onetype (tw_lpid gid);

/*
 * tw-sched.c
 */
extern void tw_sched_init(tw_pe * me);
extern void tw_scheduler_sequential(tw_pe * me);
extern void tw_scheduler_conservative(tw_pe * me);
extern void tw_scheduler_optimistic(tw_pe * me);
extern void tw_scheduler_optimistic_debug(tw_pe * me);
extern void tw_scheduler_sequential_omnet(tw_pe * me);
extern void tw_scheduler_conservative_omnet(tw_pe * me);

/*
 * tw-signal.c
 */
extern void     tw_sigsegv(int sig);
extern void     tw_sigterm(int sig);

/*
 * tw-state.c
 */
extern void tw_snapshot(tw_lp *lp, size_t state_sz);
extern long tw_snapshot_delta(tw_lp *lp, size_t state_sz);
extern void tw_snapshot_restore(tw_lp *lp, size_t state_sz, void *buffer, size_t delta_size);

/*
 * tw-timing.c
 */
extern   void     tw_wall_now(tw_wtime * t);
extern   void     tw_wall_sub(tw_wtime * r, tw_wtime * a, tw_wtime * b);
extern   double   tw_wall_to_double(tw_wtime * t);

/*
 * tw-memory.c
 */
#ifdef ROSS_MEMORY
extern size_t	tw_memory_allocate(tw_memoryq *);
#endif

/*
 * tw-util.c
 */

#define	TW_LOC	__FILE__,__LINE__
extern int tw_output(tw_lp *lp, const char *fmt, ...);
extern void tw_error(const char *file, int line, const char *fmt, ...) NORETURN;
extern void tw_printf(const char *file, int line, const char *fmt, ...);
extern void tw_calloc_stats(size_t *alloc, size_t *waste);
extern void* tw_calloc(const char *file, int line, const char *for_who, size_t e_sz, size_t n);

#endif
