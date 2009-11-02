#include <epi.h>

/*
 * epi.c -- defines the 4 simulation engine void * functions required for
 *		execution of the model.
 */

#define TWENTY_FOUR_HOURS 86400

tw_peid
epi_map(tw_lpid gid)
{
	return (tw_peid) gid / g_tw_nlp;
}

/*
 * epi_init - initialize node LPs state variables
 * 
 * state	- our node LP state space
 * lp		- our node LP
 */
void
epi_init(epi_state * state, tw_lp * lp)
{
	tw_memory	*b;
	tw_memory	*agent = NULL;
	tw_stime	 ts_min_tran = DBL_MAX;

	epi_agent	*a;
	epi_pathogen	*p;
	epi_ic_stage	*s;

	double		 x;

	int		 i;
	int		 j;
	int		 sz;

	// hard-coded, single hospital LP
	if(lp->id == g_tw_nlp-1)
	{
		fprintf(g_epi_hospital_f, "0 0 %u %u %u\n", 
			g_epi_hospital[0][0], g_epi_hospital_ww[0][0], 
			g_epi_hospital_wws[0][0]);

		g_epi_hospital[0][0] = 0;
		g_epi_hospital_ww[0][0] = 0;
		g_epi_hospital_wws[0][0] = 0;

		return;
	}

	// ID of the hospital I go to (or nearest if office/school)
	state->hospital = 0;
	state->stats = tw_calloc(TW_LOC, "", sizeof(epi_statistics), 1);
	state->ncontagious = tw_calloc(TW_LOC, "", sizeof(unsigned int), g_epi_ndiseases);
	state->ts_seir = tw_calloc(TW_LOC, "", sizeof(tw_stime), g_epi_ndiseases);

	for(i = 0; i < g_epi_ndiseases; i++)
		state->ts_seir[i] = DBL_MAX;

	// if no agents initially at this location, then we are done!
	if(0 == (sz = pq_get_size(g_epi_pq[lp->id])))
		return;

	// determine agents initial parameters / configuration
	for(i = 0; i < sz; i++)
	{
		agent = pq_next(g_epi_pq[lp->id], i);
		a = tw_memory_data(agent);

		for(j = 0; j < g_epi_ndiseases; j++)
		{
			if(!a->id && !j)
			{
				// ALLOCATE PATHOGEN AND FILL IT IN
				b = tw_memory_alloc(lp, g_epi_pathogen_fd);
				p = tw_memory_data(b);

				b->next = a->pathogens;
				a->pathogens = b;

				p->index = j;
				p->stage = EPI_SUSCEPTIBLE;
				p->ts_stage_tran = DBL_MAX;

				g_epi_regions[a->region][p->index][p->stage]--;
				p->stage = EPI_EXPOSED;
				g_epi_regions[a->region][p->index][p->stage]++;
				g_epi_exposed_today[j]++;
	
				// determine if agents start out sick.
				if(tw_rand_unif(lp->rng) < g_epi_sick_rate)
				{

				s = &g_epi_diseases[p->index].stages[p->stage];

				x = ((double) tw_rand_integer(lp->rng, 0, INT_MAX) / 
						(double) INT_MAX);

				p->ts_stage_tran = (s->min_duration + 
						(x * (s->max_duration - s->min_duration)));

				if(0.0 >= p->ts_stage_tran)
					tw_error(TW_LOC, "Immediate stage transition?!");

				//state->stats->s_ninfected++;
				state->ncontagious[p->index]++;
				state->ncontagious_tot++;
				//g_epi_hospital[0][0]++;
				g_epi_sick_init_pop[j]++;

				ts_min_tran = min(ts_min_tran, p->ts_stage_tran);

				if(!a->id)
					printf("%ld: agent exposed %d: %s, transition at %lf\n", 
					lp->id, a->id, g_epi_diseases[j].name, 
					p->ts_stage_tran);
				}
			}
		}

		// reflect ts_next change in PQ ordering
		if(ts_min_tran < a->ts_next)
		{
			a->ts_next = ts_min_tran;
			pq_delete_any(g_epi_pq[lp->id], &agent);
			pq_enqueue(g_epi_pq[lp->id], agent);
		}

		// determine if agents are a part of worried well population.
		if(g_epi_ww_rate)
		{
			if(tw_rand_unif(lp->rng) < g_epi_ww_rate)
			{
				a->behavior_flags = 1;
				g_epi_hospital_ww[0][0]++;
				g_epi_ww_init_pop++;
			}
		} else if(g_epi_wws_rate)
		{
			if(tw_rand_unif(lp->rng) < g_epi_wws_rate)
			{
				a->behavior_flags = 2;
				g_epi_hospital_wws[0][0]++;
				g_epi_wws_init_pop++;
			}
		}

		// define agent's at work contact population
		//g_epi_regions[a->region][a->stage]++;
	}

	epi_location_timer(state, lp);
}

