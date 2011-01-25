#include <rm.h>

#define PLOT 0

#define RM_RANGE_PARTICLES 1
#define TOP	1
#define RIGHT	1
#define BOTTOM	1
#define LEFT	1

tw_petype	rm_pes[] = {
	{
		(pe_init_f) rm_pe_init,
		(pe_init_f) rm_pe_post_init,
		(pe_gvt_f) rm_pe_gvt,
		(pe_final_f) rm_pe_final,
                (pe_periodic_f) 0
	},
	{0},
};

tw_lptype       rm_lps[] = {
{
	 (init_f) _rm_init,
	 (event_f) _rm_event_handler,
	 (revent_f) _rm_rc_event_handler,
	 (final_f) _rm_final,
	 (map_f) _rm_map,
	 sizeof(rm_state)},
	{0},
};

static unsigned int optimistic_memory = 0;

static const tw_optdef rm_options [] =
{
	TWOPT_UINT("memory", optimistic_memory, "Additional memory buffers"),
	TWOPT_STIME("threshold", g_rm_wave_threshold, "Wave propagation threshold"),
	TWOPT_CHAR("scenario", g_rm_spatial_scenario_fn, "Path to scenario files"),
	TWOPT_END()
};

void
rm_init(int * argc, char *** argv)
{
	/*
	 * Default Global Variables
	 */
	strcpy(g_rm_spatial_scenario_fn, "scenario");

	// add command line options
	tw_opt_add(rm_options);

	// Fix up global variables
	g_tw_ts_end += 0.1;

	// finally, init ROSS
	tw_init(argc, argv);

	if(g_rm_spatial_scenario_fn[strlen(g_rm_spatial_scenario_fn)] != '/')
		g_rm_spatial_scenario_fn[strlen(g_rm_spatial_scenario_fn)] = '/';

	strcpy(g_rm_spatial_terrain_fn, g_rm_spatial_scenario_fn);
	strcpy(g_rm_spatial_urban_fn, g_rm_spatial_scenario_fn);
	strcpy(g_rm_spatial_vegatation_fn, g_rm_spatial_scenario_fn);

	strcat(g_rm_spatial_terrain_fn, "elev.txt");
	strcat(g_rm_spatial_urban_fn, "urb.txt");
	strcat(g_rm_spatial_vegatation_fn, "veg.txt");
}

	/*
	 * The event being processed that caused this function to be
	 * called is a user model event.  Therefore, I cannot use it
	 * for state saving.  Solution: create my own event and send
	 * it with a 0.0 offset timestamp.
	 */
void
rm_wave_initiate(double * position, double signal, tw_lp * lp)
{
	tw_lpid		 gid = rm_getcell(position);
	tw_event	*e;

	rm_message	*m;

if(0 && gid > 10000499)
{
	printf("%lld %lld: (%lf, %lf, %lf) %lf\n", lp->gid, lp->id, 
		position[0], position[1], position[2], rm_getelevation(position));
	tw_error(TW_LOC, "here");
}

	e = tw_event_new(gid, g_rm_scatter_ts, lp);

	m = tw_event_data(e);
	m->type = RM_WAVE_INIT;
	m->displacement = signal;
	m->id = lp->gid;

	tw_event_send(e);
}

double
rm_move2(double range, double position[], double velocity[], tw_lp * lp)
{
#if PLOT
	rm_pe	*rpe = tw_pe_data(lp->pe);

	fprintf(rpe->move_log, "%.16lf %ld %lf %lf %lf\n", 
		tw_now(lp), lp->gid, position[0], position[1], position[2]);
#endif

	return 0.0;
}

/*
 * Should really only need to call this when the velocity vector changes
 * in the user model, RM should be able to move it at the correct frequency
 * on it's own.
 */
