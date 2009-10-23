#ifndef INC_rw_extern_h
#define INC_rw_extern_h

/*
 * rw-xml.c
 */
extern void rw_xml(rw_state *, const xmlNodePtr node, tw_lp *);

/*
 * rw-model.c
 */
extern void rw_md_opts();
extern void rw_md_init(int argc, char ** argv, char ** env);
extern void rw_md_final();

/*
 * rw.c - main model file with simulation engine function definitions and main
 */
extern int rw_main(int argc, char **argv, char **env);
extern void rw_init(rw_state *, tw_lp *);
extern void rw_event_handler(rw_state *, tw_bf *, rn_message *, tw_lp *);
extern void rw_rc_event_handler(rw_state *, tw_bf *, rn_message *, tw_lp *);
extern void rw_final(rw_state *, tw_lp *);

#if 0
/*
 * rw-connection.c
 */
extern void rw_connection_print(rw_state * state, rw_connection * c, tw_lp * lp);

/*
 * rw-pathloss.c
 */
extern double rw_pathloss_eff(rw_state * state, double d);

/*
 * rw-topo-square.c - compute positions on terrain in the shape of a square
 */
extern void rw_square_newlocation(rw_state * state, tw_lp * lp);
extern void square_segment_intersect(double *x1, double *y1, double *x2, double *y2, double *xdouble, double *ydouble);

#endif

/*
 * rw-node.c - link level helper functions
 */
extern rw_connection *rw_connect(rw_state *, tw_bf *, rw_message *, tw_lp *, int blk);
extern rw_connection *rw_disconnect(rw_state*, tw_bf*, rw_message*, tw_lp *, int);
extern void rw_move(rw_state * state, rw_message * m, tw_bf * bf, tw_lp *);
extern void rw_rc_move(rw_state *, rw_message *, tw_bf *, tw_lp *);

/*
 * rw-helper.c - helper functions
 */
extern double rw_distance(double x0, double y0, double x1, double y1);
extern double rw_intercept(double x, double y, double slope);
extern double rw_slope(double x0, double y0, double x1, double y1);
extern void rw_sort(double * p);

/* 
 * rw-global.c
 */
extern tw_fd		 g_rw_fd;

extern rw_statistics 	*g_rw_stats;
extern int 	 	 g_rw_plot;

extern FILE		*g_rw_position_f;
extern unsigned int	*g_rw_position_x;
extern unsigned int	*g_rw_position_y;
extern unsigned int	*g_rw_ids;

extern unsigned int	 g_rw_mu;
extern unsigned int	 g_rw_distr_sd;

extern tw_stime		 percent_wave;

#endif
