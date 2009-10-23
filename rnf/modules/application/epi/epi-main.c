#include <epi.h>

void
epi_md_opts()
{
	//tw_opt_add(epi_options);
}

/*
 * main	- start function for the model, setup global state space of model and
 *	  init and run the simulation executive.  Also must map LPs to PEs
 */
void
epi_main(int argc, char **argv, char **env)
{
	g_epi_stats = tw_calloc(TW_LOC, "", 1, sizeof(epi_statistics));

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
