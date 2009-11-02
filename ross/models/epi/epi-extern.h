#ifndef INC_epi_extern_h
#define INC_epi_extern_h

extern epi_agent * ga;

/*
 * epi-report.c
 */
extern void epi_report_census_tract(int today);
extern void epi_report_hospitals(int today);

/*
 * epi-num.c
 */
extern void epi_num_add(epi_state * state, tw_bf * bf, epi_agent * a, tw_lp * lp);
extern void epi_num_remove(epi_state * state, tw_bf * bf, epi_agent * a, tw_lp * lp);

/*
 * epi-timer.c
 */
extern void	 epi_location_timer(epi_state *, tw_lp * lp);

/*
 * queue-*.c
 */
extern void     *pq_create(void);
extern void      pq_enqueue(void *pq, tw_memory *);
extern void	*pq_dequeue(void *pq);
extern void	*pq_top(void *pq);
extern void	*pq_next(void *pq, int next);
extern tw_stime  pq_minimum(void *pq);
extern void      pq_delete_any(void *pq, tw_memory **);
extern unsigned  int pq_get_size(void *q);
extern void	 pq_cleanup(void * h);
extern unsigned  int pq_max_size(void *q);

/*
 * epi-seir.c - compute SEIR model for LP location
 */
extern void epi_seir_compute(epi_state * state, tw_bf * bf, int, tw_lp * lp);

/*
 * epi-agent.c - structural agent functions, including agent finite state machine
 */
extern void epi_agent_update_ts(epi_state *, tw_bf *, epi_agent * a, tw_lp * lp);
extern void epi_agent_send(tw_lp * src, tw_memory * buf, tw_stime offset, tw_lpid);
extern epi_ic_stage *epi_agent_infected(epi_agent * a, int disease);
extern epi_agent *epi_getagent(int id);
extern void epi_agent_add(epi_state *, tw_bf * bf, tw_memory *, tw_lp * lp);
extern void epi_agent_remove(epi_state *, tw_bf * bf, tw_lp * lp);
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
extern void epi_md_final();
extern tw_peid epi_map(tw_lpid gid);
extern void epi_init(epi_state *state, tw_lp * lp);
extern void epi_event_handler(epi_state *state, tw_bf * cv, epi_message *message, tw_lp *lp);
extern void epi_rc_event_handler(epi_state *, tw_bf * cv, epi_message *message, tw_lp * lp);
extern void epi_final(epi_state *state, tw_lp * lp);

/*
 * epi-stats.c - stats functions
 */
extern void epi_stats_print();

/* 
 * epi-global.c
 */
extern unsigned int	 g_epi_complete;

extern unsigned int	 g_epi_nagents;
extern tw_memoryq	*g_epi_agents;

extern void		**g_epi_pq;

extern unsigned int	**g_epi_hospital_wws;
extern unsigned int	**g_epi_hospital_ww;
extern unsigned int	**g_epi_hospital;
extern unsigned int	  g_epi_nhospital;

extern unsigned int	*g_epi_sick_init_pop;
extern unsigned int	 g_epi_wws_init_pop;
extern unsigned int	 g_epi_ww_init_pop;
extern unsigned int	*g_epi_exposed_today;

extern tw_stime		 g_epi_mean;
extern double		 g_epi_sick_rate;
extern double		 g_epi_ww_rate;
extern double		 g_epi_wws_rate;

extern double		 g_epi_ww_threshold;
extern unsigned int	 g_epi_ww_duration;

extern unsigned int	 g_epi_ndiseases;
extern epi_disease	*g_epi_diseases;

//extern unsigned int	 g_epi_nstages;
//extern epi_ic_stage	*g_epi_stages;

extern char		 g_epi_ic_fn[1024];
extern char		 g_epi_agent_fn[1024];
extern char		 g_epi_position_fn[1024];
extern char		 g_epi_log_fn[1024];
extern char		 g_epi_hospital_fn[1024];

extern FILE		*g_epi_position_f;
extern FILE	       **g_epi_log_f;
extern FILE		*g_epi_hospital_f;

extern epi_statistics 	*g_epi_stats;
extern unsigned int	***g_epi_regions;
extern unsigned int	 g_epi_nregions;
extern unsigned int	 g_epi_day;
extern int		 g_epi_mod;

extern tw_fd		 g_epi_fd;
extern tw_fd		 g_epi_pathogen_fd;

extern double		 EPSILON;

#endif
