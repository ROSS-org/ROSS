#include<epi.h>

void
epi_init_disease()
{
	FILE		*f = NULL;

	epi_disease	*d;
	epi_ic_stage	*s;

	unsigned int	 i;
	unsigned int	 j;

	double		 multiplier;

	char		 line[255];
	char		 fname[255];

	printf("EPI Disease Stage Initialization: %s\n\n", g_epi_ic_fn);

	if(NULL == (f = fopen(g_epi_ic_fn, "r")))
		tw_error(TW_LOC, "Unable to open IC Stage file");

	// first line of file is number of diseases
	fscanf(f, "%u", &g_epi_ndiseases);

	// setup vars dependant on number of diseases
	g_epi_diseases = tw_calloc(TW_LOC, "", sizeof(epi_disease), g_epi_ndiseases);
	g_epi_log_f = tw_calloc(TW_LOC, "", sizeof(FILE *), g_epi_ndiseases);

	g_epi_exposed_today = tw_calloc(TW_LOC, "", sizeof(unsigned int), g_epi_ndiseases);
	g_epi_sick_init_pop = tw_calloc(TW_LOC, "", sizeof(unsigned int), g_epi_ndiseases);

	for(i = 0; i < g_epi_ndiseases; i++)
	{
		d = &g_epi_diseases[i];

		// first line of disease is number of stages
		fscanf(f, "%u", &d->nstages);

		if (!d->nstages)
			tw_error(TW_LOC, "No stages in ic file");

		// incr for stage 0
		d->nstages++; 
		d->stages = tw_calloc(TW_LOC, "", sizeof(epi_ic_stage), d->nstages);

		/*
		 * Stage 0 is Susceptible.  All agents who are not initially infected
		 * start in this stage.  This is the default.  Its values are not
		 * in the input file, but are set during agent initialization.
		 * Agents who are infected at initialization are put into stage 1.
		 */
		s = &d->stages[0];
		s->index = 0;
		strcpy(s->name, "Susceptible");
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

		j = 1;
		s = &d->stages[j];

#if VERIFY_EPI
		printf("EPSILON: %e\n", EPSILON);
#endif

		fscanf(f, "%s", d->name);
		printf("\tDisease %d: %s\n\n", i+1, d->name);

		sprintf(fname, "%s.log", d->name);
		if(NULL == (g_epi_log_f[i] = fopen(fname, "w")))
			tw_error(TW_LOC, "Unable to open: %s \n", log);

		while(EOF != fscanf(f, "%u", &s->index))
		{
			if(0 == s->index)
				tw_error(TW_LOC, "IC should not have stage 0");
	
			if(j != s->index)
				tw_error(TW_LOC, "IC stage %d out of order-expected: %d\n",
					 s->index, j);

			fscanf(f, "%s", s->name);
	
			//printf("\tStage Name: %s\n",s->name);
	
			if (EOF == fscanf(f, "%lf", &s->start_multiplier))
				tw_error(TW_LOC, "File Error!");
	
			if (EOF == fscanf(f, "%lf", &s->stop_multiplier))
				tw_error(TW_LOC, "File Error!");
	
			if (EOF == fscanf(f, "%lf", &s->min_duration))
				tw_error(TW_LOC, "File Error!");
	
			if (EOF == fscanf(f, "%lf", &s->max_duration))
				tw_error(TW_LOC, "File Error!");
	
			if (s->min_duration > s->max_duration)
				tw_error(TW_LOC, "stage min duration > max duration");
	
			// Multiply by secons in a day
			s->min_duration *= 86400.0;
			s->max_duration *= 86400.0;
	
			if (EOF == fscanf(f, "%lf", &s->mortality_rate))
				tw_error(TW_LOC, "File Error!");

			fscanf(f, "%s", line);
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
	
			fscanf(f, "%s", line);
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
	
			fscanf(f, "%s", line);
			if (0 == strcmp(line, "true"))
				s->hospital_treatment = TW_TRUE;
			else if (0 == strcmp(line, "false"))
				s->hospital_treatment = TW_FALSE;
			else
				tw_error(TW_LOC, "invalid input for hospital_treatment: %s\n", line);
	
#if VERIFY_EPI
			printf("\thospital treatment: ");
			if (s->hospital_treatment == TW_TRUE)
				printf(" TRUE\n");
			else
				printf(" FALSE\n");
#endif

			fscanf(f, "%s", line);
			if (0 == strcmp(line, "true"))
				s->is_symptomatic = TW_TRUE;
			else if (0 == strcmp(line, "false"))
				s->is_symptomatic = TW_FALSE;
			else
				tw_error(TW_LOC, "invalid input for s_symptomatic: %s\n", line);
	
#if VERIFY_EPI
			printf("\tis symptomatic: ");
			if (s->is_symptomatic == TW_TRUE)
				printf(" TRUE\n");
			else
				printf(" FALSE\n");
#endif

			if (EOF == fscanf(f, "%lf", &s->days_in_hospital))
				tw_error(TW_LOC, "File Error!");

			s->days_in_hospital *= 86400.0;
	
			// Calculate log of multiplier.  
			// Right now, using the average of start_multiplier
			// and stop_multriplier.
			multiplier = (s->start_multiplier + s->stop_multiplier) / 2.0;
	
			if (multiplier > EPSILON)
				s->ln_multiplier = log(1 - multiplier);
			else
				s->ln_multiplier = 0.0;
	
			// Now print for log
			printf("\tStage %d: %s\n", s->index, s->name);
	
			printf("\t\t%-40s %11.2lf\n", "Min Duration", s->min_duration / 86400.0);
			printf("\t\t%-40s %11.2lf\n", "Max Duration", s->max_duration / 86400.0);
	
			if(s->mortality_rate)
				printf("\t\t%-40s %11.2lf\n", "Mortality Rate", s->mortality_rate);
	
			if (s->start_multiplier > EPSILON)
			{
				printf("\t\t%-40s %.9lf\n", "Start Mult", s->start_multiplier);
				printf("\t\t%-40s %.9lf\n", "Stop Mult", s->stop_multiplier);
			}
	
			if(s->ln_multiplier)
				printf("\t\t%-45s %.4g\n", "LN Mult", s->ln_multiplier);
	
			if(s->days_in_hospital)
				printf("\t\t%-40s %11.2lf\n", "Days in Hospital", 
						s->days_in_hospital / 86400.0);
			printf("\n");

			if(j == d->nstages-1)
				break;

			s = &d->stages[++j];
		}
	}

	if(f)
		fclose(f);
}

