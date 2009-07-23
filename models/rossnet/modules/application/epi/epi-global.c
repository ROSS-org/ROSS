#include <epi.h>

/*
 * epi-global.c -- EPI global vars
 */
unsigned int	 g_epi_hospital = 0;

epi_statistics	*g_epi_stats = NULL;

char		*g_epi_ic_fn = NULL;
char		*g_epi_position_fn = NULL;
FILE		*g_epi_position_f = NULL;
FILE		*g_epi_log_f = NULL;
FILE		*g_epi_hospital_f = NULL;

double		 g_epi_psick = 0.0;
double		 g_epi_work_while_sick_p = 0.0;
unsigned int	 g_epi_nagents = 0;
unsigned int	 g_epi_nsick = 0;
unsigned int	**g_epi_ct = NULL;
unsigned int	 g_epi_nct = 0;
unsigned int	 g_epi_day = 0;
int		 g_epi_mod = 0;
unsigned int	 g_epi_nstages = 0;
unsigned int	 g_epi_complete = 0;

int		 g_epi_stay_home = 0;
double		 g_epi_worried_well_rate = 0;
double		 g_epi_worried_well_threshold = 0;
unsigned int	 g_epi_worried_well_duration = 0;
epi_ic_stage	*g_epi_stages = NULL;

int		 g_epi_nreport_days = 0;
int		*g_epi_report_days = NULL;
int		 g_epi_next_report_day = 0;

tw_fd		 g_epi_fd;

// DBL_MIN = 2.22507e-308
double		EPSILON = 1.0e-200;
