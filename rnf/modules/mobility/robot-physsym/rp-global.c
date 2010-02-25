#include <rp.h>

tw_fd		 g_rp_fd = -1;

/*
 * rp-global.c -- Random Walk global vars
 */

/*
 * g_rp_plot		- plot node paths?
 */
unsigned int 	 g_rp_nnodes = 1;
int 		 g_rp_plot = 1;

rp_statistics	*g_rp_stats = NULL;

/*
 * User Model Topology Variables
 */
FILE		*g_rp_position_f = NULL;
unsigned int	*g_rp_position_x = NULL;
unsigned int	*g_rp_position_y = NULL;
unsigned int	*g_rp_ids = NULL;

// OLD value from Random Walk specific
unsigned int	 g_rp_mu = 1;
unsigned int	 g_rp_distr_sd = 5;

tw_stime	 percent_wave = 0.25;
