#include <epi.h>

#define EPI_MAP_LINEAR 1

/*
 * epi.c -- defines the 4 simulation engine void * functions required for
 *		execution of the model.  Contains the main function.
 */

#define TWENTY_FOUR_HOURS 86400

/*
 * epi_init - initialize node LPs state variables
 * 
 * state	- our node LP state space
 * lp		- our node LP
 */
void
epi_init(epi_state * state, tw_lp * lp)
{
	epi_agent	*a = pq_top(state->pq);

	// agents mapped to LP?
	if(!a)
	{
		// NOTE: If not home assumed to be work
		// No check for school, etc.
		state->grid_type = EPI_GRID_OFFICE_1;
		return;
	}

	state->grid_type = EPI_GRID_HOME;

	// agents will not have a remove time of zero.
	// set timer for minimum of ts_remove and ts_stage_tran with
	// appropriate event type.  EPI_STAGE_TRANSITION has priority.
	if (a->ts_stage_tran <= a->ts_remove)
	{
		state->tmr_remove = rn_timer_init(lp, a->ts_stage_tran);
		a->e_type = EPI_STAGE_TRANSITION;
	} else
	{
		state->tmr_remove = rn_timer_init(lp, a->dur[0]);
		a->e_type = EPI_REMOVE;
	}

	if(state->tmr_remove)
		state->tmr_remove->memory = (tw_memory *) a;
//	else
//		tw_error("tmr_remove at lp %ld for agent %u is NULL\n", lp->id, a->id);
	//printf("%ld: TMR for %u at %lf\n", lp->id, a->id, state->tmr_remove->recv_ts);
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
epi_event_handler(epi_state * state, tw_bf * bf, rn_message * msg, tw_lp * lp)
{
	epi_agent	*a;
	int		 today;

	// TO DO: need to check for last day to get last day of statistics
	today = (int) (tw_now(lp) / TWENTY_FOUR_HOURS);
	if( today > g_epi_day)
	{
		// report yesterday's statistics
		epi_report_census_tract(); 
		g_epi_day++;
	}

	tw_stime delta_time = tw_now(lp) - state->last_event;

	if(NULL == (a = (epi_agent *) tw_event_memory_get(lp)))
		tw_error(TW_LOC, "No agent on event!");

	*(int *) bf = 0;

	switch(a->e_type)
	{
		case EPI_ADD:
			// compute SEIR for existing, then add agent to LP
			// skip seir if no time has passed
			if ( (state->ncontagious > 0) && (EPSILON < delta_time) )
				epi_seir_compute(state, bf, a, lp);

			epi_agent_add(state, bf, a, lp);
			epi_num_add(state, bf, a, lp);
			break;
		case EPI_REMOVE:
			// compute SEIR for all, then remove agent from LP
			// skip seir if no time has passed
			if((state->ncontagious > 0) && (EPSILON < delta_time))
				epi_seir_compute(state, bf, a, lp);

			if (epi_agent_remove(state, bf, a, lp))
				epi_num_remove(state, bf, a, lp);

			break;
		case EPI_STAGE_TRANSITION:
			// compute seir for all, then change stage for agent
			// skip seir if no time has passed
			if( (state->ncontagious > 0) && (EPSILON < delta_time) )
				epi_seir_compute(state, bf, a, lp);

			epi_agent_stage_tran(state, bf, a, lp);
			break;
		default:
			tw_error(TW_LOC, "Unhandled event message!");
	}

	state->last_event = tw_now(lp);
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
epi_rc_event_handler(epi_state * state, tw_bf * bf, rn_message * msg, tw_lp * lp)
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
	if(g_epi_day)
	{
		epi_report_census_tract(); 
		g_epi_day = 0;
	}

	g_epi_stats->s_move_ev += state->stats->s_move_ev;
	g_epi_stats->s_nchecked += state->stats->s_nchecked;
	g_epi_stats->s_ndraws += state->stats->s_ndraws;
	g_epi_stats->s_ninfected += state->stats->s_ninfected;
	g_epi_stats->s_ndead += state->stats->s_ndead;
}
