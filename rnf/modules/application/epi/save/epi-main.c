#include <epi.h>

/*
 * main	- start function for the model, setup global state space of model and
 *	  init and run the simulation executive.  Also must map LPs to PEs
 */
void
epi_main(int argc, char **argv, char **env)
{
	char	log[1024];

	g_epi_stats = tw_vector_create(1, sizeof(epi_statistics));

	sprintf(log, "%s/epi.log", g_rn_logs_dir);
	g_epi_log_f = fopen(log, "w");

	if(!g_epi_log_f)
		tw_error(TW_LOC, "Unable to open: %s \n", log);

	// parse command line args
	epi_setup_options(argc, argv);

	// read input files, create global data structures, etc
	epi_init_scenario();
}

void
epi_md_final()
{
	if (g_epi_log_f)
		fclose(g_epi_log_f);

	printf("\nPandemic Flu Initial State Variables: \n");
	printf("\n");
	printf("\t%-50s %11d\n", "Total Agents", g_epi_nagents);
	printf("\t%-50s %11d\n", "Total Sick", g_epi_nsick);
	printf("\n");

	epi_stats_print();
}
