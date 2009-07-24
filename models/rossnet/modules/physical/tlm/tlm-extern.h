#ifndef INC_tlm_extern_h
#define INC_tlm_extern_h

/*
 * tlm-xml.c
 */
extern void tlm_xml(tlm_state *, const xmlNodePtr node, tw_lp *);

/*
 * tlm-model.c
 */
extern void tlm_md_init(int argc, char ** argv, char ** env);
extern void tlm_md_final();

/*
 * tlm-pe.c - model per PE functions
 */
extern void tlm_pe_init(tw_pe * me);
extern void tlm_pe_post_init(tw_pe * me);
extern void tlm_pe_gvt(tw_pe * me);
extern void tlm_pe_final(tw_pe * me);

/*
 * tlm-cell.c - perform wave propagation between cells
 */
extern void tlm_cell_handler(tlm_state *, tw_bf * bf, tlm_message * m, tw_lp * lp);
extern void tlm_cell_gather(tlm_state *, tw_bf * bf, tlm_message *, tw_lp * lp);
extern void tlm_cell_scatter(tlm_state *, tlm_message *, tw_bf * bf, tw_lp * lp);

/*
 * rc-tlm-cell.c - perform wave propagation reverse computation 
 */
extern void tlm_rc_cell_handler(tlm_state *, tw_bf * bf, tlm_message * m, tw_lp * lp);
extern void tlm_rc_scatter(tlm_state *, tw_bf * bf, tlm_message *, tw_lp *);
extern void tlm_rc_gather(tlm_state *, tw_bf * bf, tlm_message *, tw_lp *);

/*
 * tlm-timer.c - RM Wrapper for ROSS timer library
 */
extern tw_event	*tlm_timer_cancel(tw_event * t, tw_lp * lp);
extern tw_event	*tlm_timer_start(tw_event * t, int dim, tw_stime ts, tw_lp * lp);

/*
 * tlm-interface.c: implement Reactive Model (tlm) interface
 */
extern void tlm_wave_initiate(double * position, double signal, tw_lp * lp);
extern void tlm_rc_wave_initiate(double * position, double signal);
extern double tlm_move(double, double [], double [], tw_lp * lp);
extern double tlm_move2(double, double [], double [], tw_lp * lp);
extern void tlm_rc_move(tw_lp * lp);

/*
 * tlm-grid.c
 */
extern tw_lpid tlm_grid_init();

/*
 * tlm.c - Cell LP file with simulation engine function definitions
 */
extern void _tlm_init(tlm_state *, tw_lp * lp);
extern void _tlm_event_handler(tlm_state *, tw_bf * , tlm_message *, tw_lp *lp);
extern void _tlm_rc_event_handler(tlm_state *, tw_bf *, tlm_message *, tw_lp * lp);
extern void _tlm_final(tlm_state *, tw_lp * lp);
extern int tlm_main(int argc, char ** argv, char ** env);

/*
 * tlm-particle.c - particle functions
 */
extern tw_event	*tlm_particle_send(tw_lpid gid, tw_stime, tw_lp *);
extern void tlm_particle_handler(tlm_state *, tw_bf * bf, tlm_message * m, tw_lp * lp);
extern void tlm_rc_particle_handler(tlm_state *, tw_bf *, tlm_message * m, tw_lp * lp);

/*
 * tlm-proximity.c: proximity detection related functions
 */
extern void tlm_proximity_send(tlm_state *, tw_memory * p, tw_memory * b, tw_lp * lp);

/*
 * tlm-helper.c: inlined helper functions
 */
extern double	 tlm_getelevation(double * p);
extern tw_lpid	 tlm_getcell(double * position);
extern double	*tlm_getlocation(tw_lp * lp);

/*
 * tlm-global.c: global variables
 */
extern unsigned int	 g_tlm_optmem;

extern FILE		*g_tlm_waves_plt_f;
extern FILE		*g_tlm_nodes_plt_f;
extern FILE		*g_tlm_parts_plt_f;

extern tlm_statistics	*g_tlm_stats;

extern tw_lpid		 nlp_per_pe;
extern tw_lpid		 ntlm_lp_per_pe;

extern unsigned int	*g_tlm_spatial_grid;
extern unsigned int	*g_tlm_spatial_grid_i;
extern unsigned int	*g_tlm_spatial_d;
extern unsigned int	 g_tlm_spatial_dim;
extern unsigned int	 g_tlm_spatial_dir;
extern unsigned int	 g_tlm_spatial_offset;
extern tw_stime		*g_tlm_spatial_offset_ts;
extern tw_stime		 g_tlm_scatter_ts;
extern double		 g_tlm_spatial_coeff;
extern double		 g_tlm_spatial_ground_coeff;
extern int		**g_tlm_z_values;
extern int		 g_tlm_z_max;

extern char		 g_tlm_spatial_scenario_fn[];
extern char		 g_tlm_spatial_terrain_fn[];
extern char		 g_tlm_spatial_urban_fn[];
extern char		 g_tlm_spatial_vegatation_fn[];

extern double		 g_tlm_wave_threshold;
extern double		 g_tlm_wave_attenuation;
extern double		 g_tlm_wave_loss_coeff;
extern double		 g_tlm_wave_velocity;

extern tw_fd		 g_tlm_fd;

#endif
