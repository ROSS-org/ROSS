#include <num.h>

/*
 * num-global.c -- NUM global vars
 */
num_statistics	*g_num_stats = NULL;

int		 g_num_mod = 0;
unsigned int	**g_num_level = NULL;

unsigned int	*g_num_net_pop = NULL;
unsigned int	 g_num_max_users = 0;
unsigned int	 g_num_nprofiles = 0;
unsigned int	*g_num_profile_cnts = NULL;
int		 g_num_day = 0;
int		 g_num_debug = 0;
unsigned int	 g_num_debug_lp = 0;
num_profile	*g_num_profiles = NULL;
char		*g_num_profiles_fn = NULL;
char		*g_num_log_fn =NULL;
int              g_num_nreport_days = 0;
int             *g_num_report_days = NULL; 
int              g_num_next_report_day = 0;


FILE		*g_num_log_f;