void
epi_init_disease_default()
{
	epi_disease	*d;
	epi_ic_stage	*s;

	double		 multiplier;

	double		 stages[1][4][11] = 
			 {
				{
					{ (double) 'S', 
					  0.0, 0.0, 1.0, 5.0, 0.0,
					  1.0, 0.0, 0.0, 0.0, 0.0, },
	
					{ (double) 'E', 
					  0.0, 0.0, 1.0, 5.0, 0.0,
					  1.0, 0.0, 0.0, 0.0, 0.0, },
	
					{ (double) 'I', 
					  0.00004, 0.00004, 2.0, 5.0, 0.0,
					  1.0, 0.0, 1.0, 1.0, 0.0, },
	
					{ (double) 'R', 
					  0.0, 0.0, 1000.0, 1000.0, 50.0,
					  0.0, 0.0, 0.0, 0.0, 0.0, },
				},
			 };

	unsigned int	 i;
	unsigned int	 j;
	unsigned int	 k;

	char		 fname[512];

	printf("EPI Disease Stage Initialization: default\n\n");

	g_epi_ndiseases = 1;
	g_epi_diseases = tw_calloc(TW_LOC, "", sizeof(epi_disease), g_epi_ndiseases);
	g_epi_log_f = tw_calloc(TW_LOC, "", sizeof(FILE *), g_epi_ndiseases);

	g_epi_exposed_today = tw_calloc(TW_LOC, "", sizeof(unsigned int), g_epi_ndiseases);
	g_epi_sick_init_pop = tw_calloc(TW_LOC, "", sizeof(unsigned int), g_epi_ndiseases);

	for(i = 0; i < g_epi_ndiseases; i++)
	{
		d = &g_epi_diseases[i];

		d->nstages = 4;
		d->stages = tw_calloc(TW_LOC, "", sizeof(epi_ic_stage), d->nstages);

		strcpy(d->name, "Influenza");
		printf("\tDisease %d: %s\n\n", i+1, d->name);

		sprintf(fname, "%s.log", d->name);
		if(NULL == (g_epi_log_f[i] = fopen(fname, "w")))
			tw_error(TW_LOC, "Unable to open: %s \n", log);

		for(j = 0, k = 0; j < d->nstages; j++, k = 0)
		{
			s = &d->stages[j];

			s->index = j+1;
			s->name[0] = (char) (stages[i][j][k++]);
			s->name[1] = '\0';
			s->start_multiplier = stages[i][j][k++];
			s->stop_multiplier = stages[i][j][k++];
			s->min_duration = stages[i][j][k++];
			s->max_duration = stages[i][j][k++];
			s->mortality_rate = stages[i][j][k++];
			s->eligible_for_inoculation = 
				stages[i][j][k++] == 0.0 ? 0.0 : 1.0;
			s->progress_trumps_inoculation = 
				stages[i][j][k++] == 0.0 ? 0.0 : 1.0;
			s->hospital_treatment = 
				stages[i][j][k++] == 0.0 ? 0.0 : 1.0;
			s->is_symptomatic = 
				stages[i][j][k++] == 0.0 ? 0.0 : 1.0;
			s->days_in_hospital = stages[i][j][k++];
			
			/*
			 * Error checking
			 */
			if (s->min_duration > s->max_duration)
				tw_error(TW_LOC, "stage min duration > max duration");

			/*
			 * Fix-ups & Conversions
			 */	
			// convert days to seconds
			s->min_duration *= 86400.0;
			s->max_duration *= 86400.0;
			s->days_in_hospital *= 86400.0;

			// Calculate log of multiplier.  
			// Right now, using the average of start_multiplier
			// and stop_multriplier.
			multiplier = (s->start_multiplier + s->stop_multiplier) / 2.0;
	
			if (multiplier > EPSILON)
				s->ln_multiplier = log(1 - multiplier);
			else
				s->ln_multiplier = 0.0;
	
			// Now print for log
			printf("\t\tStage %d: %s\n", s->index, s->name);
			printf("\t\t\t%-40s %11.2lf\n", "Min Duration", 
					s->min_duration / 86400.0);
			printf("\t\t\t%-40s %11.2lf\n", "Max Duration", 
					s->max_duration / 86400.0);
	
			if(s->mortality_rate)
				printf("\t\t\t%-40s %11.2lf\n", "Mortality Rate", 
					s->mortality_rate);
	
			if (s->start_multiplier > EPSILON)
			{
				printf("\t\t\t%-40s %.9lf\n", "Start Multiplier", 
					s->start_multiplier);
				printf("\t\t\t%-40s %.9lf\n", "Stop Multiplier", 
					s->stop_multiplier);
			}
	
			if(s->ln_multiplier)
				printf("\t\t\t%-45s %.4g\n", "LN Mult", 
					s->ln_multiplier);
	
			if(s->days_in_hospital)
				printf("\t\t\t%-40s %11.2lf\n", "Days in Hospital", 
						s->days_in_hospital / 86400.0);
			printf("\n");
		}
	}
}

