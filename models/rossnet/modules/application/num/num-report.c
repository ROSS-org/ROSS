#include <num.h>

/********************************************
 *
 * num-report.c
 *
 * Report by day, by agent type of network use.
 *  -U parameter sets modulo day.  If:
 *    U < 0, all reporting is off.
 *    U = 0, Network User Model is off, number
 *           of agents at home is reported as
 *           level 0.
 *    U > 0, Network User Model is run every
 *           U days.
 *  Default is U = 0.
 *
 * Columns:
 *   Day
 *   Agent type
 *   Percent at level 0 (above desired rate)
 *   Percent at level 1 (above 50%)
 *   Percent at level 2 (above 10%)
 *   Percent at level 3 (below 10%)
 *   Percent at level 4 (cannot connect)
 *   Total agents attempting to use network
 *   Total Agents achieving level 0
 *   Total Agents achieving level 1
 *   Total Agents achieving level 2
 *   Total Agents achieving level 3
 *   Total Agents achieving level 4
 *******************************************/

void
num_print(int today)
{
	int		i,j;
	long int	total;
	long int	total_users = 0;

	for (i = 0; i < g_num_nprofiles; i++)
	{
		total = 0;
		for (j=0; j < NUM_LEVELS; j++)
			total += g_num_level[i][j];

		total_users += total;

		fprintf(g_num_log_f, "%d %d", today, i);

#if 0
		if (g_num_nreport_days > 0)
			fprintf(g_num_log_f, "%d %d", g_num_day-1, i);
		else if(g_num_mod > g_num_day)
			fprintf(g_num_log_f, "%d %d", g_num_day, i);
		else
			fprintf(g_num_log_f, "%d %d", g_num_day-g_num_mod, i);
#endif

		for(j = 0; j < NUM_LEVELS; j++)
		{
			if(total && g_num_mod > 0)
			{
				fprintf(g_num_log_f, " %4.4lf", 
					100.0 * 
						((float) g_num_level[i][j] / 
						(float) total));
			} else if(g_num_nreport_days > 0)
			{
				if (total)
					fprintf(g_num_log_f, " %4.4lf",
						100.0 *
						((float) g_num_level[i][j] / 
						(float) total));
				else
					fprintf(g_num_log_f, " 0.0000");
			} else if(g_num_mod == 0 && g_num_profile_cnts[j])
			{
				fprintf(g_num_log_f, " %4.4lf", 
					100.0 * 
						((float) g_num_level[i][j] / 
						(float) g_num_profile_cnts[j]));
			} else
				fprintf(g_num_log_f, " 0.0000");
		}

		if(g_num_mod > 0 || g_num_nreport_days > 0)
			fprintf(g_num_log_f," %ld", total);
		else
			fprintf(g_num_log_f," %d", g_num_profile_cnts[i]);
			

		for(j = 0; j < NUM_LEVELS; j++)
		{
			// gnuplot isn't happy with zeros
			if(j && total && !g_num_level[i][j])
				fprintf(g_num_log_f, " 0.001");
			else
				fprintf(g_num_log_f, " %d", g_num_level[i][j]);
		}

		fprintf(g_num_log_f, "\n");
	}

	if(total_users > g_num_max_users)
		g_num_max_users = total_users;

	for (i = 0; i < g_num_nprofiles; i++)
	{
		for(j = 0; j < NUM_LEVELS; j++)
			g_num_level[i][j] = 0;

		g_num_net_pop[i] = 0;
	}

	fflush(NULL);
	fsync(fileno(g_num_log_f));
}

// Bunch of conditionals for print num.log
// 1. only report for PREVIOUS day
// 2. only first agent on a given day needs report
// 3. only report if logging enabled
// 4. only report after first MOD period expires
// 5. only report every Nth day (g_num_mod)
// Argh.
//
// Report previous day if g_num_nreport_days > 0
// (Using Y option to say which days to report)
void
num_report_levels(int today)
{
	// same day, different agent
	if(today <= g_num_day)
	{
		if(g_num_report_days && 
		   g_num_day == g_num_report_days[g_num_next_report_day] && 
		   g_num_next_report_day < g_num_nreport_days+1)
			g_num_next_report_day++;

		return;
	}

	g_num_day = today;

	// log file off
	if(!g_num_log_f)
		return;

	// only print every g_num_mod days
	if (g_num_mod == 0 && g_num_nreport_days == 0)
	{
		num_print(today);
	} else if(g_num_nreport_days > 0)
	{
#if VERIFY_NUM
		printf("today: %d, g_num_report_days[%d]: %d\n",
			today, g_num_next_report_day, 
			g_num_report_days[g_num_next_report_day]);
#endif

		//if(g_num_day-1 == g_num_report_days[g_num_next_report_day])
		{
			num_print(g_num_report_days[g_num_next_report_day-1]);

			if (g_num_next_report_day < g_num_nreport_days+1)
				g_num_next_report_day++;
		}
	} else if((g_num_day+1) % g_num_mod == 0)
	{
		num_print(g_num_day-g_num_mod);
	}
}
