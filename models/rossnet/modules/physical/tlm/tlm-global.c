#include <tlm.h>

unsigned int	 g_tlm_optmem = 1000;

FILE		*g_tlm_waves_plt_f = NULL;
FILE		*g_tlm_nodes_plt_f = NULL;
FILE		*g_tlm_parts_plt_f = NULL;

tlm_statistics	*g_tlm_stats = NULL;

	/*
	 * RM Model Globals
	 *
	 * ntlmlp_per_pe	-- number of TLM LPs per PE
	 */
tw_lpid		 ntlm_lp_per_pe = 0;

	/*
	 * Spatial Grid Variables:
	 *
	 * g_tlm_spatial_grid	-- grid dimensions
	 * g_tlm_spatial_coords	-- number of cells per dimension
	 * g_tlm_spatial_d	-- distance between 2 cells
	 * g_tlm_spatial_dim	-- number of spatial dimensions
	 * g_tlm_spatial_dir	-- number of cells around a cell
	 * g_tlm_spatial_offset	-- starting cell LP id
	 * g_tlm_spatial_coeff	-- the 2 / # dimensions coefficient
	 * g_tlm_spatial_ground_coeff	-- attenuation for ground reflection
	 * g_tlm_z_values	-- placeholder for terrain Z values
	 *
	 * g_tlm_spatial_scenario_fn	-- directory containing all scenario files
	 * g_tlm_spatial_terrain_fn	-- terrain file contains Z values (elevations)
	 * g_tlm_spatial_urban_fn	-- contains building elevations
	 * g_tlm_spatial_vegatation_fn	-- contains vegatation elevations
	 */
unsigned int	*g_tlm_spatial_grid = NULL;
unsigned int	*g_tlm_spatial_grid_i = NULL;
unsigned int	*g_tlm_spatial_d = NULL;
unsigned int	 g_tlm_spatial_dim = 3;
unsigned int	 g_tlm_spatial_dir = 0;
unsigned int	 g_tlm_spatial_offset = 0;
tw_stime	*g_tlm_spatial_offset_ts = NULL;
tw_stime	 g_tlm_scatter_ts = DBL_MAX;
double		 g_tlm_spatial_coeff = 0.0;
double		 g_tlm_spatial_ground_coeff = 0.0;
int		**g_tlm_z_values = NULL;
int		 g_tlm_z_max = 0;

char		g_tlm_spatial_scenario_fn[255];
char		g_tlm_spatial_terrain_fn[255];
char		g_tlm_spatial_urban_fn[255];
char		g_tlm_spatial_vegatation_fn[255];

	/*
	 * Wave propagation global variables:
	 *
	 * g_tlm_wave_threshold	 -- the wave attenuation threshold
	 * g_tlm_wave_attenuation -- the wave attenuation coefficient for the medium
	 *				(freespace / air is 0.0003)
	 * g_tlm_wave_loss_coeff	 -- the loss coefficient that describes the
	 *			    amplitude attenuation over a distance d
	 * g_tlm_wave_velocity	 -- the velocity of the wave through the medium
	 */
double		 g_tlm_wave_threshold = 10000.0;
double		 g_tlm_wave_attenuation = 0.0003;
double		 g_tlm_wave_loss_coeff = -1.0;
double		 g_tlm_wave_velocity = 0.0;
