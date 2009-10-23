#ifndef INC_num_extern_h
#define INC_num_extern_h

/*
 * num.c
 */
extern void num_init(num_state * state, tw_lp * lp);
extern void num_event_handler(num_state * state, tw_bf *, rn_message *, tw_lp * lp);
extern void num_rc_event_handler(num_state *, tw_bf *, rn_message *, tw_lp * lp);
extern void num_final(num_state * state, tw_lp * lp);
extern void num_setup_options(int argc, char ** argv);

/*
 * num-file.c
 */
extern void num_file_start(num_state * state, tw_bf * bf, unsigned int size, tw_lp * lp);
extern void num_file_stop(num_state * state, tw_bf * bf, tw_lp * lp);
extern void num_file_complete(num_state * state, tw_bf * bf, rn_message * msg, tw_lp * lp);

/*
 * num-report.c
 */
extern void num_report_levels(int);
extern void num_print();

/*
 * num-init.c
 */
extern void num_init_scenario();

/*
 * num-helper.c
 */
extern char	*get_tod(tw_stime);

/*
 * num-main.c
 */
extern void num_main(int argc, char ** argv, char ** env);
extern void num_xml(num_state * state, const xmlNodePtr node, tw_lp * lp);
extern void num_md_final();

/* 
 * num-global.c
 */
extern unsigned int	 *g_num_net_pop;
extern unsigned int	  g_num_max_users;
extern num_statistics 	 *g_num_stats;
extern unsigned int	**g_num_level;
extern unsigned int	 *g_num_profile_cnts;
extern int		  g_num_mod;
extern int		  g_num_day;
extern int		  g_num_debug;
extern unsigned int	  g_num_debug_lp;
extern unsigned int	  g_num_nprofiles;
extern num_profile	 *g_num_profiles;
extern char		 *g_num_profiles_fn;
extern char		 *g_num_log_fn;

extern FILE		 *g_num_log_f;

/*
 * num-timer.c
 */
extern void num_timer_cancel(tw_event * timer, tw_lp * lp);
extern tw_event * num_timer_start(tw_event * timer, tw_stime ts, tw_lp * lp);

#endif
