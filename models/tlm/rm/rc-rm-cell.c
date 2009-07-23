#include <rm.h>

#define VERIFY_RM_CELL 0

	/*
	 * Two types of events are possible:
	 *
	 * RM_TIMER: timer fired, and we need to propagate wave data
	 * RM_CELL: we have received an input wave particle and must process it
	 */
void
rm_rc_cell_handler(rm_state * state, tw_bf * bf, rm_message * m, tw_lp * lp)
{
	switch(m->type)
	{
		case RM_SCATTER:
			rm_rc_scatter(state, bf, m, lp);
			state->stats->s_ncell_scatter--;
			break;

		case RM_GATHER:
			rm_rc_gather(state, bf, m, lp);
			state->stats->s_ncell_gather--;
			break;

		case RM_WAVE_INIT:
			rm_rc_scatter(state, bf, m, lp);
			state->stats->s_ncell_initiate--;
			break;

		default:
			tw_error(TW_LOC, "Unhandled event type!");
	}
}

void
rm_rc_scatter(rm_state * state, tw_bf * bf, rm_message * m, tw_lp * lp)
{
	int	 i;

	state->next_time = m->prev_time;
	state->displacement = m->displ;

	for(i = 0; i < g_rm_spatial_dir; i++)
		state->displacements[i] = m->disp[i];

#if VERIFY_RM_CELL
	printf("\t\t%d %ld: rc_scatter at %lf, dir %d, U=%lf \n", lp->pe->id, lp->id, tw_now(lp), m->direction, state->displacement);
#endif
}

void
rm_rc_gather(rm_state * state, tw_bf * bf, rm_message * m, tw_lp * lp)
{
	int	 i;

#if VERIFY_RM_CELL
		printf("%d %ld: reset next scatter ts to %lf \n", lp->pe->id, lp->id, state->next_time);
#endif

	state->next_time = m->prev_time;

	// my cell recvs from nbrs dir 5, which is my dir 4
	if(m->direction % 2 == 0)
		i = m->direction + 1;
	else
		i = m->direction - 1;

	state->displacements[i] = m->disp[i];
	state->displacement = m->displ;

#if VERIFY_RM_CELL
	printf("\t\t%d %ld: rc_gather at %lf, dir %d, U=%lf, next scatter %lf \n", lp->pe->id, lp->id, tw_now(lp), i, state->displacement, state->next_time);
#endif
}
