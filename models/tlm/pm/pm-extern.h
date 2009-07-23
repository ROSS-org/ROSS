#ifndef INC_pm_extern_h
#define INC_pm_extern_h
extern int the_one;

/*
 * pm-pe.c - model per PE functions
 */
extern void pm_pe_init(tw_pe * me);
extern void pm_pe_post_init(tw_pe * me);
extern void pm_pe_gvt(tw_pe * me);
extern void pm_pe_final(tw_pe * me);

/*
 * pm-init.c - init the model components
 */
extern void pm_init_scenario();

/*
 * pm.c - main model file with simulation engine function definitions and main
 */
extern void pm_init(pm_state *, tw_lp *);
extern void pm_event_handler(pm_state *, tw_bf *, pm_message *, tw_lp *);
extern void pm_rc_event_handler(pm_state *, tw_bf *, pm_message *, tw_lp *);
extern void pm_final(pm_state *, tw_lp *);

/*
 * pm-stats.c - stats functions
 */
extern void pm_stats_print();

#if 0
/*
 * pm-connection.c
 */
extern void pm_connection_print(pm_state * state, pm_connection * c, tw_lp * lp);

/*
 * pm-pathloss.c
 */
extern double pm_pathloss_eff(pm_state * state, double d);

/*
 * pm-topo-square.c - compute positions on terrain in the shape of a square
 */
extern void pm_square_newlocation(pm_state * state, tw_lp * lp);
extern void square_segment_intersect(double *x1, double *y1, double *x2, double *y2, double *xdouble, double *ydouble);

#endif

/*
 * pm-node.c - link level helper functions
 */
extern pm_connection *pm_connect(pm_state *, tw_bf *, pm_message *, tw_lp *, int blk);
extern pm_connection *pm_disconnect(pm_state*, tw_bf*, pm_message*, tw_lp *, int);
extern void pm_move(pm_state * state, pm_message * m, tw_bf * bf, tw_lp *);
extern void pm_rc_move(pm_state *, pm_message *, tw_bf *, tw_lp *);

/*
 * pm-helper.c - helper functions
 */
extern double pm_distance(double x0, double y0, double x1, double y1);
extern double pm_intercept(double x, double y, double slope);
extern double pm_slope(double x0, double y0, double x1, double y1);
extern void pm_sort(double * p);

/* 
 * pm-global.c
 */
extern unsigned int	 g_pm_dimensions;
extern unsigned int	 g_pm_nnodes;
extern double 		 g_pm_runtime;

extern pm_statistics 	*g_pm_stats;
extern int 	 	 g_pm_plot;

extern FILE		*g_pm_position_f;
extern unsigned int	*g_pm_position_x;
extern unsigned int	*g_pm_position_y;
extern unsigned int	*g_pm_ids;

extern unsigned int	 g_pm_mu;
extern unsigned int	 g_pm_distr_sd;

extern tw_stime		 percent_wave;
extern char		 run_id[1024];

extern double	 grid[3];
extern double	 spacing[3];

#endif
