#include <pm.h>
int the_one = 0;

/*
 * pm-global.c -- PM global vars
 */
unsigned int	 g_pm_dimensions = 3;

/*
 * g_pm_runtime	- end time in node ranges traversed per node
 * g_pm_plot		- plot node paths?
 */
double 		 g_pm_runtime = 0;
unsigned int 	 g_pm_nnodes = 1;
int 		 g_pm_plot = 1;

pm_statistics	*g_pm_stats = NULL;

/*
 * User Model Topology Variables
 */
FILE		*g_pm_position_f = NULL;
unsigned int	*g_pm_position_x = NULL;
unsigned int	*g_pm_position_y = NULL;
unsigned int	*g_pm_ids = NULL;

// random-walk specific
unsigned int	 g_pm_mu = 1;
unsigned int	 g_pm_distr_sd = 5;

tw_stime	 percent_wave = 0.25;
char		 run_id[1024] = "undefined";


// ultra high-resolution
//double	 grid[3] = {10000, 10000, 10};
//double	 spacing[3] = {10, 10, 10};

// high-resolution
double	 grid[3] = {1000, 1000, 4};
double	 spacing[3] = {100, 100, 100};

// low-resolution
//double	 grid[3] = {100, 100, 10};
//double	 spacing[3] = {1000, 1000, 10};
