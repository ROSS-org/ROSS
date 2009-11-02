#include <rm.h>
#include <getopt.h>

/*
 * rm-setup.c - command line arguments
 */
static const tw_optdef rm_options [] =
{
	//TW_OPT_UINT("cmd", var, "help text"),
	TWOPT_END()
};

/*
 * rm_setup_options - parse the command line arguments
 */
void
rm_setup_options(int argc, char ** argv)
{
	/*
	 * Default Global Variables
	 */
	strcpy(g_rm_spatial_scenario_fn, "scenario");
	strcpy(g_rm_spatial_terrain_fn, "elev.txt");
	strcpy(g_rm_spatial_urban_fn, "urb.txt");
	strcpy(g_rm_spatial_vegatation_fn, "veg.txt");

	tw_opt_add(rm_options);
	
	// Fix up global variables
	g_tw_ts_end += 0.1;
}
