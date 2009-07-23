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
epi_report_census_tract(int today)
{
	int i,j,k;
	int total[g_epi_ndiseases];

	g_epi_complete = 1;

	for(i = 0; i < g_epi_nregions; i++)
	{
		for(j = 0; j < g_epi_ndiseases; j++)
		{
			total[j] = 0;

			for (k = 0; k <= g_epi_diseases[j].nstages; k++)
				total[j] += g_epi_regions[i][j][k];
		}

		for(j = 0; j < g_epi_ndiseases; j++)
		{
			if(0 == total[j])
				continue;

			fprintf(g_epi_log_f[j], "%d %d %d", today, i, j);

			for(k = 0; k <= g_epi_diseases[j].nstages; k++)
				fprintf(g_epi_log_f[j], " %d", (100*g_epi_regions[i][j][k])/total[j]);

			fprintf(g_epi_log_f[j]," %d", total[j]);

			for(k = 0; k <= g_epi_diseases[j].nstages; k++)
				fprintf(g_epi_log_f[j], " %d", g_epi_regions[i][j][k]);

			// print just newly exposed for today
			fprintf(g_epi_log_f[j], " %d", g_epi_exposed_today[j]);
			g_epi_exposed_today[j] = 0;

			if(g_epi_regions[i][j][1] || g_epi_regions[i][j][2])
				g_epi_complete = 0;

			fprintf(g_epi_log_f[j], "\n");
		}
	}

	fflush(NULL);
	fflush(NULL);
	fflush(NULL);
}

void
epi_report_hospitals(int today)
{
	int	 i;

	for(i = 0; i < g_epi_nhospital; i++)
	{
		fprintf(g_epi_hospital_f, "%d %d %u %u %u\n", 
			today, i, g_epi_hospital[i][0], 
			g_epi_hospital_ww[i][0], g_epi_hospital_wws[i][0]);

		g_epi_hospital[i][0] = 0;
		g_epi_hospital_ww[i][0] = 0;
		g_epi_hospital_wws[i][0] = 0;
	}

	fflush(NULL);
	fflush(NULL);
	fflush(NULL);
}
