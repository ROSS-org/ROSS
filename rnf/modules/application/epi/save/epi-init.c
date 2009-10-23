#include<epi.h>

// specify 0.5 of the overall range
#define EPI_RANGE	50

void
init_default_ic_stages()
{
	epi_ic_stage	*s;

	g_epi_stages = tw_vector_create(sizeof(epi_ic_stage), 1);

	/*
	 * Stage 0 is Susceptible.  All agents who are not initially infected
	 * start in this stage.  This is the default.  Its values are not
	 * in the input file, but are set during agent initialization.
	 * Agents who are infected at initialization are put into stage 1.
	 */

	/* Initialize susceptible stage */
	s = &g_epi_stages[0];
	s->stage_index = 0;
	s->stage_name = tw_vector_create(sizeof(char), 12);
	strcpy(s->stage_name,"Susceptible");
	s->start_multiplier = 0.0;
	s->stop_multiplier = 0.0;
	s->min_duration = DBL_MAX;
	s->max_duration = DBL_MAX;
	s->mortality_rate = 0.0;
	s->eligible_for_inoculation = TW_TRUE;
	s->progress_trumps_inoculation = TW_FALSE;
	s->hospital_treatment = TW_FALSE;
	s->is_symptomatic = TW_FALSE;
	s->days_in_hospital = 0.0;
};

