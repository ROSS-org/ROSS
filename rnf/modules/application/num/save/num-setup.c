#include <num.h>
#include <getopt.h>

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
	int	 	 c;

	g_num_profiles_fn = tw_vector_create(sizeof(char), 255);

	// should make the log file an input option
	// log file has day, agent_id, type_id, throughput, desired_throughput, failures
	g_num_log_fn = tw_vector_create(sizeof(char), 255);
	sprintf(g_num_log_fn, "%s/num.log", g_rn_logs_dir);

	printf("\nNetwork User Specified Input Parameters: \n\n");

	opterr = 0;
	optind = 0;
	while((c = getopt(argc, argv, "-P:U:B:")) != -1)
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
			default: ;
		}
	}

	if(0 == strcmp(g_num_profiles_fn, ""))
		sprintf(g_num_profiles_fn, "tools/%s/profile.txt", g_rn_tools_dir);

}