/*
 * epi_event_handler - main event processing (per node LP)
 * 
 * state	- our node LP state space
 * bf		- a bitfield provided by the simulation ecxecutive
 * m		- the incoming event or message
 * lp		- our node LP
 */
void
epi_event_handler(epi_state * state, tw_bf * bf, epi_message * msg, tw_lp * lp)
{
	tw_memory	*buf;
	tw_memory	*b;

	epi_agent	*a;

	int		 population;
	int		 ncontagious[g_epi_ndiseases];
	int		 today;
	int		 pq_clean = 0;
	int		 i;

	// TO DO: need to check for last day to get last day of statistics
	today = (int) (tw_now(lp) / TWENTY_FOUR_HOURS);
	population = pq_get_size(g_epi_pq[lp->id]);

	for(i = 0; i < g_epi_ndiseases; i++)
		ncontagious[i] = state->ncontagious[i];

	if(today > g_epi_day)
	{
		// report yesterday's statistics
		epi_report_census_tract(today); 
		epi_report_hospitals(today);

		g_epi_day++;

		printf("%ld: DAY %d \n", lp->id, today);
	}

	if(g_epi_complete)
		return;

	// may return NULL in case of EPI_REMOVE
	if(NULL != (buf = tw_event_memory_get(lp)))
	{
		a = tw_memory_data(buf);

		while(NULL != (b = tw_event_memory_get(lp)))
		{
			b->next = a->pathogens;
			a->pathogens = b;
		}

		epi_agent_add(state, bf, buf, lp);

		printf("%ld: ADD %d at %lf \n", lp->id, a->id, tw_now(lp));
	} else
	{
		epi_agent_remove(state, bf, lp);
	}

	// Bring SEIR computation up to date for the agents at this location
	// Ok to skip if no change to infectious population, 
	// or no change to overall location population
	//
	// basically, avoid calling epi_seir_update, like it was the plague,
	// pun intended.
	for(i = 0; i < g_epi_ndiseases; i++)
	{
		if((population != pq_get_size(g_epi_pq[lp->id]) && state->ncontagious[i]) ||
		   ncontagious[i] != state->ncontagious[i])
		{
			epi_seir_compute(state, bf, i, lp);
			pq_clean = 1;
		}
	}

	if(pq_clean)
		pq_cleanup(g_epi_pq[lp->id]);

	epi_location_timer(state, lp);
}

/*
 * epi_rc_event_handler - reverse compute effects of out-of-order events
 * 			  + rollback LP random number generators
 *			  + restore LP state variables
 * 
 * state	- our node LP state space
 * bf		- a bitfield provided by the simulation ecxecutive
 * m		- the incoming event or message
 * lp		- our node LP
 */
void
epi_rc_event_handler(epi_state * state, tw_bf * bf, epi_message * msg, tw_lp * lp)
{
}

/*
 * epi_final - finalize node LP statistics / outputting
 * 
 * state	- our node LP state space
 * lp		- our node LP
 */
