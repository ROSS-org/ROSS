#include <rw.h>

tw_fd		 g_rw_fd = -1;

/*
 * rw-global.c -- Random Walk global vars
 */

/*
 * g_rw_plot		- plot node paths?
 */
unsigned int 	 g_rw_nnodes = 1;
int 		 g_rw_plot = 1;

rw_statistics	*g_rw_stats = NULL;

/*
 * User Model Topology Variables
 */
FILE		*g_rw_position_f = NULL;
unsigned int	*g_rw_position_x = NULL;
unsigned int	*g_rw_position_y = NULL;
unsigned int	*g_rw_ids = NULL;

// random-walk specific
unsigned int	 g_rw_mu = 1;
unsigned int	 g_rw_distr_sd = 5;

tw_stime	 percent_wave = 0.25;
