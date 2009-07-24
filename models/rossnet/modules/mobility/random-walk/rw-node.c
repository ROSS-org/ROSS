#include <rw.h>

int
off_grid(double * p)
{
	if(p[0] < 0 || 
	   p[0] >= g_tlm_spatial_grid[0] * g_tlm_spatial_d[0] ||
	   p[1] < 0 || 
	   p[1] >= g_tlm_spatial_grid[1] * g_tlm_spatial_d[1])
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
change_direction(rw_state * state, rw_message * m, tw_bf * bf, tw_lp * lp)
{
	state->theta = tw_rand_integer(lp->rng, 0, 7);
	state->change_dir = tw_rand_integer(lp->rng, g_rw_mu, g_rw_distr_sd) +
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

	state->velocity[0] *= g_tlm_spatial_d[0];
	state->velocity[1] *= g_tlm_spatial_d[1];

#if VERIFY_RW_NODE
	printf("Changing dir at %lf: %d \n", tw_now(lp), state->theta);
	printf("\t%lf %lf\n", state->velocity[0], state->velocity[1]);
#endif
}

inline void
new_location(rw_state * state, tw_bf * bf, tw_lp * lp)
{
	state->position[0] += state->velocity[0];
	state->position[1] += state->velocity[1];
}

inline void
rc_new_location(rw_state * state, tw_bf * bf, tw_lp * lp)
{
	state->position[0] -= state->velocity[0];
	state->position[1] -= state->velocity[1];
}

/*
 * rw_move - compute next location for this node
 */
void
rw_move(rw_state * state, rw_message * m, tw_bf * bf, tw_lp * lp)
{
	tw_event	*e;
	tw_memory	*b;

	rw_message	*msg;

	//double		 signal;

	* (int *) bf = 0;

	// state-save position variables
	m->x_pos = state->position[0];
	m->y_pos = state->position[1];
	m->theta = state->theta;
	m->change_dir = state->change_dir;

	// time to change direction?
	if(state->change_dir == tw_now(lp))
		change_direction(state, m, bf, lp);

	new_location(state, bf, lp);

	while(off_grid(state->position))
	{
		rc_new_location(state, bf, lp);
		change_direction(state, m, bf, lp);
		new_location(state, bf, lp);
	}

	state->position[2] = tlm_getelevation(state->position);

#if VERIFY_RW_NODE || 1
	printf("%lld: (%lf, %lf, %lf) next %d\n", lp->gid, state->position[0],
		state->position[1], state->position[2], state->change_dir);
#endif

	// need to call RM interface function
	// tlm_move2 simply logs the movement of the node
	//tlm_move2(state->range, state->position, state->velocity, lp);

#if 0
	if(signal)
		tw_printf(TW_LOC, "%ld: recvd signal at %lf: %lf \n", 
				lp->id, tw_now(lp), signal);
#endif

	// send ourselves an event for the next timestep
	e = rn_event_direct(lp->gid, tw_rand_exponential(lp->rng, 1.0), lp);
	b = tw_memory_alloc(lp, g_rw_fd);

	msg = tw_memory_data(b);

	msg->type = RW_TIMESTEP;

	tw_event_memory_set(e, b, g_rw_fd);
	tw_event_send(e);
}

void
rw_rc_move(rw_state * state, rw_message * m, tw_bf * bf, tw_lp * lp)
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
