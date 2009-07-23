#include <epi.h>

epi_agent * ga = NULL;
unsigned int X = 1;

/*
 * epi-global.c -- EPI global vars
 */
tw_fd		 g_epi_fd = 0;
tw_fd		 g_epi_pathogen_fd = 0;

tw_memoryq	*g_epi_agents = NULL;
void		**g_epi_pq = NULL;

unsigned int	 g_epi_ndiseases = 0;
epi_disease	*g_epi_diseases = NULL;

unsigned int	 g_epi_nagents = 1;

unsigned int	 g_epi_nregions = 1;
unsigned int	***g_epi_regions = NULL;

epi_statistics	*g_epi_stats = NULL;

unsigned int	**g_epi_hospital_wws = NULL;
unsigned int	**g_epi_hospital_ww = NULL;
unsigned int	**g_epi_hospital = NULL;
unsigned int	  g_epi_nhospital = 0;

unsigned int	*g_epi_sick_init_pop = 0;
unsigned int	 g_epi_ww_init_pop = 0;
unsigned int	 g_epi_wws_init_pop = 0;
unsigned int	*g_epi_exposed_today = 0;

// 6 hours, in seconds
tw_stime	 g_epi_mean = 21600.0;
double		 g_epi_sick_rate = 0.0003;
double		 g_epi_ww_rate = 0.0;
double		 g_epi_wws_rate = 0.0;
double		 g_epi_ww_threshold = 0;
unsigned int	 g_epi_ww_duration = 0;

char		 g_epi_agent_fn[1024] = "";
char		 g_epi_ic_fn[1024] = "";
char		 g_epi_position_fn[1024];
char		 g_epi_log_fn[1024];
char		 g_epi_hospital_fn[1024] = "hospital.log";

FILE		*g_epi_position_f = NULL;
FILE		**g_epi_log_f = NULL;
FILE		*g_epi_hospital_f = NULL;

unsigned int	 g_epi_day = 0;
int		 g_epi_mod = 0;

unsigned int	 g_epi_complete = 0;

epi_ic_stage	*g_epi_stages = NULL;

// DBL_MIN = 2.22507e-308
double		EPSILON = 1.0e-200;
