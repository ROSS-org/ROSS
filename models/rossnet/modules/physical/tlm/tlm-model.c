#include <tlm.h>

static FILE	*g_tlm_output_f = NULL;

static const tw_optdef tlm_options [] =
{
	TWOPT_GROUP("Transmission Line Matrix Model"),
	TWOPT_UINT("memory", g_tlm_optmem, "Additional memory buffers"),
	TWOPT_STIME("threshold", g_tlm_wave_threshold, "Wave propagation threshold"),
	TWOPT_END()
};

void
tlm_md_init(int argc, char ** argv, char ** env)
{
	tw_lpid		 nlp_grid;

	int		 i;

	// add command line options
	tw_opt_add(tlm_options);

	if(!g_rn_environment)
		return;

	g_tlm_stats = tw_calloc(TW_LOC, "", sizeof(*g_tlm_stats), 1);

	// g_tlm_output_fix up global variables
	g_tw_ts_end += 0.1;

	if(0 != strcmp(g_rn_tools_dir, ""))
		sprintf(g_tlm_spatial_terrain_fn, "tools/%s/terrain.txt", g_rn_tools_dir);
	else
		tw_error(TW_LOC, "No terrain file specified!");

	/*
	 * Open debug plotting files
	 */
#if 0
	g_tlm_waves_plt_f = fopen("waves.plt", "w");
	g_tlm_nodes_plt_f = fopen("user_nodes.plt", "w");
	g_tlm_parts_plt_f = fopen("particles.plt", "w");

	if(!g_tlm_nodes_plt_f || !g_tlm_parts_plt_f)
		tw_error(TW_LOC, "Unable to open plotting files!");
#endif

	g_tlm_stats = tw_calloc(TW_LOC, "tlm stats", sizeof(tlm_statistics), 1);
	memset(g_tlm_stats, 0, sizeof(tlm_statistics));

	// # of cells around me = 2 * # spatial_dim
	g_tlm_spatial_dir = g_tlm_spatial_dim * 2;
	g_tlm_spatial_coeff = 2.0 / g_tlm_spatial_dir;

	g_tlm_spatial_grid_i = tw_calloc(TW_LOC, "spatial grid i", sizeof(int), g_tlm_spatial_dim);
	g_tlm_spatial_offset_ts = tw_calloc(TW_LOC, "spatial offset ts", sizeof(tw_stime), g_tlm_spatial_dir);
	g_tlm_spatial_offset = g_rn_nmachines / (tw_nnodes() * g_tw_npe);

	g_tlm_spatial_ground_coeff = 0.75;

	if(0.0 > g_tlm_wave_loss_coeff)
	{
		g_tlm_wave_loss_coeff = 1.0 / exp(g_tlm_wave_attenuation * g_tlm_spatial_d[0]);

		if(tw_ismaster())
			printf("\n\tSETTING WAVE LOSS COEFF %lf! \n\n", g_tlm_wave_loss_coeff);
	}

	// speed of light in m/s
	g_tlm_wave_velocity = 299792458.0;

	nlp_grid = tlm_grid_init();
	ntlm_lp_per_pe = ceil(nlp_grid / (tw_nnodes() * g_tw_npe));

	if(tw_nnodes() == 1)
		ntlm_lp_per_pe = nlp_grid;

	g_tw_events_per_pe = 1.5 * nlp_grid / (tw_nnodes() * g_tw_npe);
	g_tw_events_per_pe += g_tlm_optmem;

#if 0
	for(i = 0; i < g_tlm_spatial_offset; i++)
		tw_lp_settype(i, types);
#endif

	//tw_error(TW_LOC, "setting types not ported");

	if(!tw_ismaster())
		return;

	printf("\nInitializing Model: Transmission Line Matrix\n");
#if DWB
	printf("\t\t%-42s %11d (%ld)\n", "TLM Membufs", 1000000, g_tlm_fd);
#endif

#if RM_LOG_STATS
	g_tlm_output_f = fopen("tlm.log", "w");

	if(!g_tlm_output_f)
		tw_error(TW_LOC, "Unable to open TLM logfile!");
#else
	g_tlm_output_f = stdout;
#endif

	fprintf(g_tlm_output_f, "\n");
	fprintf(g_tlm_output_f, "\t%-50s\n", "Spatial Parameters:");
	fprintf(g_tlm_output_f, "\n");
	fprintf(g_tlm_output_f, "\t\t%-42s %11.4lf\n", "Spatial Coefficient", g_tlm_spatial_coeff);
	fprintf(g_tlm_output_f, "\n");
	fprintf(g_tlm_output_f, "\t\t%-42s %11dD\n", "Dimensions Computed", g_tlm_spatial_dim);

	for(i = 0; i < g_tlm_spatial_dim; i++)
	{
		fprintf(g_tlm_output_f, "\t\t%-42s %11d %dD\n", "Cells per Dimension", 
						g_tlm_spatial_grid[i], i+1);
		fprintf(g_tlm_output_f, "\t\t%-42s %11d %dD\n", "Cell Spacing ", 
						g_tlm_spatial_d[i], i+1);
	}

	fprintf(g_tlm_output_f, "\n");
	fprintf(g_tlm_output_f, "\t%-50s\n", "Temporal Parameters:");
	fprintf(g_tlm_output_f, "\n");
	fprintf(g_tlm_output_f, "\t\t%-42s %11.11lf\n", "Scatter Offset TS", g_tlm_scatter_ts);
	fprintf(g_tlm_output_f, "\t\t%-42s %11.4lf\n", "Loss Coefficient", g_tlm_wave_loss_coeff);
	fprintf(g_tlm_output_f, "\t\t%-42s %11.4lf\n", "Velocity", g_tlm_wave_velocity);

	for(i = 0; i < g_tlm_spatial_dim; i++)
		fprintf(g_tlm_output_f, "\t\t%-42s %11.11lf %dD\n", "Timestep (d/V)", 
					g_tlm_spatial_offset_ts[i], i+1);
	fprintf(g_tlm_output_f, "\t\t%-42s %11.4lf\n", "Amplitude Threshold", g_tlm_wave_threshold);

	fprintf(g_tlm_output_f, "\t%-50s %11d\n", "Spatial Offset", 
						g_tlm_spatial_offset);
}

	/*
	 * This function provides the Reactive Model with a post-simulation 'main'
	 */
void
tlm_md_final()
{
	if(!tw_ismaster())
		return;

	if(!g_rn_environment)
		return;

	fprintf(g_tlm_output_f, "\nTransmission Line Matrix Model: Statistics: \n");
#if DWB
#if SAVE_MEM
	fprintf(g_tlm_output_f, "\tRange Statistics: \n");
	fprintf(g_tlm_output_f, "\t\t%-42s %11ld\n", "Range Particles Received", 
						g_tlm_stats->s_nparticles);
#endif
#endif
	fprintf(g_tlm_output_f, "\tWave Statistics: \n\n");
	fprintf(g_tlm_output_f, "\t\t%-42s %11lld\n", "Initiate", 
						g_tlm_stats->s_ncell_initiate);
	fprintf(g_tlm_output_f, "\t\t%-42s %11lld\n", "Scatters", 
						g_tlm_stats->s_ncell_scatter);
	fprintf(g_tlm_output_f, "\t\t%-42s %11lld\n", "Gathers", 
						g_tlm_stats->s_ncell_gather);


	if(g_tlm_output_f != stdout)
		fclose(g_tlm_output_f);

	//fclose(g_tlm_waves_plt_f);
	//fclose(g_tlm_nodes_plt_f);
	//fclose(g_tlm_parts_plt_f);
}
