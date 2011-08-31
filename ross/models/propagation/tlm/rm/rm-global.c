#include <rm.h>

FILE	*g_rm_waves_plt_f = NULL;
FILE	*g_rm_nodes_plt_f = NULL;
FILE	*g_rm_parts_plt_f = NULL;

rm_statistics	*g_rm_stats = NULL;

	/*
	 * RM Model Globals
	 *
	 * nlp_per_pe		-- total number of LPs per PE
	 * nrmlp_per_per	-- total number of RM LPs per PE
	 *
	 * g_rm_pe		-- pe to hold PM types
	 */
tw_lpid		 nlp_per_pe = 0;
tw_lpid		 nrmlp_per_pe = 0;

tw_pe		 g_rm_pe;

	/*
	 * Spatial Grid Variables:
	 *
	 * g_rm_spatial_grid	-- grid dimensions
	 * g_rm_spatial_coords	-- number of cells per dimension
	 * g_rm_spatial_d	-- distance between 2 cells
	 * g_rm_spatial_dim	-- number of spatial dimensions
	 * g_rm_spatial_dir	-- number of cells around a cell
	 * g_rm_spatial_offset	-- starting cell LP id
	 * g_rm_spatial_coeff	-- the 2 / # dimensions coefficient
	 * g_rm_spatial_ground_coeff	-- attenuation for ground reflection
	 * g_rm_z_values	-- placeholder for terrain Z values
	 *
	 * g_rm_spatial_scenario_fn	-- directory containing all scenario files
	 * g_rm_spatial_terrain_fn	-- terrain file contains Z values (elevations)
	 * g_rm_spatial_urban_fn	-- contains building elevations
	 * g_rm_spatial_vegatation_fn	-- contains vegatation elevations
	 */
unsigned int	*g_rm_spatial_grid = NULL;
unsigned int	*g_rm_spatial_grid_i = NULL;
double		*g_rm_spatial_d = NULL;
unsigned int	 g_rm_spatial_dim = 2;
unsigned int	 g_rm_spatial_dir = 0;
unsigned int	 g_rm_spatial_offset = 0;
tw_stime	*g_rm_spatial_offset_ts = NULL;
tw_stime	 g_rm_scatter_ts = DBL_MAX;
double		 g_rm_spatial_coeff = 0.0;
double		 g_rm_spatial_ground_coeff = 0.0;
int		**g_rm_z_values = NULL;
int		 g_rm_z_max = 0;

char		g_rm_spatial_scenario_fn[255];
char		g_rm_spatial_terrain_fn[255];
char		g_rm_spatial_urban_fn[255];
char		g_rm_spatial_vegatation_fn[255];

tw_fd		 g_rm_fd;

	/*
	 * Wave propagation global variables:
	 *
	 * g_rm_wave_threshold	 -- the wave attenuation threshold
	 * g_rm_wave_attenuation -- the wave attenuation coefficient for the medium
	 *				(freespace / air is 0.0003)
	 * g_rm_wave_loss_coeff	 -- the loss coefficient that describes the
	 *			    amplitude attenuation over a distance d
	 * g_rm_wave_velocity	 -- the velocity of the wave through the medium
	 */
double		 g_rm_wave_threshold = 10000.0;
double		 g_rm_wave_attenuation = 0.0003;
double		 g_rm_wave_loss_coeff = -1.0;
double		 g_rm_wave_velocity = 0.0;
