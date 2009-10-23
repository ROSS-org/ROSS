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
 *   Percent at level 1 (above acceptible rate)
 *   Percent at level 2 (below acceptible rate)
 *   Percent at level 3 (cannot connect)
 *   Total agents attempting to use network
 *   Total Agents achieving level 0
 *   Total Agents achieving level 1
 *   Total Agents achieving level 2
 *   Total Agents achieving level 3
 *   Total agent population utilizing network
 *******************************************/

void
num_print()
{
	int		i,j;
	long int	total;
	long int	total_users = 0;
	int		delta_day;

	delta_day = (g_num_mod > 0)?g_num_mod:1;

	for (i = 0; i < g_num_nprofiles; i++)
	{
		total = 0;
		for (j=0; j < NUM_LEVELS; j++)
			total += g_num_level[i][j];

		total_users += total;

		if(g_num_mod > g_num_day)
			fprintf(g_num_log_f, "%d %d", g_num_day, i);
		else
			fprintf(g_num_log_f, "%d %d", g_num_day-g_num_mod, i);

		for(j = 0; j < NUM_LEVELS; j++)
		{
			if(total && g_num_mod > 0)
			{
				fprintf(g_num_log_f, " %4.4lf", 
					100.0 * 
						((float) g_num_level[i][j] / 
						(float) total));
			} else if(g_num_mod == 0)
			{
				fprintf(g_num_log_f, " %4.4lf", 
					100.0 * 
						((float) g_num_level[i][j] / 
						(float) g_num_profile_cnts[j]));
			} else
				fprintf(g_num_log_f, " 0.0000");
		}

		if(g_num_mod > 0)
			fprintf(g_num_log_f," %ld", total);
		else
			fprintf(g_num_log_f," %d", g_num_profile_cnts[i]);
			

		for(j = 0; j < NUM_LEVELS; j++)
			fprintf(g_num_log_f, " %d", g_num_level[i][j]);

		fprintf(g_num_log_f, " %d\n", g_num_net_pop[i]);
	}

	if(total_users > g_num_max_users)
		g_num_max_users = total_users;

	for (i = 0; i < g_num_nprofiles; i++)
	{
		for(j = 0; j < NUM_LEVELS; j++)
			g_num_level[i][j] = 0;

		g_num_net_pop[i] = 0;
	}

}

// Bunch of conditionals for print num.log
// 1. only report for previous day
// 2. only first agent on a given day needs report
// 3. only report if logging enabled
// 4. only report after first MOD period expires
// 5. only report every Nth day (g_num_mod)
// Argh.
void
num_report_levels(int today)
{
	// same day, different agent
	if(today <= g_num_day)
		return;

	g_num_day = today;

	// log file off
	if(!g_num_log_f)
		return;

	// do not print until after first mod period has passed
	if(today < g_num_mod)
		return;

	// only print every g_num_mod days
	if((g_num_day+1) % g_num_mod == 0)
		num_print();
}