void
init_ic_stages_from_file()
{
	FILE		*f = NULL;

	epi_ic_stage	*s;

	unsigned int	 i;

	double		multiplier;

	char		*line;

	printf("IC Stage Initialization: \n\n");

	f = fopen(g_epi_ic_fn, "r");

	if (!f)
		tw_error(TW_LOC, "Unable to open IC Stage file %s!", g_epi_ic_fn);

	// first line is number of stages
	fscanf(f, "%u", &g_epi_nstages);

	if (!g_epi_nstages)
		tw_error(TW_LOC, "No stages in %d", g_epi_nstages);

	//printf("Number of stages: %d\n\n", g_epi_nstages);

	g_epi_nstages++; /* Increment for stage 0 */

	g_epi_stages = tw_vector_create(sizeof(epi_ic_stage), g_epi_nstages);

	/*
	 * Stage 0 is Susceptible.  All agents who are not initially infected
	 * start in this stage.  This is the default.  Its values are not
	 * in the input file, but are set during agent initialization.
	 * Agents who are infected at initialization are put into stage 1.
	 */
	i = 0;
	s = &g_epi_stages[0];
	s->stage_index = 0;
	s->stage_name = tw_vector_create(sizeof(char), 12);
	strcpy(s->stage_name,"Susceptible");
	s->start_multiplier = 0.0;
	s->stop_multiplier = 0.0;
	s->min_duration = DBL_MAX;
	s->max_duration = DBL_MAX;
	s->mortality_rate = 0.0;
	s->eligible_for_inoculation = TW_TRUE;
	s->progress_trumps_inoculation = TW_FALSE;
	s->hospital_treatment = TW_FALSE;
	s->is_symptomatic = TW_FALSE;
	s->days_in_hospital = 0.0;

	i = 1;
	s = &g_epi_stages[i];
	line = tw_vector_create(1, 1024);

	// printf("  EPSILON: %e\n", EPSILON);

	while(EOF != fscanf(f, "%u", &s->stage_index))
	{
		if(0 == s->stage_index)
			tw_error(TW_LOC, "IC should not have stage 0");

		if(i != s->stage_index)
			tw_error(TW_LOC, "IC stage %d out of order; expected :d\n",s->stage_index, i);

		//printf("\tStage: %d\n", s->stage_index);

		line = fgets(line, 1022, f); /* discard eol */
		line = fgets(line, 1022, f);
		line[strlen(line)-1] = '\0';

		s->stage_name = tw_vector_create(sizeof(char) * (strlen(line)+1), 1);
		strcpy(s->stage_name, line);

		//printf("\tStage Name: %s\n",s->stage_name);

		if (EOF == fscanf(f, "%lf", &s->start_multiplier))
			tw_error(TW_LOC, "File Error!");

		if (EOF == fscanf(f, "%lf", &s->stop_multiplier))
			tw_error(TW_LOC, "File Error!");

		if (s->start_multiplier > EPSILON)
			printf("Stage %2d:\n\tstart multiplier: %10.8f\n\t stop multiplier: %10.8f\n", s->stage_index,
				s->start_multiplier, s->stop_multiplier);

		if (EOF == fscanf(f, "%lf", &s->min_duration))
			tw_error(TW_LOC, "File Error!");

		//printf("\tmin duration: %f\n", s->min_duration);

		if (EOF == fscanf(f, "%lf", &s->max_duration))
			tw_error(TW_LOC, "File Error!");

		s->min_duration *= 86400.0;
		s->max_duration *= 86400.0;

		//printf("\tmax duration: %f\n", s->max_duration);

		if (EOF == fscanf(f, "%lf", &s->mortality_rate))
			tw_error(TW_LOC, "File Error!");

		//printf("\tmortality rate: %f\n", s->mortality_rate);

		line = fgets(line, 1022, f); /* discard eol */
		line = fgets(line, 1022, f);
		line[strlen(line)-1] = '\0';
		if (0 == strcmp(line, "true"))
			s->eligible_for_inoculation = TW_TRUE;
		else if (0 == strcmp(line, "false"))
			s->eligible_for_inoculation = TW_FALSE;
		else
			tw_error(TW_LOC, "invalid input for eligible_for_inoculation: %s\n", line);

		//printf("\teligible for inoculation: ");
		//if (s->eligible_for_inoculation == TW_TRUE)
		//	printf(" TRUE\n");
		//else
		//	printf(" FALSE\n");

		line = fgets(line, 1022, f);
		line[strlen(line)-1] = '\0';
		if (0 == strcmp(line, "true"))
			s->progress_trumps_inoculation = TW_TRUE;
		else if (0 == strcmp(line, "false"))
			s->progress_trumps_inoculation = TW_FALSE;
		else
			tw_error(TW_LOC, "invalid input for progress_trumps_inoculation: %s\n", line);

		//printf("\tprogress trumps inoculation: ");
		//if (s->progress_trumps_inoculation == TW_TRUE)
		//	printf(" TRUE\n");
		//else
		//	printf(" FALSE\n");

		line = fgets(line, 1022, f);
		line[strlen(line)-1] = '\0';
		if (0 == strcmp(line, "true"))
			s->hospital_treatment = TW_TRUE;
		else if (0 == strcmp(line, "false"))
			s->hospital_treatment = TW_FALSE;
		else
			tw_error(TW_LOC, "invalid input for hospital_treatment: %s\n", line);

		//printf("\thospital treatment: ");
		//if (s->hospital_treatment == TW_TRUE)
		//	printf(" TRUE\n");
		//else
		//	printf(" FALSE\n");

		line = fgets(line, 1022, f);
		line[strlen(line)-1] = '\0';
		if (0 == strcmp(line, "true"))
			s->is_symptomatic = TW_TRUE;
		else if (0 == strcmp(line, "false"))
			s->is_symptomatic = TW_FALSE;
		else
			tw_error(TW_LOC, "invalid input for s_symptomatic: %s\n", line);

		//printf("\tis symptomatic: ");
		//if (s->is_symptomatic == TW_TRUE)
		//	printf(" TRUE\n");
		//else
		//	printf(" FALSE\n");

		if (EOF == fscanf(f, "%lf", &s->days_in_hospital))
			tw_error(TW_LOC, "File Error!");

		s->days_in_hospital *= 86400.0;

		// Calculate log of multiplier.  Right now, using the average of start_multiplier
		// and stop_multriplier.
		multiplier = (s->start_multiplier + s->stop_multiplier) / 2.0;
		if (multiplier >= (1.0 - EPSILON))
			s->ln_multiplier = -25.0;
		if (multiplier > EPSILON)
			s->ln_multiplier = log(multiplier);
		else
			s->ln_multiplier = DBL_MIN;
		//printf("\tstage %d, multiplier: %e, ln_multiplier: %e\n",i, multiplier, s->ln_multiplier); 	
		//printf("\tdays in hospital: %f\n", s->days_in_hospital);

		//printf("\n");

		s = &g_epi_stages[++i];
	}
	if (i != g_epi_nstages)
		tw_error(TW_LOC, "Number of stages initialized (%d) does not equal nstages (%d)\n", i, g_epi_nstages);

	if (f)
		fclose(f);
}

/*
 * scenario initialization
 * must initialize ic stages before initializing agents.
 */
void
epi_init_scenario()
{
	if (0 == strcmp(g_epi_ic_fn, "default"))
		init_default_ic_stages();
	else
		init_ic_stages_from_file();
}
