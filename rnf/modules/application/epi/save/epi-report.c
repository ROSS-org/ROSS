/**********************************************
 * epi-report.c                               *
 *                                            *
 * Writes percent of agents (by CT) who are   *
 *   Susceptible, Incubating, Symptomatic,    *
 *   Recovered, Dead.                         *
 *                                            *
 * Columns:                                   *
 *   Day                                      *
 *   CT                                       *
 *   Percent Susceptible                      *
 *   Percent Incubating                       *
 *   Percent Symptomatic                      *
 *   Percent Recovered                        *
 *   Percent Deceased                         *
 *   Total Agents in CT                       *
 *   Agents Susceptible                       *
 *   Agents Exposed (Incubating)              *
 *   Agents Infected (Symptomatic)            *
 *   Agents Recovered                         *
 *   Agents Dead                              *
 **********************************************/

#include <epi.h>

// 5/17/2007 ERB added a CT total to report

void
epi_report_census_tract()
{
	int i,j;
	int total;

	for(i = 0; i < g_epi_nct; i++)
	{
		total = 0;
		for (j = 0; j <= g_epi_nstages; j++)
		{
			total += g_epi_ct[i][j];
		}

		fprintf(g_epi_log_f, "%d,%d", g_epi_day, i);

		for(j = 0; j <= g_epi_nstages; j++)
		{
			fprintf(g_epi_log_f, ",%d", (100*g_epi_ct[i][j])/total);
		}

		fprintf(g_epi_log_f,",%d", total);

		for(j = 0; j <= g_epi_nstages; j++)
		{
			fprintf(g_epi_log_f, ",%d", g_epi_ct[i][j]);
		}

		fprintf(g_epi_log_f, "\n");
	}
}