double
rm_move(double range, double position[], double velocity[], tw_lp * lp)
{
	//tw_lpid	 me = rm_getcell(position);

#if RM_RANGE_PARTICLES
	double	 pos[g_rm_spatial_dim];

	int	 end;
	int	 i;
#endif

	int	 x;
	int	 y;

	//printf("%ld: moving at %lf \n", lp->id, tw_now(lp));

	// compute LP id of the cell containing this user model LP
	x = position[0] / g_rm_spatial_d[0];
	y = position[1] / g_rm_spatial_d[1];

	if((x < 0 || x > g_rm_spatial_grid[0]) ||
	   (y < 0 || y > g_rm_spatial_grid[1]))
	{
		printf("%lld: walked off grid! (%d, %d)\n", lp->id, x, y);
		return 0.0;
	}

	fprintf(g_rm_nodes_plt_f, "%lld %lf %lf %lf\n", lp->id, 
		position[0], position[1], position[2]);

#if RM_RANGE_PARTICLES
	// send particles to cells at range boundary
	pos[0] = x - (range / g_rm_spatial_d[0]);
	pos[1] = y + (range / g_rm_spatial_d[1]);
	pos[2] = rm_getelevation(pos);

	end = range * 2;

	// top
	if(pos[1] >= 0 && pos[1] < g_rm_spatial_grid[1])
	{
		for(i = 0; i < end+1; i++)
		{
#if TOP
			if(pos[0] >= 0 && pos[0] < g_rm_spatial_grid[0])
				rm_particle_send(rm_getcell(pos), 1.0, lp);
#endif

			pos[0]++;
		}
	} else
		pos[0] += end + 1;

	// right
	pos[0]--;
	if(pos[0] >= 0 && pos[0] < g_rm_spatial_grid[0])
	{
		for(i = 0; i < end; i++)
		{
			pos[1]--;

#if RIGHT
			if(pos[1] >= 0 && pos[1] < g_rm_spatial_grid[1])
				rm_particle_send(rm_getcell(pos), 1.0, lp);
#endif
		}
	} else
		pos[1] -= end;

	// bottom
	if(pos[1] >= 0 && pos[1] < g_rm_spatial_grid[1])
	{
		for(i = 0; i < end; i++)
		{
			pos[0]--;

#if BOTTOM
			if(pos[0] >= 0 && pos[0] < g_rm_spatial_grid[0])
				rm_particle_send(rm_getcell(pos), 1.0, lp);
#endif
		}
	} else
		pos[0] -= end;

	// left
	if(pos[0] >= 0 && pos[0] < g_rm_spatial_grid[0])
	{
		for(i = 0; i < end-1; i++)
		{
			pos[1]++;

#if LEFT
			if(pos[1] >= 0 && pos[1] < g_rm_spatial_grid[1])
				rm_particle_send(rm_getcell(pos), 1.0, lp);
#endif
		}
	} else
		pos[1] += end - 1;
#endif

	tw_error(TW_LOC, "Cannot get displacement for remote LP this way!");

#if 0
	if(me->cur_state)
	{
		rm_state * state = me->cur_state;
		return state->displacement;
	} else
		return 0.0;
#endif
}

void
rm_rc_move(tw_lp * lp)
{
	//tw_memory	*b;

	//while(NULL != (b = tw_event_memory_get(lp)))
		//tw_memory_alloc_rc(lp, b, g_rm_fd);
}

	/*
	 * This function initializes the environment model.. should be called
	 * once by user model.  Allows EM to control how ROSS is init'd.
	 *
	 * This function provides the Reactive Model with a pre-simulation 'main'
	 */
int
rm_initialize(tw_petype * ptypes, tw_lptype * types, tw_peid npe, 
		tw_kpid nkp, tw_lpid nradios, size_t msg_sz)
{
	//FILE		*fp;

	tw_lptype 	*t;
	//tw_pe		*pe;
	//tw_kp		*kp;
	//tw_lp		*lp;

	//size_t		 size;

	//int		 max_name;
	int	   	 ntypes;
	tw_lpid		 nlp_grid;
	//int		 nkp_grid;
	//int		 nnodes;
	int		 i;
	//int		 j;
	//int		 k;
	//int		 m;

	//int		 kp_per_pe;

	/*
	 * Open debug plotting files
	 */
#if 0
	g_rm_waves_plt_f = fopen("waves.plt", "w");
	g_rm_nodes_plt_f = fopen("user_nodes.plt", "w");
	g_rm_parts_plt_f = fopen("particles.plt", "w");

	if(!g_rm_nodes_plt_f || !g_rm_parts_plt_f)
		tw_error(TW_LOC, "Unable to open plotting files!");
#endif

	g_rm_stats = tw_calloc(TW_LOC, "rm stats", sizeof(rm_statistics), 1);
	memset(g_rm_stats, 0, sizeof(rm_statistics));

	// # of cells around me = 2 * # spatial_dim
	g_rm_spatial_dir = g_rm_spatial_dim * 2;
	g_rm_spatial_offset = nradios;
	g_rm_spatial_coeff = 2.0 / g_rm_spatial_dir;

	g_rm_spatial_grid_i = tw_calloc(TW_LOC, "spatial grid i", sizeof(int), g_rm_spatial_dim);
	g_rm_spatial_offset_ts = tw_calloc(TW_LOC, "spatial offset ts", sizeof(tw_stime), g_rm_spatial_dir);

	g_rm_spatial_ground_coeff = 0.75;

	if(0.0 > g_rm_wave_loss_coeff)
	{
		g_rm_wave_loss_coeff = 0.5;
		g_rm_wave_loss_coeff = 1.0 / exp(g_rm_wave_attenuation * g_rm_spatial_d[0]);

		if(tw_node_eq(&g_tw_mynode, &g_tw_masternode))
			printf("\n\tSETTING WAVE LOSS COEFF %lf! \n\n", g_rm_wave_loss_coeff);
	}

	g_rm_wave_velocity = 3.0 * 1000.0 * 1000.0 * 1000.0;

	// Require NULL terminated array, plus LPs for Cells
	for(ntypes = 2, t = types; t->state_sz; t++)
		ntypes++;

	//printf("Creating %d lp types\n", ntypes);
	t = tw_calloc(TW_LOC, "lp types array", sizeof(tw_lptype), ntypes);
	memcpy(t, types, sizeof(tw_lptype) * (ntypes-2));
	memcpy(&t[ntypes-2], rm_lps, sizeof(rm_lps));

	nlp_grid = rm_grid_init();
	nrmlp_per_pe = ceil(nlp_grid / (tw_nnodes() * g_tw_npe));

	if(tw_nnodes() == 1)
		nrmlp_per_pe = nlp_grid;
	nlp_per_pe = nradios + nrmlp_per_pe;

	g_tw_events_per_pe = .1 * nlp_grid / (tw_nnodes() * g_tw_npe);
	g_tw_events_per_pe += optimistic_memory;

	rm_grid_terrain();

	for(i = 0; i < g_tw_npe; i++)
		tw_pe_settype(g_tw_pe[i], rm_pes);
	tw_pe_settype(&g_rm_pe, ptypes);

	g_tw_rng_default = TW_FALSE;
	tw_define_lps(nlp_per_pe, sizeof(rm_message), 0);

	for(i = 0; i < g_rm_spatial_offset; i++)
		tw_lp_settype(i, types);

	for( ; i < g_tw_nlp; i++)
		tw_lp_settype(i, rm_lps);

	return 1;
}

	/*
	 * This function provides the Reactive Model with a post-simulation 'main'
	 */
