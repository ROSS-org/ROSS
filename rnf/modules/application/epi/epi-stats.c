#include <epi.h>

void
epi_stats_print()
{
	printf("\nEPI Statistics: \n");
	printf("\n");
	printf("\t%-50s %11.2lf\n", "Sim End Time (secs)", g_tw_ts_end);
	printf("\n");
	//printf("\t%-50s %11ld\n", "Ttl Move Events", g_epi_stats->s_move_ev);
	//printf("\t%-50s %11ld\n", "Ttl Agents Checked for Infection", g_epi_stats->s_nchecked);
	//printf("\t%-50s %11ld\n", "Ttl Draws from Uniform Random Distribution", g_epi_stats->s_ndraws);
	printf("\t%-50s %11ld\n", "Ttl Number of Infected Agents", g_epi_stats->s_ninfected);
	printf("\t%-50s %11ld\n", "Ttl Number of Dead Agents", g_epi_stats->s_ndead);
	printf("\n");
}
