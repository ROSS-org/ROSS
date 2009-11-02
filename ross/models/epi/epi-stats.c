#include <epi.h>

void
epi_stats_print()
{
	int	 i;

	printf("\nEPI Statistics: \n");
	printf("\t%-50s %11.4lf\n", "Initial Infection Prob", g_epi_sick_rate);
	printf("\t%-50s %11.4lf\n", "Initial Worried Well Prob", g_epi_ww_rate);
	printf("\t%-50s %11.4lf\n", "Initial Work While Sick Prob", g_epi_wws_rate);
	printf("\n");
	printf("\t%-50s\n", "Initial Infection Populations:");

	for(i = 0; i < g_epi_ndiseases; i++)
		printf("\t\tDisease %-40d %5d\n", i, g_epi_sick_init_pop[i]);

	printf("\t%-50s %11d\n", "Initial Worried Well Population", g_epi_ww_init_pop);
	printf("\t%-50s %11d\n", "Initial Work While Sick Population", g_epi_wws_init_pop);
	printf("\n");
	printf("\t%-50s %11d\n", "Ttl Number of Infected Agents", g_epi_stats->s_ninfected);
	printf("\t%-50s %11d\n", "Ttl Number of Dead Agents", g_epi_stats->s_ndead);
	printf("\n");





	//printf("\t%-50s %11ld\n", "Ttl Move Events", g_epi_stats->s_move_ev);
	//printf("\t%-50s %11ld\n", "Ttl Agents Checked for Infection", g_epi_stats->s_nchecked);
	//printf("\t%-50s %11ld\n", "Ttl Draws from Uniform Random Distribution", g_epi_stats->s_ndraws);
}
