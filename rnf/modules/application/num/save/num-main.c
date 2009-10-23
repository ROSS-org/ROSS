#include <num.h>

/*
 * main	- start function for the model, setup global state space of model and
 *	  init and run the simulation executive. 
 */
void
num_main(int argc, char **argv, char **env)
{
	int	 ndays;

	g_num_stats = tw_vector_create(1, sizeof(num_statistics));

	// ensure endtime doesn't fall at midnight
	ndays = g_tw_ts_end / 86400;
	if(g_tw_ts_end - (ndays * 86400) == 0)
		g_tw_ts_end += 1;

	// parse coommand line args
	num_setup_options(argc, argv);

	// read input files, create global data structures, etc
	num_init_scenario();
}

void
num_md_final()
{
	printf("\nNetwork User Model Final Statistics: \n");
	printf("\n");
	printf("\t%-50s %11d\n", "Max Network Population", g_num_max_users);
	printf("\t%-50s %11d\n", "Ttl Start Events", g_num_stats->s_nstart);
	printf("\t%-50s %11d\n", "Ttl Stop Events", g_num_stats->s_nstop);
	printf("\t%-50s %11d\n", "Ttl File Transfers", g_num_stats->s_nfiles);
	printf("\n");

	if (g_num_log_f)
		fclose(g_num_log_f);
}
