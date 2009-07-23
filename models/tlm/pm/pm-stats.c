#include <pm.h>

void
pm_stats_print()
{
	if(g_tw_mynode)
		return;

	printf("\nPM Statistics: \n");
	printf("\n");
	printf("\t%-50s %11.2lf\n", "Waves Percentage", percent_wave);
	printf("\t%-50s %11d\n", "Number of Radios", g_pm_nnodes * g_tw_npe * tw_nnodes());
	printf("\t%-50s %11d\n", "Number of Waves", g_pm_stats->s_nwaves);
	printf("\t%-50s %11ld\n", "Number of Moves", g_pm_stats->s_move_ev);

#if 0
	printf("\t%-50s %11ld\n", "Ttl PROX Events", g_pm_stats->s_prox_ev);
	printf("\t%-50s %11ld\n", "Ttl Recv Events", g_pm_stats->s_recv_ev);
	printf("\n");

	printf("PM Connections:\n\n");
	printf("\t%-42s %11ld\n", "Ttl Direct Conns",
						g_pm_stats->s_nconnect);
	printf("\t%-42s %11ld\n", "Ttl Direct Disconnects",
						g_pm_stats->s_ndisconnect);
	printf("\n");
#endif
}
