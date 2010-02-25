#ifndef INC_rp_extern_h
#define INC_rp_extern_h

/*
 * rw-xml.c
 */
extern void rp_xml(rp_state *, const xmlNodePtr node, tw_lp *);

/*
 * rw-model.c
 */
extern void rp_md_opts();
extern void rp_md_init(int argc, char ** argv, char ** env);
extern void rp_md_final();

/*
 * rw.c - main model file with simulation engine function definitions and main
 */
extern int rp_main(int argc, char **argv, char **env);
extern void rp_init(rp_state *, tw_lp *);
extern void rp_event_handler(rp_state *, tw_bf *, rn_message *, tw_lp *);
extern void rp_rc_event_handler(rp_state *, tw_bf *, rn_message *, tw_lp *);
extern void rp_final(rp_state *, tw_lp *);

/*
 * rp-node.c - link level helper functions
 */
extern rp_connection *rp_connect(rp_state *, tw_bf *, rp_message *, tw_lp *, int blk);
extern rp_connection *rp_disconnect(rp_state*, tw_bf*, rp_message*, tw_lp *, int);
extern void rp_move(rp_state * state, rp_message * m, tw_bf * bf, tw_lp *);
extern void rp_rc_move(rp_state *, rp_message *, tw_bf *, tw_lp *);

/*
 * rp-helper.c - helper functions
 */
extern double rp_distance(double x0, double y0, double x1, double y1);
extern double rp_intercept(double x, double y, double slope);
extern double rp_slope(double x0, double y0, double x1, double y1);
extern void rp_sort(double * p);

/* 
 * rp-global.c
 */
extern tw_fd		 g_rp_fd;

extern rp_statistics 	*g_rp_stats;
extern int 	 	 g_rp_plot;

extern FILE		*g_rp_position_f;
extern unsigned int	*g_rp_position_x;
extern unsigned int	*g_rp_position_y;
extern unsigned int	*g_rp_ids;

extern unsigned int	 g_rp_mu;
extern unsigned int	 g_rp_distr_sd;

extern tw_stime		 percent_wave;

#endif