void
epi_final(epi_state * state, tw_lp * lp)
{
	if(lp->id == g_tw_nlp-1)
		return;

	if(g_epi_day)
	{
		epi_report_census_tract(g_epi_day); 
		epi_report_hospitals(g_epi_day);
		g_epi_day = 0;
	}

	//g_epi_stats->s_move_ev += state->stats->s_move_ev;
	//g_epi_stats->s_nchecked += state->stats->s_nchecked;
	//g_epi_stats->s_ndraws += state->stats->s_ndraws;
	g_epi_stats->s_ninfected += state->stats->s_ninfected;
	g_epi_stats->s_ndead += state->stats->s_ndead;
}

tw_lptype       mylps[] = {
	{(init_f) epi_init,
	 (event_f) epi_event_handler,
	 (revent_f) epi_rc_event_handler,
	 (final_f) epi_final,
	 (map_f) epi_map,
	 sizeof(epi_state)},
	{0},
};

const tw_optdef app_opt[] =
{
	TWOPT_GROUP("EPI Model: "),

	// output files
	TWOPT_CHAR("positions", g_epi_position_fn, "Agent position output file"),
	TWOPT_CHAR("hospital-output", g_epi_hospital_fn, "Hospital output file name"),
	TWOPT_UINT("report-int", g_epi_mod, "Reporting Interval"),

	// agent parameters
	TWOPT_STIME("sick-rate", g_epi_sick_rate, "Probability agents start sick"),
	TWOPT_CHAR("worried-well", g_epi_ww_rate, "Probability agents are worried well"),
	TWOPT_STIME("wwell-thresh", g_epi_ww_threshold, "Worried well threshold"),
	TWOPT_UINT("wwell-dur", g_epi_ww_duration, "Worried well duration"),
	TWOPT_STIME("work-while-sick", g_epi_wws_rate, "Probability agents work while sick"),

	// init model algorithmically
	TWOPT_GROUP("EPI Model: initialize algorithmically (default mode)"),
	TWOPT_UINT("nagents", g_epi_nagents, "number of agents per location per CPU"),
	TWOPT_ULONG("nlocations", g_tw_nlp, "number of locations per region per CPU"),
	TWOPT_UINT("nregions", g_epi_nregions, "number of regions per CPU"),
	TWOPT_UINT("mean", g_epi_mean, "exponential mean for location durations (secs)"),

	// init model from input files
	TWOPT_GROUP("EPI Model: initialize from file"),
	TWOPT_CHAR("ic", g_epi_ic_fn, "IC input file name"),
	TWOPT_CHAR("agents", g_epi_agent_fn, "Agent input file name"),
	TWOPT_END(),
	TWOPT_END(),
	TWOPT_END()
};

/*
 * main	- start function for the model, setup global state space of model and
 *	  init and run the simulation executive.  Also must map LPs to PEs
 */
int
main(int argc, char **argv, char **env)
{
	int		 i;

	tw_opt_add(app_opt);
	tw_init(&argc, &argv);

	g_tw_events_per_pe = (g_tw_nlp / g_tw_npe) * 4;
	g_tw_memory_nqueues = 2;

	tw_define_lps(g_tw_nlp, sizeof(epi_message), 0);

	epi_disease_init();

	// create KP memory buffer queues
	for(i = 0; i < g_tw_nkp; i++)
	{
		g_epi_fd = tw_kp_memory_init(tw_getkp(i), 
					//0, //g_epi_nagents / g_tw_nkp,
					g_epi_nagents / g_tw_nkp,
				  	sizeof(epi_agent), 1);
		g_epi_pathogen_fd = tw_kp_memory_init(tw_getkp(i), 
					(g_epi_nagents / g_tw_nkp) * g_epi_ndiseases,
				  	sizeof(epi_pathogen), 1);
	}

	// Round-robin mapping of LPs to KPs and KPs to PEs
	for(i = 0; i < g_tw_nlp; i++)
		tw_lp_settype(i, &mylps[0]);

	// read input files, create global data structures, etc
	epi_agent_init();

	tw_run();

	epi_model_final();

	return 0;
}

void
epi_md_final()
{
}