void
rm_run(char ** argv)
{
	FILE	*F;

	int	 i;

	// if not master, just call tw_run and return
	if(!tw_node_eq(&g_tw_mynode, &g_tw_masternode))
	{
		tw_run();
		return;
	}

#if RM_LOG_STATS
	F = fopen("reactive_model.log", "w");

	if(!F)
		tw_error(TW_LOC, "Unable to open Reactive Model logfile!");
#else
	F = stdout;
#endif

	fprintf(F, "\nReactive Model Parameters: \n");
	fprintf(F, "\n");
	fprintf(F, "\t%-50s\n", "Spatial Parameters:");
	fprintf(F, "\n");
	fprintf(F, "\t\t%-50s %11.4lf\n", "Spatial Coefficient", g_rm_spatial_coeff);
	fprintf(F, "\n");
	fprintf(F, "\t\t%-50s %11dD\n", "Dimensions Computed", g_rm_spatial_dim);

	for(i = 0; i < g_rm_spatial_dim; i++)
	{
		fprintf(F, "\t\t%-50s %11d %dD\n", "Cells per Dimension", 
						g_rm_spatial_grid[i], i+1);
		fprintf(F, "\t\t%-50s %11.4lf %dD\n", "Cell Spacing ", 
						g_rm_spatial_d[i], i+1);
	}

	fprintf(F, "\n");
	fprintf(F, "\t%-50s\n", "Temporal Parameters:");
	fprintf(F, "\n");
	fprintf(F, "\t\t%-50s %11.11lf\n", "Scatter Offset TS", g_rm_scatter_ts);
	fprintf(F, "\t\t%-50s %11.4lf\n", "Loss Coefficient", g_rm_wave_loss_coeff);
	fprintf(F, "\t\t%-50s %11.4lf\n", "Velocity", g_rm_wave_velocity);

	for(i = 0; i < g_rm_spatial_dim; i++)
		fprintf(F, "\t\t%-50s %11.11lf %dD\n", "Timestep (d/V)", 
					g_rm_spatial_offset_ts[i], i+1);
	fprintf(F, "\t\t%-50s %11.4lf\n", "Amplitude Threshold", g_rm_wave_threshold);

	tw_run();

	fprintf(F, "\nReactive Model Statistics: \n");
#if SAVE_MEM
	fprintf(F, "\tRange Statistics: \n");
	fprintf(F, "\t%-50s %11ld\n", "Range Particles Received", 
						g_rm_stats->s_nparticles);
#endif
	fprintf(F, "\tWave Statistics: \n\n");
	fprintf(F, "\t%-50s %11lld\n", "Initiate", 
						g_rm_stats->s_ncell_initiate);
	fprintf(F, "\t%-50s %11lld\n", "Scatters", 
						g_rm_stats->s_ncell_scatter);
	fprintf(F, "\t%-50s %11lld\n", "Gathers", 
						g_rm_stats->s_ncell_gather);

	//fclose(g_rm_waves_plt_f);
	//fclose(g_rm_nodes_plt_f);
	//fclose(g_rm_parts_plt_f);
}

void
rm_end(void)
{
	tw_end();
}
