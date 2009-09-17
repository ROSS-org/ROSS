#ifndef INC_rm_extern_h
#define INC_rm_extern_h

/*
 * rm-pe.c - model per PE functions
 */
extern void rm_pe_init(tw_pe * me);
extern void rm_pe_post_init(tw_pe * me);
extern void rm_pe_gvt(tw_pe * me);
extern void rm_pe_final(tw_pe * me);

/*
 * rm-cell.c - perform wave propagation between cells
 */
extern void rm_cell_handler(rm_state *, tw_bf * bf, rm_message * m, tw_lp * lp);
extern void rm_cell_gather(rm_state *, tw_bf * bf, rm_message *, tw_lp * lp);
extern void rm_cell_scatter(rm_state *, rm_message *, tw_bf * bf, tw_lp * lp);

/*
 * rc-rm-cell.c - perform wave propagation reverse computation 
 */
extern void rm_rc_cell_handler(rm_state *, tw_bf * bf, rm_message * m, tw_lp * lp);
extern void rm_rc_scatter(rm_state *, tw_bf * bf, rm_message *, tw_lp *);
extern void rm_rc_gather(rm_state *, tw_bf * bf, rm_message *, tw_lp *);

/*
 * rm-timer.c - RM Wrapper for ROSS timer library
 */
extern tw_event	*rm_timer_cancel(tw_event * t, tw_lp * lp);
extern tw_event	*rm_timer_start(tw_event * t, int dim, tw_stime ts, tw_lp * lp);

/*
 * rm-interface.c: implement Reactive Model (rm) interface
 */
extern void rm_init(int * argc, char *** argv);
extern void rm_setup_options(int argc, char ** argv);
extern void rm_initialize_terrain(double * grid, double * spacing, int);
extern int rm_grid_initialize(char * filename);
extern void rm_wave_initiate(double * position, double signal, tw_lp * lp);
extern void rm_rc_wave_initiate(double * position, double signal);
extern double rm_move(double, double [], double [], tw_lp * lp);
extern double rm_move2(double, double [], double [], tw_lp * lp);
extern void rm_rc_move(tw_lp * lp);
extern int rm_initialize(tw_petype *, tw_lptype *, tw_peid, tw_kpid, tw_lpid, size_t);
extern void rm_run(char **argv);
extern void rm_end(void);

/*
 * rm.c - Cell LP file with simulation engine function definitions
 */
extern tw_peid _rm_map(tw_lpid gid);
extern void _rm_init(rm_state *, tw_lp * lp);
extern void _rm_event_handler(rm_state *, tw_bf * , rm_message *, tw_lp *lp);
extern void _rm_rc_event_handler(rm_state *, tw_bf *, rm_message *, tw_lp * lp);
extern void _rm_final(rm_state *, tw_lp * lp);

/*
 * rm-particle.c - particle functions
 */
extern tw_event	*rm_particle_send(tw_lpid gid, tw_stime, tw_lp *);
extern void rm_particle_handler(rm_state *, tw_bf * bf, rm_message * m, tw_lp * lp);
extern void rm_rc_particle_handler(rm_state *, tw_bf *, rm_message * m, tw_lp * lp);

/*
 * rm-proximity.c: proximity detection related functions
 */
extern void rm_proximity_send(rm_state *, tw_memory * p, tw_memory * b, tw_lp * lp);

/*
 * rm-helper.c: inlined helper functions
 */
extern double	 rm_getelevation(double * p);
extern tw_lpid	 rm_getcell(double * position);
extern double	*rm_getlocation(tw_lp * lp);

/*
 * rm-global.c: global variables
 */
extern FILE		*g_rm_waves_plt_f;
extern FILE		*g_rm_nodes_plt_f;
extern FILE		*g_rm_parts_plt_f;

extern rm_statistics	*g_rm_stats;

extern tw_lpid		 nlp_per_pe;
extern tw_lpid		 nrmlp_per_pe;
extern tw_pe		 g_rm_pe;

extern unsigned int	*g_rm_spatial_grid;
extern unsigned int	*g_rm_spatial_grid_i;
extern double		*g_rm_spatial_d;
extern unsigned int	 g_rm_spatial_dim;
extern unsigned int	 g_rm_spatial_dir;
extern unsigned int	 g_rm_spatial_offset;
extern tw_stime		*g_rm_spatial_offset_ts;
extern tw_stime		 g_rm_scatter_ts;
extern double		 g_rm_spatial_coeff;
extern double		 g_rm_spatial_ground_coeff;
extern int		**g_rm_z_values;
extern int		 g_rm_z_max;

extern char		 g_rm_spatial_scenario_fn[];
extern char		 g_rm_spatial_terrain_fn[];
extern char		 g_rm_spatial_urban_fn[];
extern char		 g_rm_spatial_vegatation_fn[];

extern double		 g_rm_wave_threshold;
extern double		 g_rm_wave_attenuation;
extern double		 g_rm_wave_loss_coeff;
extern double		 g_rm_wave_velocity;

extern tw_fd		 g_rm_fd;

#endif
