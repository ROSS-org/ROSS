#include <pm.h>

tw_peid
pm_map(tw_lpid gid)
{
	return (tw_peid) gid / nlp_per_pe;
}

/*
 * pm.c -- defines the 4 simulation engine void * functions required for
 *		execution of the model.
 */

/*
 * pm_init - initialize node LPs state variables
 * 
 * state	- our node LP state space
 * lp		- our node LP
 */
void
pm_init(pm_state * state, tw_lp * lp)
{
	tw_event	*e;
	pm_message	*m;

	int		 x = 50 + (g_tw_mynode * 100);

	// allocate and configure RNG
	if(lp->rng)
		tw_error(TW_LOC, "RNG defined!");

	tw_rand_init_streams(lp, 1);
	state->stats = tw_calloc(TW_LOC, "PM LP stats", sizeof(pm_statistics), 1);

	// made up signal strength
	// 3GHz frequency is 12.4 mu eV
	state->transmit = 12400000.0;
	state->range = 4000; // meters
	state->frequency = 1; // seconds
	state->prev_time = 0.0;

	state->position = tw_calloc(TW_LOC, "PM LP position", sizeof(double), g_rm_spatial_dim);
	state->velocity = tw_calloc(TW_LOC, "PM LP velocity", sizeof(double), g_rm_spatial_dim);

	// randomize direction
	if(!g_pm_mu)
		g_pm_mu = 1;

	if(g_pm_mu > g_pm_distr_sd)
		g_pm_distr_sd = g_pm_mu;

#if ONE || 1
//if(lp->id % 1000 != 0)
if(g_tw_mynode != the_one)
	return;
#endif

	state->position[0] = tw_rand_integer(lp->rng, x - 4, x + 4);
	state->position[0] *= spacing[0];

	state->position[1] = tw_rand_integer(lp->rng, 100, grid[1]-100);
	state->position[1] *= spacing[1];

	//state->position[2] = rm_getelevation(state->position) + spacing[2];
	state->position[2] = rm_getelevation(state->position);

	state->velocity[0] = state->velocity[1] = 0.0;

#if 0
	printf("%d: (%lf, %lf, %lf)\n", 
			lp->gid, state->position[0], 
			state->position[1], state->position[2]);
#endif


	rm_move2(state->range, state->position, state->velocity, lp);

	// setup event to self to move
	e = tw_event_new(lp->gid, tw_rand_exponential(lp->rng, 10.0), lp);
	//e = tw_event_new(lp->gid, 0.0, lp);
	m = tw_event_data(e);
	m->type = PM_TIMESTEP;
	tw_event_send(e);
}

/*
 * pm_event_handler - event processing (per node LP)
 * 
 * state	- our node LP state space
 * bf		- a bitfield provided by the simulation ecxecutive
 * m		- the incoming event or message
 * lp		- our node LP
 * 
 * In this model, nodes send "move" events, then must REACT to RM_EVENTS
 * that signal proximity.
 */
void
pm_event_handler(pm_state * state, tw_bf * bf, pm_message * m, tw_lp * lp)
{
	*(int *) bf = (int) 0;

	switch(m->type)
	{
		case PM_TIMESTEP:
			// Compute this radio's next location
			pm_move(state, m, bf, lp);
			state->stats->s_move_ev++;

			if(tw_rand_unif(lp->rng) <= percent_wave)
			{
				rm_wave_initiate(state->position, state->transmit, lp);
				state->stats->s_nwaves++;
			}
			break;
		case RM_PROXIMITY_LP:
			// Recv other node new location
			rm_wave_initiate(state->position, state->transmit, lp);
			state->stats->s_prox_ev++;
			break;
		case RM_PROXIMITY_ENV:
			// Recv Signal from Environment
			tw_error(TW_LOC, "in PROXIMITY\n");
			state->stats->s_recv_ev++;
			break;
		default:
			tw_error(TW_LOC, "%d %ld %ld: Unhandled event message: %d", g_tw_mynode, lp->gid, lp->id, m->type);
	}
}

/*
 * pm_rc_event_handler - reverse compute effects of out-of-order events
 * 			  + rollback LP random number generators
 *			  + restore LP state variables
 * 
 * state	- our node LP state space
 * bf		- a bitfield provided by the simulation ecxecutive
 * m		- the incoming event or message
 * lp		- our node LP
 */
void
pm_rc_event_handler(pm_state * state, tw_bf * bf, pm_message * m, tw_lp * lp)
{
	switch(m->type)
	{
		case PM_TIMESTEP:
			pm_rc_move(state, m, bf, lp);
			state->stats->s_move_ev--;
			tw_rand_reverse_unif(lp->rng);
			state->stats->s_nwaves--;
			break;
		case RM_PROXIMITY_LP:
			tw_error(TW_LOC, "Should not be here!");
			state->stats->s_prox_ev--;
			break;
		case RM_PROXIMITY_ENV:
			// Recv other node new location
			tw_error(TW_LOC, "Should not be here!");
			state->stats->s_recv_ev--;
			break;
		default:
			tw_error(TW_LOC, "Unhandled RC event message!");
	}
}

/*
 * pm_final - finalize node LP statistics / outputting
 * 
 * state	- our node LP state space
 * lp		- our node LP
 */
void
pm_final(pm_state * state, tw_lp * lp)
{
	g_pm_stats->s_nwaves += state->stats->s_nwaves;
	g_pm_stats->s_move_ev += state->stats->s_move_ev;
	g_pm_stats->s_prox_ev += state->stats->s_prox_ev;
	g_pm_stats->s_recv_ev += state->stats->s_recv_ev;
}

tw_petype my_pes[] =
{
        {
                (pe_init_f) pm_pe_init,
                (pe_init_f) pm_pe_post_init,
                (pe_gvt_f) pm_pe_gvt,
                (pe_final_f) pm_pe_final,
                (pe_periodic_f) 0
        },
        {0},
};

tw_lptype my_lps[] =
{
	{
		(init_f) pm_init,
		(event_f) pm_event_handler,
		(revent_f) pm_rc_event_handler,
		(final_f) pm_final,
		(map_f) pm_map,
		sizeof(pm_state)
	},
	{0},
};

static const tw_optdef pm_options [] =
{
	TWOPT_UINT("the-one", the_one, "single radio to model"),
	TWOPT_UINT("nradios", g_pm_nnodes, "number of radios"),
	TWOPT_UINT("rw-min", g_pm_mu, "random walk min change dir"),
	TWOPT_UINT("rw-max", g_pm_distr_sd, "random walk max change dir"),
	TWOPT_STIME("wave-percent", percent_wave, "percentage of waves per move"),
	TWOPT_CHAR("run", run_id, "user supplied run name"),
	TWOPT_END()
};

/*
 * main	- start function for the model, g_pm_setup global state space of model and
 *	  init and run the simulation executive.  Also must map LPs to HW.
 */
int
main(int argc, char **argv, char **env)
{
	int i ;
	g_pm_stats = tw_calloc(TW_LOC, "PM stats", 
				sizeof(pm_statistics), 1);

	// pass options to ROSS and initialize RM
	tw_opt_add(pm_options);
	rm_init(&argc, &argv);

	pm_init_scenario();

	rm_initialize_terrain(grid, spacing, g_pm_dimensions);

	if(!rm_initialize(my_pes, my_lps, g_tw_npe, g_tw_nkp, 
				g_pm_nnodes, sizeof(pm_message)))
		tw_error(TW_LOC, "Could not init ROSS!");

	rm_run(argv);

	pm_stats_print();

	rm_end();

	return 0;
}