void
epi_init_agent(void)
{
	FILE		*g;
	FILE		*f;
	fpos_t		 pos;

	tw_memory	*b;
	tw_memory	*next;

	epi_agent	*a;

	int		*home_to_ct;
	int		 nagents = 0;
	int		 nlp = -1;
	int		 i;
	int		 j;
	int		 h;
	int		 o;
	int		 t;

	printf("\nEPI Agent Initialization: \n\n");

	home_to_ct = tw_calloc(TW_LOC, "home to ct", sizeof(*home_to_ct), 3363607);
	// create census tract table, hard-coded to 2214 (all CT) for Chicago
	g_epi_nregions = 2215;
	g_epi_regions = tw_calloc(TW_LOC, "", sizeof(unsigned int *), g_epi_nregions);

	// need to read in homes.dat to get CT ids
	if(NULL == (g = fopen("homes.dat", "r")))
		tw_error(TW_LOC, "Unable to open: homes.dat");

	i = 0;
	while(EOF != (fscanf(g, "%d", &home_to_ct[i++])))
		;

	for(i = 0; i < g_epi_nregions; i++)
	{
		g_epi_regions[i] = tw_calloc(TW_LOC, "", sizeof(unsigned int *), g_epi_ndiseases);

		for(j = 0; j < g_epi_ndiseases; j++)
			g_epi_regions[i][j] = tw_calloc(TW_LOC, "", sizeof(unsigned int), 
						g_epi_diseases[j].nstages);
	}

	if(NULL == (f = fopen("agents.dat", "r")))
		tw_error(TW_LOC, "Unable to open: agents.dat");

	i = 0;

	if(!g_epi_nagents)
	{
		fgetpos(f, &pos);
		while(EOF != fscanf(f, "%d %d %*d", &h, &o))
			g_epi_nagents++;
		fsetpos(f, &pos);
	}

	if(0 == g_epi_nagents)
		tw_error(TW_LOC, "No agents in agents.dat!");

	g_epi_agents = tw_calloc(TW_LOC, "", sizeof(tw_memoryq), 1);
	g_epi_agents->start_size = g_epi_nagents;
	g_epi_agents->d_size = sizeof(epi_agent);
	g_epi_agents->grow = 1;

	tw_memory_allocate(g_epi_agents);

	b = g_epi_agents->head;
	while(EOF != (fscanf(f, "%d %d %d", &h, &o, &t)))
	{
		// allocate the agent
		a = tw_memory_data(b);
		a->id = nagents++;

		if(h && h > nlp)
			nlp = h;

		if(o != -1 && o > nlp)
			nlp = o;

		// the CT id is stored on the h-th line of the homes.dat file
		a->region = home_to_ct[h];

		if(a->region > 2214)
			tw_error(TW_LOC, "Bad Home to CT Mapping!");

		a->pathogens = NULL;

#if 0
		// default disease stats
		a->stage = EPI_SUSCEPTIBLE;
		a->ts_stage_tran = DBL_MAX;
#endif

		a->curr = 0;
		//a->ts_last_tran = 0.0;

		// go to work at 9am on first day
		a->ts_next = a->ts_remove = (double) 32400.0;
		//a->n_infected = 0;

		if(-1 != o)
			a->nloc = 3;
		else
			a->nloc = 2;

		// only two locs =(
		a->loc[1] = o;
		a->loc[0] = a->loc[a->nloc-1] = h;

		a->dur[0] = 9 * 3600;
		a->dur[1] = 8 * 3600;
		a->dur[a->nloc-1] += 7 * 3600;

		a->behavior_flags = 0;
		a->ts_remove = a->ts_next = a->dur[a->curr];

		for(i = 0; i < g_epi_ndiseases; i++)
			g_epi_regions[a->region][i][0]++;

		if(g_epi_nagents == nagents)
			break;

		b = b->next;
	}

	if(nlp == 0)
		tw_error(TW_LOC, "No locations!");
	else
		printf("\t%-48s %11d\n", "Max Location", ++nlp);

	g_epi_pq = tw_calloc(TW_LOC, "", sizeof(void *), nlp);

	for(i = 0; i < nlp; i++)
		g_epi_pq[i] = pq_create();

	for(b = g_epi_agents->head; b; b = next)
	{
		a = tw_memory_data(b);
		next = b->next;
		pq_enqueue(g_epi_pq[a->loc[0]], b);
	}

	g_tw_nlp = nlp;

}

