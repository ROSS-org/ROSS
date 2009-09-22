#ifndef INC_ross_extern_h
#define	INC_ross_extern_h

extern void	tw_rand_init_streams(tw_lp * lp, unsigned int nstreams);

/*
 * tw-stats.c
 */
extern void tw_stats(tw_pe * me);

/*
 * queue-*.c
 */
extern tw_pq *tw_pq_create(void);
extern void tw_pq_enqueue(tw_pq *, tw_event *);
extern tw_event *tw_pq_dequeue(tw_pq *);
extern tw_stime tw_pq_minimum(tw_pq *);
extern void tw_pq_delete_any(tw_pq *, tw_event *);
extern unsigned int tw_pq_get_size(tw_pq *);
extern unsigned int tw_pq_max_size(tw_pq *);

/*
 * tw-timer.c
 */

/*
 * ross-global.c
 */
extern tw_lpid  g_tw_nlp;
extern tw_lpid	g_tw_lp_offset;
extern tw_kpid  g_tw_nkp;
extern tw_lp	**g_tw_lp;
extern tw_kp	**g_tw_kp;
extern int      g_tw_sv_growcnt;
extern int      g_tw_fossil_attempts;
extern unsigned int	g_tw_nRNG_per_lp;
extern tw_lpid		g_tw_rng_default;
extern size_t		g_tw_rng_max;
extern tw_seed		*g_tw_rng_seed;
extern unsigned int	g_tw_mblock;
extern tw_stime		g_tw_ts_end;
extern unsigned int	g_tw_sim_started;
extern unsigned int	g_tw_master;
extern size_t		g_tw_msg_sz;
extern size_t		g_tw_event_msg_sz;

extern unsigned int	g_tw_memory_nqueues;
extern size_t		g_tw_memory_sz;

extern tw_peid  g_tw_npe;
extern tw_pe **g_tw_pe;
extern int      g_tw_events_per_pe;

extern tw_barrier g_tw_simstart;
extern tw_barrier g_tw_simend;
extern tw_barrier g_tw_network;
extern tw_barrier g_tw_gvt_b;

extern tw_mutex   g_tw_debug_lck;
extern tw_mutex   g_tw_event_q_lck;
extern tw_mutex   g_tw_cancel_q_lck;
extern tw_mutex   g_tw_cancel_q_sz_lck;

extern unsigned int	    g_tw_gvt_threshold;
extern unsigned int	    g_tw_gvt_done;

extern unsigned int	g_tw_net_device_size;
extern tw_node g_tw_mynode;
extern tw_node g_tw_masternode;

extern FILE		*g_tw_csv;

/*
 * clock-*
 */
extern void tw_clock_init(tw_pe * me);
extern tw_clock tw_clock_now(tw_pe * me);

/*
 * signal-*.c
 */
extern void     tw_register_signals(void);

/*
 * tw-event.c
 */
extern void		 tw_event_send(tw_event * event);
extern void		 tw_event_rollback(tw_event * event);

/*
 * ross-inline.h
 */
INLINE(void)		 tw_event_free(tw_pe *, tw_event *);
INLINE(void)		*tw_event_data(tw_event * event);
INLINE(void *)		 tw_memory_data(tw_memory * m);
INLINE(tw_event *)	 tw_event_new(tw_lpid, tw_stime, tw_lp *);
INLINE(tw_pe *)		 tw_getpe(tw_peid id);

/*
 * tw-lp.c
 */
extern tw_lp		*tw_lp_next_onpe(tw_lp * last, tw_pe * pe);
extern void		 tw_lp_settype(tw_lpid lp, const tw_lptype * type);
extern void		 tw_lp_onpe(tw_lpid index, tw_pe * pe, tw_lpid id);
extern void		 tw_lp_onkp(tw_lp * lp, tw_kp * kp);
extern void		 tw_init_lps(tw_pe * me);

/*
 * tw-kp.c
 */
extern void	 tw_kp_onpe(tw_lpid id, tw_pe * pe);
extern void	 kp_fossil_remote(tw_kp * kp);
extern tw_kp	*tw_kp_next_onpe(tw_kp * last, tw_pe * pe);
extern void	 tw_init_kps(tw_pe * me);

extern void	 tw_kp_rollback_event(tw_event *event);
extern void	 tw_kp_rollback_to(tw_kp * kp, tw_stime to);
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

/*
 * tw-sched.c
 */
extern void tw_scheduler_seq(tw_pe * me);
extern void tw_scheduler(tw_pe * me);

/*
 * tw-signal.c
 */
extern void     tw_sigsegv(int sig);
extern void     tw_sigterm(int sig);

/*
 * tw-state.c
 */
extern void tw_state_save(tw_lp * lp, tw_event * cevent);
extern void tw_state_rollback(tw_lp * lp, tw_event * revent);
extern void tw_state_alloc(tw_lp * lp, int nvect);

/*
 * tw-timing.c
 */
extern   void     tw_wall_now(tw_wtime * t);
extern   void     tw_wall_sub(tw_wtime * r, tw_wtime * a, tw_wtime * b);
extern   double   tw_wall_to_double(tw_wtime * t);

/*
 * tw-memory.c
 */
extern size_t tw_memory_getsize(tw_pe * pe, int fd);
extern size_t tw_memory_allocate(tw_memoryq *);
extern tw_memory *tw_memory_alloc(tw_lp *lp, tw_fd fd);
extern void    tw_memory_free(tw_lp *lp, tw_memory *m, tw_fd fd);
extern void    tw_memory_alloc_rc(tw_lp *lp, tw_memory *head, tw_fd fd);
extern tw_memory *tw_memory_free_rc(tw_lp *lp, tw_fd fd);
extern void tw_memory_free_single(tw_pe * pe, tw_memory * m, tw_fd fd);
extern tw_fd tw_memory_init(size_t n_mem, size_t d_sz, tw_stime mult);

/*
 * tw-util.c
 */

#define	TW_LOC	__FILE__,__LINE__
extern void tw_error(
	const char *file,
	int line,
	const char *fmt,
	...) NORETURN;
extern void tw_printf(
	const char *file,
	int line,
	const char *fmt,
	...);
extern void tw_exit(int rv);
extern void tw_calloc_stats(size_t *alloc, size_t *waste);
extern void* tw_calloc(
	const char *file,
	int line,
	const char *for_who,
	size_t e_sz,
	size_t n);
extern void* tw_unsafe_realloc(
	const char *file,
	int line,
	const char *for_who,
	void *addr,
	size_t len);

#endif
