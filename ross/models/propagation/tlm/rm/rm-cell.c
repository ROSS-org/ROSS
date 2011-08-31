#include <rm.h>

#define PLOT 0
#define VERIFY_RM_CELL 0

	/*
	 * Two types of events are possible:
	 *
	 * RM_TIMER: timer fired, and we need to propagate wave data
	 * RM_CELL: we have received an input wave particle and must process it
	 * RM_CELL_WAVE: initiate a new wave propagation
	 */
void
rm_cell_handler(rm_state * state, tw_bf * bf, rm_message * m, tw_lp * lp)
{
	switch(m->type)
	{
		// scatter the energy stored at this cell:
		// here we simply need to create a GATHER event to each
		// neighboring LP with the energy properly attenuated.
		case RM_SCATTER:
			rm_cell_scatter(state, m, bf, lp);
			state->stats->s_ncell_scatter++;
			break;

		// here we are gathering energy from one of our neighboring LPs.
		case RM_GATHER:
			rm_cell_gather(state, bf, m, lp);
			state->stats->s_ncell_gather++;
			break;

		// initiate a new wave scatter
		// note: it is possible another wave is currently passing
		//       through this cell
		case RM_WAVE_INIT:
			rm_cell_scatter(state, m, bf, lp);
			state->stats->s_ncell_initiate++;
			break;

		default:
			tw_error(TW_LOC, "Unhandled event type!");
	}
}

	/*
	 * Time to scatter, so must send stored energy to neighbors.
	 */
void
rm_cell_scatter(rm_state * state, rm_message * m, tw_bf * bf, tw_lp * lp)
{
	tw_event	*e = NULL;

	rm_message	*msg;
#if PLOT
	rm_pe		*rpe;
#endif
	double		 d;

	int		 i;


	// state-save vars
	m->prev_time = state->next_time;
	m->displ = state->displacement;

	state->displacement += (m->displacement * g_rm_wave_loss_coeff);

#if PLOT
	double		*pos = rm_getlocation(lp);
	rpe = tw_pe_data(lp->pe);
	fprintf(rpe->wave_log, "%.16lf %ld %ld %lf %lf %lf %.2lf\n", 
			tw_now(lp), m->id, lp->id, 
			pos[0], pos[1], pos[2], state->displacement);
#endif

#if VERIFY_RM_CELL
	printf("%d %ld: SCATTER at %.10lf from dir %d, U=%lf\n", lp->pe->id, lp->id, tw_now(lp), m->direction, state->displacement);
#endif

	// For each direction i, send total displacement minus the contributed
	// displacement from direction i.
	for(i = 0; i < g_rm_spatial_dir; i++)
	{
		// Grid boundary condition
		if(-1 == state->nbrs[i])
			continue;

		d = (g_rm_spatial_coeff * state->displacement) - state->displacements[i];

		// Ground reflection: attenuate using ground coeff
		if((i % 2 == 0 && state->nbrs[i] == state->nbrs[i+1]) ||
			(i % 2 == 1 && state->nbrs[i] == state->nbrs[i-1]))
			d *= g_rm_spatial_ground_coeff;

		// do not scatter below a given threshold
		if(d <= g_rm_wave_threshold)
			continue;

		//e = tw_event_new(state->nbrs[i], 0.1, lp);
		e = tw_event_new(state->nbrs[i], g_rm_spatial_offset_ts[i] - g_rm_scatter_ts, lp);

		msg = tw_event_data(e);
		msg->type = RM_GATHER;
		msg->direction = i;
		msg->displacement = d;
		msg->id = m->id;

#if VERIFY_RM_CELL
		printf("\t%d %ld: scatter dir %d (lp %ld) at %.10lf, U=%lf \n", lp->pe->id,  
			lp->gid, i, state->nbrs[i], e->recv_ts, msg->displacement);
#endif

		tw_event_send(e);
	}

	// return to equilibrium: cancel out displacements in each
	//			  direction for next timestep
	for(i = 0; i < g_rm_spatial_dir; i++)
	{
		m->disp[i] = state->displacements[i];
		state->displacements[i] = 0.0;
	}

	state->displacement = 0.0;
}

void
rm_cell_gather(rm_state * state, tw_bf * bf, rm_message * m, tw_lp * lp)
{
	tw_event	*e;
	rm_message	*msg;

	int	 i;

	// state-save next_time
	m->prev_time = state->next_time;

	// schedule event to begin scattering wave
	if(state->next_time < tw_now(lp))
	{
		e = tw_event_new(lp->gid, g_rm_scatter_ts, lp);

		msg = tw_event_data(e);
		msg->type = RM_SCATTER;
		msg->direction = m->direction;
		msg->id = m->id;
		msg->displacement = 0.0;

		tw_event_send(e);

		state->next_time = e->recv_ts;

#if VERIFY_RM_CELL
		printf("%d %ld: GATHER next scatter time %.10lf (was %.10lf)\n",
			lp->pe->id, lp->id, state->next_time, m->prev_time);
#endif
	}

	// my cell recvs from nbrs dir 5, which is my dir 4
	if(m->direction % 2 == 0)
		i = m->direction + 1;
	else
		i = m->direction - 1;

	// from Nutaro:  on recv'ing an input event, the cell computes it's total
	// displacement as the sum of the displacements of it's neighbors

	m->disp[i] = state->displacements[i];
	state->displacements[i] = m->displacement;// * g_rm_wave_loss_coeff;

	m->displ = state->displacement;
	state->displacement += m->displacement;// * g_rm_wave_loss_coeff);

#if VERIFY_RM_CELL
	printf("\t%d %ld: gather from dir %d at %.10lf, U=%lf \n", lp->pe->id, 
		lp->id, m->direction, tw_now(lp), state->displacement);
#endif
}
