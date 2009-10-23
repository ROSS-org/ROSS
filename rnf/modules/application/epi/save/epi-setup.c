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
	int		 cnt = 0;

	g_epi_nsick = 0;
	g_epi_ic_fn = tw_vector_create(sizeof(char), 255);
	g_epi_position_fn = tw_vector_create(sizeof(char), 255);

	printf("\nEpi User Specified Input Parameters: \n\n");

	opterr = 0;
	optind = 0;

	while(-1 != (c = getopt(argc, argv, "-W:T:D:S:I:M:2:")))
	{
		cnt = 1;

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
				g_epi_position_fn = tw_vector_create(sizeof(char), 255);
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
			case 'D':
				g_epi_worried_well_duration = atoi(optarg);
				printf("\t%-50s %11d\n", "Worried Well duration", g_epi_worried_well_duration);
				break;
			case '2':
				g_epi_work_while_sick_p = atof(optarg);
				printf("\t%-50s %11f\n", "Probability agent will go to work while sick",
					g_epi_work_while_sick_p);
				break;
			case '?':
			default: ;
		}
	}

	if(!cnt)
		printf("\t%-50s\n", "Using default variables");

	if(0 == strcmp(g_epi_ic_fn, ""))
		sprintf(g_epi_ic_fn, "tools/%s/ic.txt", g_rn_tools_dir);

	/*
	 * Fix-up vars
	 */
		
	printf("\n");
}
