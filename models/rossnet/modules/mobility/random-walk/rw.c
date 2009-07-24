#include <rw.h>

/*
 * rw.c -- defines the 4 simulation engine void * functions required for
 *		execution of the model.
 */

/*
 * rw_init - initialize node LPs state variables
 * 
 * state	- our node LP state space
 * lp		- our node LP
 */
void
rw_init(rw_state * state, tw_lp * lp)
{
	tw_event	*e;
	tw_memory	*b;

	rw_message	*m;

	tw_rand_init_streams(lp, 1);

	state->stats = tw_calloc(TW_LOC, "PM LP stats", sizeof(rw_statistics), 1);

	// made up signal strength
	// 3GHz frequency is 12.4 mu eV
	state->transmit = 12400000.0;
	state->range = 4000; // meters
	state->frequency = 1; // seconds
	state->prev_time = 0.0;

	state->position = tw_calloc(TW_LOC, "position", 
					sizeof(double), g_tlm_spatial_dim);
	state->velocity = tw_calloc(TW_LOC, "velocity", 
					sizeof(double), g_tlm_spatial_dim);

	// randomize direction
	if(!g_rw_mu)
		g_rw_mu = 1;

	if(g_rw_mu > g_rw_distr_sd)
		g_rw_distr_sd = g_rw_mu;

	state->position[0] = tw_rand_integer(lp->rng, 0, g_tlm_spatial_grid[0]-1);
	state->position[0] *= g_tlm_spatial_d[0];

	state->position[1] = tw_rand_integer(lp->rng, 0, g_tlm_spatial_grid[1]-1);
	state->position[1] *= g_tlm_spatial_d[1];

	//state->position[2] = tlm_getelevation(state->position) + spacing[2];
	state->position[2] = tlm_getelevation(state->position);

#if 1
	printf("%lld: START_LOC (%lf, %lf, %lf)\n", 
			lp->gid, state->position[0], 
			state->position[1], state->position[2]);
#endif

	// velocity is zero since we only compute one step of radio movement
	state->velocity[0] = 1.0 * g_tlm_spatial_d[0];
	state->velocity[1] = 1.0 * g_tlm_spatial_d[1];

	// setup event to self to move
	e = rn_event_direct(lp->gid, tw_rand_exponential(lp->rng, 1.0), lp);
	b = tw_memory_alloc(lp, g_rw_fd);

	m = tw_memory_data(b);
	m->type = RW_TIMESTEP;

	tw_event_memory_set(e, b, g_rw_fd);

	rn_event_send(e);
}

/*
 * rw_event_handler - event processing (per node LP)
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
rw_event_handler(rw_state * state, tw_bf * bf, rn_message * msg, tw_lp * lp)
{
	tw_memory	*b;

	rw_message	*m;

	*(int *) bf = (int) 0;

	if(NULL == (b = tw_event_memory_get(lp)))
		tw_error(TW_LOC, "%lld: no membuf on RW event!", lp->gid);

	m = tw_memory_data(b);

	switch(m->type)
	{
		case RW_TIMESTEP:
			// Compute this radio's next location
			rw_move(state, m, bf, lp);
			state->stats->s_move_ev++;

			if(tw_rand_unif(lp->rng) <= percent_wave)
			{
				tlm_wave_initiate(state->position, state->transmit, lp);
				state->stats->s_nwaves++;
			}
			break;
		case RM_PROXIMITY_LP:
			// Recv other node new location
			tlm_wave_initiate(state->position, state->transmit, lp);
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

	tw_memory_free(lp, b, g_rw_fd);
}

/*
 * rw_rc_event_handler - reverse compute effects of out-of-order events
 * 			  + rollback LP random number generators
 *			  + restore LP state variables
 * 
 * state	- our node LP state space
 * bf		- a bitfield provided by the simulation ecxecutive
 * m		- the incoming event or message
 * lp		- our node LP
 */
void
rw_rc_event_handler(rw_state * state, tw_bf * bf, rn_message * msg, tw_lp * lp)
{
	tw_memory	*b;

	rw_message	*m;

	if(NULL == (b = tw_memory_free_rc(lp, g_rw_fd)))
		tw_error(TW_LOC, "%lld: no membuf on event in RW!", lp->gid);

	m = tw_memory_data(b);

	switch(m->type)
	{
		case RW_TIMESTEP:
			rw_rc_move(state, m, bf, lp);
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

	tw_event_memory_get_rc(lp, b, g_rw_fd);
}

/*
 * rw_final - finalize node LP statistics / outputting
 * 
 * state	- our node LP state space
 * lp		- our node LP
 */
void
rw_final(rw_state * state, tw_lp * lp)
{
	g_rw_stats->s_nwaves += state->stats->s_nwaves;
	g_rw_stats->s_move_ev += state->stats->s_move_ev;
	g_rw_stats->s_prox_ev += state->stats->s_prox_ev;
	g_rw_stats->s_recv_ev += state->stats->s_recv_ev;
}
