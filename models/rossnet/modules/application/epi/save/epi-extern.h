#ifndef INC_epi_extern_h
#define INC_epi_extern_h

extern void epi_report_census_tract();

/*
 * epi-num.c
 */
extern void epi_num_add(epi_state * state, tw_bf * bf, epi_agent * a, tw_lp * lp);
extern void epi_num_remove(epi_state * state, tw_bf * bf, epi_agent * a, tw_lp * lp);

/*
 * epi-timer.c
 */
extern void	 epi_timer_cancel(tw_event * tmr, tw_lp * lp);
extern tw_event *epi_timer_start(epi_agent * a, tw_event * t, tw_stime ts, tw_lp * lp);
extern tw_memory *epi_timer_update(epi_state * state, tw_lp * lp);

/*
 * queue-*.c
 */
extern void     *pq_create(void);
extern void      pq_enqueue(void *pq, epi_agent *);
extern void	*pq_dequeue(void *pq);
extern void	*pq_top(void *pq);
extern void	*pq_next(void *pq, int next);
extern tw_stime  pq_minimum(void *pq);
extern void      pq_delete_any(void *pq, epi_agent **);
extern unsigned int pq_get_size(void *q);
extern unsigned int pq_max_size(void *q);

/*
 * epi-seir.c - compute SEIR model for LP location
 */
extern void epi_seir_compute(epi_state * state, tw_bf * bf, epi_agent *, tw_lp * lp);
extern void epi_rc_seir_compute(epi_state * state, tw_bf * bf, epi_agent *, tw_lp * lp);

/*
 * epi-agent.c - structural agent functions, including agent finite state machine
 */
extern epi_agent *epi_getagent(int id);
extern void epi_agent_add(epi_state *, tw_bf * bf, epi_agent *, tw_lp * lp);
extern void epi_rc_agent_add(epi_state *, tw_bf * bf, epi_agent *, tw_lp * lp);
extern int  epi_agent_remove(epi_state *, tw_bf * bf, epi_agent *, tw_lp * lp);
extern void epi_rc_agent_remove(epi_state *, tw_bf * bf, epi_agent *, tw_lp * lp);
extern void epi_agent_stage_tran(epi_state *, tw_bf * bf, epi_agent *, tw_lp * lp);
extern void epi_report_census_track();

/*
 * epi-setup.c - setup command line args
 */
extern void epi_setup_options(int argc, char ** argv);

/*
 * epi-init.c - init the model components
 */
extern void epi_init_scenario();

/*
 * epi.c - main model file with simulation engine function definitions and main
 */
extern void epi_main(int argc, char ** argv, char ** env);
extern void epi_xml(epi_state * state, const xmlNodePtr node, tw_lp * lp);
extern void epi_md_final();
extern void epi_init(epi_state *state, tw_lp * lp);
extern void epi_event_handler(epi_state *state, tw_bf * cv, rn_message *message, tw_lp *lp);
extern void epi_rc_event_handler(epi_state *, tw_bf * cv, rn_message *message, tw_lp * lp);
extern void epi_final(epi_state *state, tw_lp * lp);

/*
 * epi-stats.c - stats functions
 */
extern void epi_stats_print();

/* 
 * epi-global.c
 */
extern double		 g_epi_psick;
extern double		 g_epi_work_while_sick_p;
extern unsigned int	 g_epi_nagents;
extern unsigned int	 g_epi_nsick;

extern unsigned int	 g_epi_nstages;
extern epi_ic_stage	*g_epi_stages;

extern char		*g_epi_ic_fn;
extern char		*g_epi_position_fn;
extern char		*g_epi_log_fn;

extern FILE		*g_epi_position_f;
extern FILE		*g_epi_log_f;

extern epi_statistics 	*g_epi_stats;
extern unsigned int	**g_epi_ct;
extern unsigned int	 g_epi_nct;
extern unsigned int	 g_epi_day;
extern double		 g_epi_worried_well_rate;
extern double		 g_epi_worried_well_threshold;
extern unsigned int	 g_epi_worried_well_duration;

extern tw_fd		 g_epi_fd;

extern double		 EPSILON;

#endif
