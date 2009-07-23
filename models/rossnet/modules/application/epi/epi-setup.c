#include <epi.h>
#include <getopt.h>

/*
 * epi-setup.c - parse command line arguments, and setup global model variables
 */

/*
 * epi_usage - print the available command line arguments
 */
void
epi_usage(void)
{
	printf("==> ./test [--opt=val]+ \n");
	printf("\n");
	printf("Optional arguments:\n");
	printf("\tic: input file for infectivity stage data, default: ic.txt\n");
	printf("\tnsick: specify number of agents initially infected \n" \
		"\t\t [ ignored when reading from input file ]\n");
	printf("\toutput: output file for agent moves and status changes\n");
	printf("\n");

	exit(0);
}

/*
 * epi_setup_options - parse the command line arguments
 */
void
epi_setup_options(int argc, char ** argv)
{
	int	 	 c;

	char		 hospital_fn[1024];
	char		 log_fn[1024];

	g_epi_nsick = 0;
	g_epi_ic_fn = tw_calloc(TW_LOC, "", sizeof(char), 255);
	g_epi_position_fn = tw_calloc(TW_LOC, "", sizeof(char), 255);
	sprintf(hospital_fn, "%s/hospital.log", g_rn_logs_dir),
	sprintf(log_fn, "%s/epi.log", g_rn_logs_dir);

	printf("\nEpi User Specified Input Parameters: \n\n");

	opterr = 0;
	optind = 0;

	while(-1 != (c = getopt(argc, argv, "-W:U:h:T:D:S:I:B:M:2:")))
	{
		switch(c)
		{
			case 'I':
				strcpy(g_epi_ic_fn, optarg);
				printf("\t%-50s %11s\n", "IC File", g_epi_ic_fn);
				break;
			case 'S':
				g_epi_psick = atof(optarg);
				printf("\t%-50s %11f\n", "Probability Agents are Sick", g_epi_psick);
				break;
			case 'M':
				g_epi_position_fn = tw_calloc(TW_LOC, "", sizeof(char), 255);
				strcpy(g_epi_position_fn, optarg);
				printf("\t%-50s %11s\n", "Position (Agent Moves) File", g_epi_position_fn);
				g_epi_position_f = fopen(g_epi_position_fn, "w");
				if (!g_epi_position_f)
					tw_printf(TW_LOC, "Unable to create output file: %s \n",
						g_epi_position_fn);
				break;
			case 'W':
				g_epi_worried_well_rate = atof(optarg);
				printf("\t%-50s %11f\n", "Probability of being Worried Well", g_epi_worried_well_rate);
				break;
			case 'T':
				g_epi_worried_well_threshold = atof(optarg);
				printf("\t%-50s %11f\n", "Worried Well threshold", g_epi_worried_well_threshold);
				break;
			case 'U':
                                g_epi_mod = atoi(optarg);

				printf("\t%-50s %11d\n", 
					"Reporting Interval", 
					g_epi_mod);
				break;
			case 'D':
				g_epi_worried_well_duration = atoi(optarg);
				printf("\t%-50s %11d\n", "Worried Well duration", g_epi_worried_well_duration);
				break;
			case '2':
				g_epi_work_while_sick_p = atof(optarg);
				printf("\t%-50s %11f\n", "Probability agent will go to work while sick",
					g_epi_work_while_sick_p);
				break;
			case 'B':
				if ( strcmp(optarg, "HOME") == 0 )
				{
					g_epi_stay_home = 1;
					printf("\tAll agents stay home and use the network\n");
				}
				break;
			case 'h':
				sprintf(hospital_fn, "%s", optarg);
				break;
		}
	}

	if(0 == strcmp(g_epi_ic_fn, ""))
		sprintf(g_epi_ic_fn, "tools/%s/ic.txt", g_rn_tools_dir);

	/*
	 * Fix-up vars, open output files, etc
	 */
	if(NULL == (g_epi_log_f = fopen(log_fn, "w")))
		tw_error(TW_LOC, "Unable to open: %s \n", log_fn);

	if(NULL == (g_epi_hospital_f = fopen(hospital_fn, "w")))
		tw_error(TW_LOC, "Unable to open: %s \n", hospital_fn);

	printf("\n");
}
