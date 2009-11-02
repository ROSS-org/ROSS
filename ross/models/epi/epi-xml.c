#include <epi.h>

// Modified 4/30 ERB to add location for home at end of day
// Modified 5/16 ERB to add log file for ic stages by day
// Modified 5/22 ERB to change sick agent selection

void
epi_xml(epi_state * state, tw_lp * lp)
{
	xmlNodePtr	 agent;
	xmlNodePtr	 move;
	rn_machine	*m = rn_getmachine(lp->id);

	epi_agent	*a;
	epi_ic_stage	*s;

	double		 x;

	int		 ct;
	int		 i;

	char		*sick;

	// create census tract table if it does not exist
	// need 1 more than number of stages to save dead agents
	// dead is not a stage.
	if (!g_epi_ct)
	{
		g_epi_nct = 2200; //rn_getarea(m)->nsubnets;
		g_epi_ct = tw_vector_create(sizeof(unsigned int *), g_epi_nct);

		for (i = 0; i < g_epi_nct; i++)
			g_epi_ct[i] = tw_vector_create(sizeof(unsigned int), g_epi_nstages);
	}

	state->stats = tw_vector_create(sizeof(epi_statistics), 1);
	state->pq = pq_create();

	ct = rn_getsubnet(m)->id - rn_getarea(m)->subnets[0].id;

	// spin through agents 'homed' at this location and allocate + init them
	//
	// NOTE: May not have *any* agents homed at a given location
	for(agent = node->children; agent; agent = agent->next)
	{
		if(0 != strcmp((char *) agent->name, "agent"))
			continue;

		a = tw_vector_create(sizeof(epi_agent), 1);

		a->ct = m->uid; //ct;
		a->num = tw_getlp(atoi(xml_getprop(agent, "num")));
		//a->a_type = atoi(xml_getprop(agent,"type"));

		sick = xml_getprop(agent, "sick");
		if(g_epi_psick > EPSILON)
		{
			if ( tw_rand_unif(lp->rng) < g_epi_psick )
				sick = "1";
		}

		if(0 != strcmp(sick, "0"))
		{
			//a->ts_infected = atof(sick);
			a->stage = EPI_INCUBATING;

			s = &g_epi_stages[a->stage];

			x = ((double) tw_rand_integer(lp->rng, 0, INT_MAX) / (double) INT_MAX);
		
			a->ts_stage_tran = (s->min_duration + 
						(x * (s->max_duration - s->min_duration)));

#if EPI_XML_VERIFY
			printf("\tstage: %d, time %5f \n",
				s->stage_index, a->ts_stage_tran);
#endif

			g_epi_nsick++;
		} else
		{
			a->stage = EPI_SUSCEPTIBLE;
			a->ts_stage_tran = DBL_MAX;
			//a->ts_infected = DBL_MAX;
		}

		g_epi_ct[a->ct][a->stage]++;

		//a->id = g_epi_nagents++;
		g_epi_nagents++;
		a->curr = 0;
		//a->ts_last_tran = 0.0;
		a->ts_next = a->ts_remove = (double) 86400.0;
		//a->n_infected = 0;

		for(move = agent->children; move; move = move->next)
		{
			if(0 != strcmp((char *) move->name, "move"))
				continue;

			a->nloc++;
		}

		if(!a->nloc)
			tw_error(TW_LOC, "Agent has no locations!");

		a->nloc++; // add a location to return home at end of day

		a->loc = tw_vector_create(sizeof(int) * a->nloc, 1);
		a->dur = tw_vector_create(sizeof(tw_stime) * a->nloc, 1);

		int seconds_used = 0; // totals duration to check for 24 hours

		for(i = 0, move = agent->children; move; move = move->next)
		{
			if(0 != strcmp((char *) move->name, "move"))
				continue;

			a->loc[i] = atoi(xml_getprop(move, "loc"));
			a->dur[i] = atoi(xml_getprop(move, "dur")) * 3600;
			seconds_used += a->dur[i];
			i++;
		}

		if (seconds_used > 86400)
			tw_error(TW_LOC, "Agent has duration more than 24 hours\n");

		else if (seconds_used == 86400)
			a->nloc--; // do not need last location
		else {
			a->loc[a->nloc-1] = a->loc[0]; // return home
			a->dur[a->nloc-1] = 86400 - seconds_used;
		}

		a->behavior_flags = 0;
		if (g_epi_worried_well_rate > 0)
		{
			if (tw_rand_unif(lp->rng) < g_epi_worried_well_rate)
				a->behavior_flags = 1;
		} else if (g_epi_work_while_sick_p > EPSILON)
			if (tw_rand_unif(lp->rng) < g_epi_work_while_sick_p)
				a->behavior_flags = 2;

		// Set ts_next and enqueue
		a->ts_remove = a->dur[a->curr];
		if(a->ts_remove < a->ts_stage_tran)
			a->ts_next = a->ts_remove;
		else
			a->ts_next = a->ts_stage_tran;

		pq_enqueue(state->pq, a);
	}
}
