#include "wifi-mac.h"

unsigned int	 g_wifi_mac_optmem = 1000;

FILE		*g_wifi_mac_waves_plt_f = NULL;
FILE		*g_wifi_mac_nodes_plt_f = NULL;
FILE		*g_wifi_mac_parts_plt_f = NULL;

wifi_mac_statistics	*g_wifi_mac_stats = NULL;

	/*
	 * RM Model Globals
	 *
	 * nwifi_maclp_per_pe	-- number of wifi_mac LPs per PE
	 */
tw_lpid		 nwifi_mac_lp_per_pe = 0;

double		 g_wifi_mac_wave_threshold = 10000.0;
double		 g_wifi_mac_wave_attenuation = 0.0003;
double		 g_wifi_mac_wave_loss_coeff = -1.0;
double		 g_wifi_mac_wave_velocity = 0.0;
