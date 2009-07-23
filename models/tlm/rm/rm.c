#include <rm.h>

tw_peid
_rm_map(tw_lpid gid)
{
	return (tw_peid) (gid / nlp_per_pe);
}

	/*
	 * rm.c:  This model provides the functions required by ROSS to 
	 *	  execute the Reactive Model Logical Processes.
	 */

	/*
	 * This function initializes the Reactive Model LP state.  The purpose
	 * of this function is to initialize the LP state variables.
	 */
void
_rm_init(rm_state *state, tw_lp * lp)
{
	int	 i;

	int	 n_z0;
	int	 n_z1;

	int	 z;

	double	*np;

	state->particles = tw_memoryq_init();
	state->stats = tw_calloc(TW_LOC, "stats", sizeof(rm_statistics), 1);

	// requires 2 * number of dimensions space to store incoming displacement,
	// plus the LP we received it from
	state->nbrs = tw_calloc(TW_LOC, "neighbors",
			sizeof(tw_lp *), g_rm_spatial_dir);
	state->displacements = tw_calloc(TW_LOC, "displacements",
					sizeof(double), g_rm_spatial_dir);

	// setup pointers to neighboring cells so that I can send them particles
	np = rm_getlocation(lp);

	// must init to -1
	for(i = 0; i < g_rm_spatial_dir; i++)
		state->nbrs[i] = -1;

//if(!g_tw_mynode) printf("%d: S: (%lf, %lf)\n", lp->gid, np[0], np[1]);
	// our neighbor array follows the compass pts: W, E, N, S, then DOWN, UP
	for(i = 0; i < g_rm_spatial_dim - 1; i++)
	{
		np[i] -= g_rm_spatial_d[i];
		if(np[i] >= 0)
		{
//if(!g_tw_mynode) printf("%d: %d: (%lf, %lf)\n", lp->gid, i, np[0], np[1]);
			n_z0 = rm_getelevation(np);

			if(n_z0 <= np[2] && (np[2] - n_z0) < g_rm_z_max)
				state->nbrs[i*2] = rm_getcell(np);
		}

		np[i] += 2.0 * g_rm_spatial_d[i];
		if(np[i] < g_rm_spatial_grid[i] * g_rm_spatial_d[i])
		{
			n_z1 = rm_getelevation(np);
if(0 && !g_tw_mynode)
{
	printf("%lld: %d: (%lf, %lf, %lf)\n", lp->gid, i, np[0], np[1], np[2]);

	if(lp->gid == 6)
	{
		printf("n_z1: %d, z_max: %d\n", n_z1, g_rm_z_max);
		printf("n_z1: %d, z_max: %d\n", n_z1, g_rm_z_max);
	}
}

			// nbr Z value must be within # of layers
			if(n_z1 <= np[2] && (np[2] - n_z1) < g_rm_z_max)
				state->nbrs[(i*2)+1] = rm_getcell(np);
			else if(n_z1 > np[2] && state->nbrs[i*2])
				state->nbrs[(i*2)+1] = state->nbrs[i*2];
			else if(!state->nbrs[i*2] && state->nbrs[(i*2)+1])
				state->nbrs[i*2] = state->nbrs[(i*2)+1];
		}
		np[i] -= g_rm_spatial_d[i];
	}

	// now connect this cell to cells below / above
	np = rm_getlocation(lp);
//if(!g_tw_mynode) printf("%d: F: (%lf, %lf)\n\n", lp->gid, np[0], np[1]);
	z = rm_getelevation(np);

	// below, not lowest cell
	if(np[2] > z)
	{
		np[2] -= g_rm_spatial_d[2];
		state->nbrs[4] = rm_getcell(np);
		np[2] += g_rm_spatial_d[2];
	}

	// above, not highest cell
	np[2] += g_rm_spatial_d[2];
	if(np[2] < z + g_rm_z_max)
		state->nbrs[5] = rm_getcell(np);

	np[2] -= g_rm_spatial_d[2];


	// setup for ground reflection in 3D models
#if 0
	if(z == np[2])
		state->nbrs[4] = state->nbrs[5];
#endif

	// gratuitious error checking

	// make sure we have at least 1 neighbor
	for(i = 0; i < g_rm_spatial_dir; i++)
	{
		if(state->nbrs[i] != -1)
			return;
	}

	tw_error(TW_LOC, "No neighbors!");
}

	/*
	 * This is the Reactive Model LP event handler.  The purpose of this
	 * function is to handle RM-RM LP events, and generate events into the
	 * user model(s) when certain conditions have been met / detected.  
	 * Presently, the only condition we are supporting is proximity 
	 * detection.
	 */

void
state_copy(rm_state * a, rm_state * b, double * d)
{
	int	 i;

	a->next_time = b->next_time;
	a->displacement = b->displacement;

	for(i = 0; i < g_rm_spatial_dir; i++)
		d[i] = b->displacements[i];
}

void
_rm_event_handler(rm_state * state, tw_bf * bf, rm_message * m, tw_lp * lp)
{
	switch(m->type)
	{
		// wave propagation types
		case RM_SCATTER:
		case RM_GATHER:
		case RM_WAVE_INIT:
			rm_cell_handler(state, bf, m, lp);
			break;

		// proximity detection type
		case RM_PARTICLE:
			tw_error(TW_LOC, "Unhandled event type: %d\n", m->type);
			rm_particle_handler(state, bf, m, lp);
#if SAVE_MEM
			state->stats->s_nparticles++;
#endif
			break;
		default:
			tw_error(TW_LOC, "%ld: Unhandled event type: %d, from %ld at %.16lf\n", 
				 lp->gid, m->type, m->id, tw_now(lp));
	}
}

	/*
	 * This is the Reactive Model LP reverse computation event handler.  The
	 * purpose of this function is to handle the reverse computation on the 
	 * LP state during a rollback.  It is only necessary to reverse incorrect
	 * operations performed on the LP state.
	 */
void
_rm_rc_event_handler(rm_state * state, tw_bf * bf, rm_message * m, tw_lp * lp)
{
	switch(m->type)
	{
		case RM_SCATTER:
		case RM_GATHER:
		case RM_WAVE_INIT:
			rm_rc_cell_handler(state, bf, m, lp);
			break;

		case RM_PARTICLE:
			tw_error(TW_LOC, "on particles!");
			rm_rc_particle_handler(state, bf, m, lp);
			state->stats->s_nparticles--;
			break;
		default:
			tw_error(TW_LOC, "Unhandled event type!");
	}
}

	/*
	 * This is the Reactive Model LP final function.  The purpose of this
	 * function is to give the LP an opportunity at the end of the simulation
	 * execution to aggregate statistical data.
	 */
void
_rm_final(rm_state *state, tw_lp * lp)
{
	g_rm_stats->s_nparticles += state->stats->s_nparticles;
	g_rm_stats->s_ncell_scatter += state->stats->s_ncell_scatter;
	g_rm_stats->s_ncell_gather += state->stats->s_ncell_gather;
	g_rm_stats->s_ncell_initiate += state->stats->s_ncell_initiate;
}
