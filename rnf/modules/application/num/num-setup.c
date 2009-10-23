#include <num.h>
#include <getopt.h>

#define MAX_REPORT_DAYS 185

/*
 * num-setup.c - parse command line arguments, and setup global model variables
 */

/*
 * num_usage - print the available command line arguments
 */
void
num_usage(void)
{
	printf("==> ./test [--opt=val]+ \n");
	printf("\n");
	printf("Optional arguments:\n");
	printf("\tprofile: input file for network users profile, default: profile.txt\n");
	printf("\n");

	exit(0);
}

/*
 * num_setup_options - parse the command line arguments
 */
void
num_setup_options(int argc, char ** argv)
{
	int	c;
        int     report_days[MAX_REPORT_DAYS];
        int     nreport_days = 0;

	g_num_profiles_fn = tw_calloc(TW_LOC, "", sizeof(char), 255);
	g_num_day = 0;

	// should make the log file an input option
	// log file has day, agent_id, type_id, throughput, desired_throughput, failures
	g_num_log_fn = tw_calloc(TW_LOC, "", sizeof(char), 255);
	sprintf(g_num_log_fn, "%s/num.log", g_rn_logs_dir);

	printf("\nNetwork User Specified Input Parameters: \n\n");

	opterr = 0;
	optind = 0;
	while((c = getopt(argc, argv, "-P:U:B:Y:")) != -1)
	{
		switch(c)
		{
			case 'P':
				strcpy(g_num_profiles_fn, optarg);
				printf("\t%-50s %11s\n", 
					"Network User Profile", 
					g_num_profiles_fn);
				break;
			case 'U':
                                g_num_mod = atoi(optarg);

				printf("\t%-50s %11d\n", 
					"Reporting Interval", 
					g_num_mod);

				g_num_day = g_num_mod;

				if (g_num_nreport_days > 0)
                                        tw_error(TW_LOC, "Cannot have both 'U' and 'Y' options\n");
				break;
			case 'B':
				if (strcmp(optarg,"NUM") == 0)
				{
					g_num_debug = 1;
					printf("\tNUM Reporting throughput for first non-type 0 agent.\n");
				}
				else
					g_num_debug = 0;
				break;
                        case 'Y':
                                nreport_days++;
                                if (nreport_days >= MAX_REPORT_DAYS)
                                        tw_error(TW_LOC, "Asking for too many epi report days\n");
                                if (g_num_mod > 0)
                                        tw_error(TW_LOC, "Cannot have both 'U' option and 'Y' option\n");
                                report_days[nreport_days-1] = atoi(optarg);

				if(!g_num_day)
					g_num_day = atoi(optarg);
                                break;

			default: ;
		}
	}

	if(0 == strcmp(g_num_profiles_fn, ""))
		sprintf(g_num_profiles_fn, "tools/%s/profile.txt", g_rn_tools_dir);

	if (nreport_days > 0)
        {
                g_num_report_days = tw_calloc(TW_LOC, "", sizeof(int), nreport_days);
                g_num_nreport_days = nreport_days;
                printf("\tReport days: ");
                for (c = 0; c < nreport_days; c++)
                {
                        g_num_report_days[c] = report_days[c];
                        if (c > 0)
                                printf(", %d", report_days[c]);
                        else
                                printf("%d", report_days[c]);
                }
                printf("\n");
        }

        /*
         * Fix-up vars   
         */
                         
        printf("\n");

}
