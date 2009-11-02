#include <pm.h>

int
off_grid(double * p)
{
	if(p[0] < g_rm_spatial_d[0] || 
	   p[0] >= (g_rm_spatial_grid[0] - 1) * g_rm_spatial_d[0] ||
	   p[1] < g_rm_spatial_d[1] || 
	   p[1] >= (g_rm_spatial_grid[1] - 1) * g_rm_spatial_d[1])
	{
		return 1;
	}

	return 0;
}

# if 0
	MAP:
	0 1 2
	3 R 4
	5 6 7
#endif

void
change_direction(pm_state * state, pm_message * m, tw_bf * bf, tw_lp * lp)
{
	state->theta = tw_rand_integer(lp->rng, 0, 7);
	state->change_dir = tw_rand_integer(lp->rng, g_pm_mu, g_pm_distr_sd) +
				tw_now(lp);

	* (int *) bf += 2;

	switch(state->theta)
	{
		case 0:
			state->velocity[0] = -1.0;
			state->velocity[1] =  1.0;
			break;
		
		case 1:
			state->velocity[0] =  0.0;
			state->velocity[1] =  1.0;
			break;
		
		case 2:
			state->velocity[0] =  1.0;
			state->velocity[1] =  1.0;
			break;
		
		case 3:
			state->velocity[0] =  1.0;
			state->velocity[1] =  0.0;
			break;
		
		case 4:
			state->velocity[0] = -1.0;
			state->velocity[1] =  0.0;
			break;
		
		case 5:
			state->velocity[0] = -1.0;
			state->velocity[1] = -1.0;
			break;
		
		case 6:
			state->velocity[0] =  0.0;
			state->velocity[1] = -1.0;
			break;
		
		case 7:
			state->velocity[0] =  1.0;
			state->velocity[1] =  1.0;
			break;
		default:
			tw_error(TW_LOC, "Bad direction!");
	}

	state->velocity[0] *= g_rm_spatial_d[0];
	state->velocity[1] *= g_rm_spatial_d[1];

#if VERIFY_PM_NODE
	printf("Changing dir at %lf: %d \n", tw_now(lp), state->theta);
	printf("\t%lf %lf\n", state->velocity[0], state->velocity[1]);
#endif
}

void
new_location(pm_state * state, tw_bf * bf, tw_lp * lp)
{
	state->position[0] += state->velocity[0];
	state->position[1] += state->velocity[1];
}

void
rc_new_location(pm_state * state, tw_bf * bf, tw_lp * lp)
{
	state->position[0] -= state->velocity[0];
	state->position[1] -= state->velocity[1];
}

/*
 * pm_move - compute next location for this node
 */
void
pm_move(pm_state * state, pm_message * m, tw_bf * bf, tw_lp * lp)
{
	tw_event	*e;
	pm_message	*msg;

	//double		 signal;

	* (int *) bf = 0;

	// state-save position
	m->x_pos = state->position[0];
	m->y_pos = state->position[1];
	m->theta = state->theta;
	m->change_dir = state->change_dir;

	// time to change direction?
	//if(state->change_dir-- == 0)
	if(state->change_dir-- == 0)
		change_direction(state, m, bf, lp);

	new_location(state, bf, lp);

	while(off_grid(state->position))
	{
		rc_new_location(state, bf, lp);
		change_direction(state, m, bf, lp);
		new_location(state, bf, lp);
	}

	state->position[2] = rm_getelevation(state->position);

#if VERIFY_PM_NODE
	printf("%ld: (%lf, %lf, %lf)\n", lp->gid, state->position[0],
		state->position[1], state->position[2]);
#endif

	// need to call RM interface function
	// rm_move2 simply logs the movement of the node
	rm_move2(state->range, state->position, state->velocity, lp);

#if 0
	if(signal)
		tw_printf(TW_LOC, "%ld: recvd signal at %lf: %lf \n", 
				lp->id, tw_now(lp), signal);
#endif

	// send ourselves an event for the next timestep
	e = tw_event_new(lp->gid, tw_rand_exponential(lp->rng, 10.0), lp);
	//e = tw_event_new(lp->gid, 1.0, lp);
	msg = tw_event_data(e);

	msg->type = PM_TIMESTEP;
	msg->from = lp->gid;

	tw_event_send(e);
}

void
pm_rc_move(pm_state * state, pm_message * m, tw_bf * bf, tw_lp * lp)
{
	int	 n = * (int *) bf;
	int	 i;

	tw_rand_reverse_unif(lp->rng);

	state->position[0] = m->x_pos;
	state->position[1] = m->y_pos;
	state->theta = m->theta;
	state->change_dir = m->change_dir;

	for(i = 0; i < n; i++)
		tw_rand_reverse_unif(lp->rng);
}
