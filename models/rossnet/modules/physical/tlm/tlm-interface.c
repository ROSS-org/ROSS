#include <tlm.h>

#define TOP	1
#define RIGHT	1
#define BOTTOM	1
#define LEFT	1

	/*
	 * The event being processed that caused this function to be
	 * called is a user model event.  Therefore, I cannot use it
	 * for state saving.  Solution: create my own event and send
	 * it with a 0.0 offset timestamp.
	 */
void
tlm_wave_initiate(double * position, double signal, tw_lp * lp)
{
	tw_lpid		 gid = tlm_getcell(position);
	tw_event	*e;

	tlm_message	*m;

	e = tw_event_new(gid, g_tlm_scatter_ts, lp);

	m = tw_event_data(e);
	m->type = RM_WAVE_INIT;
	m->displacement = signal;
	m->id = lp->gid;

	tw_event_send(e);
}

double
tlm_move2(double range, double position[], double velocity[], tw_lp * lp)
{
#if PLOT
	tlm_pe	*rpe = tw_pe_data(lp->pe);

	fprintf(rpe->move_log, "%lf %ld %lf %lf %lf\n", 
		tw_now(lp), lp->gid, position[0], position[1], position[2]);
#endif

	return 0.0;
}

/*
 * Should really only need to call this when the velocity vector changes
 * in the user model, RM should be able to move it at the correct frequency
 * on it's own.
 */
double
tlm_move(double range, double position[], double velocity[], tw_lp * lp)
{
	//tw_lpid	 me = tlm_getcell(position);

#if RM_RANGE_PARTICLES
	double	 pos[g_tlm_spatial_dim];

	int	 end;
	int	 i;
#endif

	int	 x;
	int	 y;

	//printf("%ld: moving at %lf \n", lp->gid, tw_now(lp));

	// compute LP id of the cell containing this user model LP
	x = position[0] / g_tlm_spatial_d[0];
	y = position[1] / g_tlm_spatial_d[1];

	if((x < 0 || x > g_tlm_spatial_grid[0]) ||
	   (y < 0 || y > g_tlm_spatial_grid[1]))
	{
		printf("%lld: walked off grid! (%d, %d)\n", lp->gid, x, y);
		return 0.0;
	}

	fprintf(g_tlm_nodes_plt_f, "%lld %lf %lf %lf\n", lp->gid, 
		position[0], position[1], position[2]);

#if RM_RANGE_PARTICLES
	// send particles to cells at range boundary
	pos[0] = x - (range / g_tlm_spatial_d[0]);
	pos[1] = y + (range / g_tlm_spatial_d[1]);
	pos[2] = tlm_getelevation(pos);

	end = range * 2;

	// top
	if(pos[1] >= 0 && pos[1] < g_tlm_spatial_grid[1])
	{
		for(i = 0; i < end+1; i++)
		{
#if TOP
			if(pos[0] >= 0 && pos[0] < g_tlm_spatial_grid[0])
				tlm_particle_send(tlm_getcell(pos), 1.0, lp);
#endif

			pos[0]++;
		}
	} else
		pos[0] += end + 1;

	// right
	pos[0]--;
	if(pos[0] >= 0 && pos[0] < g_tlm_spatial_grid[0])
	{
		for(i = 0; i < end; i++)
		{
			pos[1]--;

#if RIGHT
			if(pos[1] >= 0 && pos[1] < g_tlm_spatial_grid[1])
				tlm_particle_send(tlm_getcell(pos), 1.0, lp);
#endif
		}
	} else
		pos[1] -= end;

	// bottom
	if(pos[1] >= 0 && pos[1] < g_tlm_spatial_grid[1])
	{
		for(i = 0; i < end; i++)
		{
			pos[0]--;

#if BOTTOM
			if(pos[0] >= 0 && pos[0] < g_tlm_spatial_grid[0])
				tlm_particle_send(tlm_getcell(pos), 1.0, lp);
#endif
		}
	} else
		pos[0] -= end;

	// left
	if(pos[0] >= 0 && pos[0] < g_tlm_spatial_grid[0])
	{
		for(i = 0; i < end-1; i++)
		{
			pos[1]++;

#if LEFT
			if(pos[1] >= 0 && pos[1] < g_tlm_spatial_grid[1])
				tlm_particle_send(tlm_getcell(pos), 1.0, lp);
#endif
		}
	} else
		pos[1] += end - 1;
#endif

	tw_error(TW_LOC, "Cannot get displacement for remote LP this way!");

#if 0
	if(me->cur_state)
	{
		tlm_state * state = me->cur_state;
		return state->displacement;
	} else
		return 0.0;
#endif
}

void
tlm_rc_move(tw_lp * lp)
{
	//tw_memory	*b;

	//while(NULL != (b = tw_event_memory_get(lp)))
		//tw_memory_alloc_rc(lp, b, g_tlm_fd);
}