void
epi_init_agent_default(void)
{
	tw_memory	*b;

	epi_agent	*a;

	int		 i;
	int		 j;

	int		 id;
	int		 lid;
	int		 rid;

	if(!g_epi_nagents)
		tw_error(TW_LOC, "No agents specified!");

	if(!g_tw_nlp)
		tw_error(TW_LOC, "No locations specified!");

	if(!g_epi_nregions)
		tw_error(TW_LOC, "No regions specified!");

	g_epi_regions = tw_calloc(TW_LOC, "", sizeof(unsigned int *), g_epi_nregions);

	// create reporting tables for each region, for each disease
	for(i = 0; i < g_epi_nregions; i++)
	{
		g_epi_regions[i] = tw_calloc(TW_LOC, "", sizeof(*g_epi_regions[i]), 
						g_epi_ndiseases);

		for(j = 0; j < g_epi_ndiseases; j++)
			g_epi_regions[i][j] = tw_calloc(TW_LOC, "", 
							sizeof(*g_epi_regions[i][j]), 
							g_epi_diseases[j].nstages);
	}

	// allocate the location priority queues for this node
	g_epi_pq = tw_calloc(TW_LOC, "", sizeof(*g_epi_pq), g_tw_nlp);

	for(i = 0; i < g_tw_nlp; i++)
		g_epi_pq[i] = pq_create();

	// round-robin mapping of agents to locations, and locations to regions
	for(i = 0; i < g_epi_nagents; i++)
	{
		lid = i % g_tw_nlp;
		rid = lid % g_epi_nregions;

		b = tw_memory_alloc(g_tw_lp[lid], g_epi_fd);
		a = tw_memory_data(b);

		a->id = id++;
		a->region = rid;

		a->pathogens = NULL;
		a->curr = 0;
		a->nloc = tw_rand_integer(g_tw_lp[lid]->rng, 0, 10);

		// setup "home" location
		a->loc[0] = lid;
		a->dur[0] = tw_rand_exponential(g_tw_lp[lid]->rng, g_epi_mean);
		a->ts_next = a->ts_remove = a->dur[0];
		pq_enqueue(g_epi_pq[a->loc[0]], b);

		printf("A %d nloc %d: (%d, %lf) ", 
			a->id, a->nloc, a->loc[0], a->dur[0]);

		for(j = 1; j < a->nloc; j++)
		{
			a->loc[j] = tw_rand_integer(g_tw_lp[lid]->rng, 0, 
					(g_tw_nlp * tw_nnodes()) - 2);
			a->dur[j] = tw_rand_exponential(g_tw_lp[lid]->rng,
					g_epi_mean);

			printf("(%d %lf) ", a->loc[j], a->dur[j]);
		}

		printf("\n");
		ga = a;
	}
}

