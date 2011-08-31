#include <rm.h>

#define VERIFY_PARTICLE 1

	/*
	 * Create and send new boundary particle
	 */
tw_event	*
rm_particle_send(tw_lpid gid, tw_stime offset, tw_lp * src_lp)
{
	tw_event	*e;
	//tw_memory	*b;

	rm_message	*m;
	//rm_particle	*p;

	//printf("%ld: sending particle to cell %ld \n", src_lp->id, gid);

	if(gid == 0)
		tw_error(TW_LOC, "here");
	e = tw_event_new(gid, offset, src_lp);
	m = tw_event_data(e);

	m->type = RM_PARTICLE;

#if DWB
	b = tw_memory_alloc(src_lp, g_rm_fd);
	tw_event_memory_set(e, b, g_rm_fd);

	p = tw_memory_data(b);
	p->user_lp = src_lp;
#endif

	tw_event_send(e);

	return e;
}

void
rm_particle_handler(rm_state * state, tw_bf * bf, rm_message * m, tw_lp * lp)
{
	//tw_memory	*b;
	//tw_memory	*b_in;

#if VERIFY_PARTICLE && DWB
	rm_particle	*p;

	double		*position;
#endif

#if DWB
	b_in = tw_event_memory_get(lp);
#endif

#if 0
	if(state->prev_time != tw_now(lp))
	{
		if(state->initial)
		{
			bf->c1 = 1;
			tw_memory_free(lp, state->initial, g_rm_fd);
			state->initial = NULL;
		}

		state->initial = b_in;
		state->prev_time = tw_now(lp);
	} else if(state->initial != NULL)
	{
		bf->c0 = 1;
		rm_proximity_send(state, b_in, state->initial, lp);

		tw_memory_free(lp, b_in, g_rm_fd);
		tw_memory_free(lp, state->initial, g_rm_fd);
		state->initial = NULL;
	} else
		tw_error(TW_LOC, "Initial == NULL, but prev_time == now!");
#endif

#if DWB
	// if in next timestep, dump the particle queue and add this particle
	//if(state->prev_time != tw_now(lp) && state->particles->size != 0)
	if(state->prev_time != tw_now(lp))
	{
		bf->c0 = 1;

		while((NULL != (b = tw_memoryq_pop(state->particles))))
			tw_memory_free(lp, b, g_rm_fd);

		state->initial = b_in;
		state->prev_time = tw_now(lp);
	} else // check if we have detected a collision
	{
		// and send notifications to affected user LPs
		for(b = state->particles->head; b; b = b->next)
			rm_proximity_send(state, b_in, state->initial, lp);
	}

	// add this particle to the queue
	tw_memoryq_push(state->particles, b_in);
#endif

#if VERIFY_PARTICLE && DWB
	// plot leading edge of node range
	p = tw_memory_data(b_in);
	position = rm_getlocation(lp);
	fprintf(g_rm_parts_plt_f, "%lf %ld %lf %lf %ld\n", tw_now(lp), 
		p->user_lp->id, position[0], position[1], lp->id);
#endif
}

void
rm_rc_particle_handler(rm_state * state, tw_bf * bf, rm_message * m, tw_lp * lp)
{
	//tw_memory	*b;
	//tw_memory	*b_in;

	//state->prev_time = tw_now(lp);

#if DWB
	if(bf->c0 == 1)
	{
		state->initial = tw_memory_free_rc(lp, g_rm_fd);
		tw_event_memory_get_rc(lp, tw_memory_free_rc(lp, g_rm_fd), g_rm_fd);
	} else
	{
		tw_event_memory_get_rc(lp, state->initial, g_rm_fd);
		state->initial = NULL;

		if(bf->c1 == 1)
			state->initial = tw_memory_free_rc(lp, g_rm_fd);
	}
#endif

#if 0
	// remove this particle to the queue
	b_in = tw_memoryq_pop(state->particles);

	// place particle back onto event
	//tw_event_memory_get_rc(lp, b_in, g_rm_fd);

	// if in next timestep, dump the particle queue and add this particle
	if(bf->c0 == 1)
	{
		state->prev_time = tw_now(lp);

		// unfree particles and place back into queue
		while((NULL != (b = tw_memory_free_rc(lp, g_rm_fd))))
			tw_memoryq_push(state->particles, b);
	}
#endif
}
