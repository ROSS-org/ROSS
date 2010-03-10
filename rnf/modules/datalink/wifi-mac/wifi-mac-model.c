#include "wifi-mac.h"

static FILE	*g_wifi_mac_output_f = NULL;

static const tw_optdef wifi_mac_options [] =
{
	TWOPT_GROUP("802.11b Mac Layer Model"),
	TWOPT_UINT("memory", g_wifi_mac_optmem, "Additional memory buffers"),
	TWOPT_END()
};

void
wifi_mac_md_opts()
{
	tw_opt_add(wifi_mac_options);
}

void
wifi_mac_md_init(int argc, char ** argv, char ** env)
{
	tw_lpid		 nlp_grid;

	int		 i;

	if(!g_rn_environment)
		return;

	g_wifi_mac_stats = tw_calloc(TW_LOC, "", sizeof(*g_wifi_mac_stats), 1);

	// g_wifi_mac_output_fix up global variables
	g_tw_ts_end += 0.1;

	printf("\nInitializing Model: Transmission Line Matrix\n");

}

	/*
	 * This function provides the Reactive Model with a post-simulation 'main'
	 */
void
wifi_mac_md_final()
{
	if(!tw_ismaster())
		return;

	if(!g_rn_environment)
		return;

	fprintf(g_wifi_mac_output_f, "\nTransmission Line Matrix Model: Statistics: \n");

}