void
epi_init_hospital()
{
	int	 i;

	g_epi_nhospital = 1;
	g_epi_hospital = tw_calloc(TW_LOC, "", sizeof(unsigned int *), 1);
	g_epi_hospital_ww = tw_calloc(TW_LOC, "", sizeof(unsigned int *), 1);
	g_epi_hospital_wws = tw_calloc(TW_LOC, "", sizeof(unsigned int *), 1);

	if(NULL == (g_epi_hospital_f = fopen(g_epi_hospital_fn, "w")))
		tw_error(TW_LOC, "Unable to open for writing: %s", g_epi_hospital_fn);

	for(i = 0; i < g_epi_nhospital; i++)
	{
		g_epi_hospital[i] = tw_calloc(TW_LOC, "", sizeof(unsigned int), 1);
		g_epi_hospital_ww[i] = tw_calloc(TW_LOC, "", sizeof(unsigned int), 1);
		g_epi_hospital_wws[i] = tw_calloc(TW_LOC, "", sizeof(unsigned int), 1);
	}
}

void
epi_disease_init()
{
	g_epi_stats = tw_calloc(TW_LOC, "Global EPI stats", sizeof(*g_epi_stats), 1);

	// initialize disease configurations
	if(0 == strcmp("", g_epi_ic_fn))
		epi_init_disease_default();
	else
		epi_init_disease();

	// initialize hospitals (statistics gathering)
	epi_init_hospital();
}

/*
 * scenario initialization
 * must initialize ic stages before initializing agents.
 */
void
epi_agent_init()
{
	// initialize the agents (events)
	if(0 == strcmp("", g_epi_agent_fn))
		epi_init_agent_default();
	else
		epi_init_agent();
}

void
epi_model_final()
{
	int	 i;

	for(i = 0; i < g_epi_ndiseases; i++)
		if(g_epi_log_f && g_epi_log_f[i])
			fclose(g_epi_log_f[i]);

	if(!tw_ismaster())
		return;

	printf("\nPandemic Flu Initial State Variables: \n");
	printf("\n");
	printf("\t%-50s %11d\n", "Total Population", g_epi_nagents);
	printf("\t%-50s\n", "Initial Sick Populations:");

	for(i = 0; i < g_epi_ndiseases; i++)
		printf("\t\tDisease %-40d %5d\n", i, g_epi_sick_init_pop[i]);

	printf("\t%-50s %11d\n", "Initial Worried Well Pop", g_epi_ww_init_pop);
	printf("\t%-50s %11d\n", "Initial Work While Sick Pop", g_epi_wws_init_pop);
	printf("\n");
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

#if 0
	printf("\t%-50s %11ld\n", "Ttl Move Events", g_epi_stats->s_move_ev);
	printf("\t%-50s %11ld\n", "Ttl Agents Checked for Infection", g_epi_stats->s_nchecked);
	printf("\t%-50s %11ld\n", "Ttl Draws from Uniform Random Distribution", g_epi_stats->s_ndraws);
#endif
}
