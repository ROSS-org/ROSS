#include <epi.h>

void
timer_helper(epi_state * state, tw_lp * lp)
{
	epi_agent	 *a;

	// timer fired, so reset time to next lowest agent
	// Note that queue may be empty because the only agent in this location died.
	if(NULL != (a = pq_top(state->pq)))
	{
		if (a->ts_stage_tran <= a->ts_remove)
		{
			a->e_type = EPI_STAGE_TRANSITION;
			state->tmr_remove = epi_timer_start(a, state->tmr_remove,
				a->ts_stage_tran, lp);
		} else
		{
			a->e_type = EPI_REMOVE;
			state->tmr_remove = epi_timer_start(a, state->tmr_remove,
				a->ts_remove, lp);
		}
	} else
		state->tmr_remove = NULL;
}

/*
 * State changes occur when an agent is added or removed from a group,
 * or when an agent changes stage.  In either case, a check is made
 * to see if anyone becomes infected (change from stage 0 to stage 1).
 * The timer for the next event can be for a remove or for stage
 * transition.  Adds are performed by a message from another group.
 * Each agent has two possible times for the next event, the remove
 * time (ts_remove) and the stage change time (ts_stage_tran).
 */

/*
 * Stage transition causes the agent to be removed from the
 * queue (it is at the head), increment the stage, collect
 * statistics, and place back in the queue with the minimum
 * of the next stage transition time and the remove time.
 */
void
epi_agent_stage_tran(epi_state * state, tw_bf * bf, epi_agent * a, tw_lp * lp)
{
	epi_ic_stage	*s;

	// this agent is at head of pq
	a = pq_dequeue(state->pq);

	// simple error checking
	if(NULL == a)
		tw_error(TW_LOC, "Empty PQ!");

	// if this agent is contagious in the old stage, then reduce the location number of
	// contagious agents by one.
	s = &g_epi_stages[a->stage];
	if (abs(s->ln_multiplier) > EPSILON)
		state->ncontagious--;

	if(state->ncontagious < 0)
		tw_error(TW_LOC, "Less than zero ncontagious!");

	g_epi_ct[a->ct][a->stage]--;
	a->stage++;
	g_epi_ct[a->ct][a->stage]++;

	if (a->stage >= g_epi_nstages)
		tw_error(TW_LOC, "Invalid stage: %d", a->stage);

	s = &g_epi_stages[a->stage];
	a->ts_last_tran = tw_now(lp);
	if (abs(s->max_duration - s->min_duration) < EPSILON)
		a->ts_stage_tran += s->min_duration;
	else
		a->ts_stage_tran += s->min_duration + 
				tw_rand_unif(lp->id) * (s->max_duration - s->min_duration);

	// Perform mortality check
	if (tw_rand_unif(lp->id) < s->mortality_rate )
	{
		pq_dequeue(state->pq);
		g_epi_ct[a->ct][a->stage]--;
		a->stage = g_epi_nstages;
		a->days_remaining = 0;
		g_epi_ct[a->ct][g_epi_nstages]++;

		// collect statistics
		state->stats->s_ndead++;
	} else
	{
		// if new stage has infectivity, 
		// then increment location number of contagious agents.
		if (abs(s->ln_multiplier) > EPSILON)
		{
			state->ncontagious++;
		
			// then must set number of days to stay home
			if(!(a->behavior_flags & EPI_AGENT_WORK_WHILE_SICK))
				a->days_remaining = ((int) a->ts_stage_tran / tw_now(lp)) + 1;
		}

		if (a->ts_stage_tran <= a->ts_remove)
			a->ts_next = a->ts_stage_tran;
		else
			a->ts_next = a->ts_remove;

		// re-enqueue with new ts_next
		pq_enqueue(state->pq, a);
	}

	timer_helper(state, lp);
}

	/*
	 * Adding an agent means placing them into our PQ.  If agent
	 * remove ts is < PQ->min, then must update LP remove timer.
	 *
	 * Check time for stage change.  Place in queue by minimum
	 * stage change time or remove time.  Set timer for stage
	 * change time (and event) or remove time (and event).
	 */
void
epi_agent_add(epi_state * state, tw_bf * bf, epi_agent * a, tw_lp * lp)
{
	// Test to make sure this location is this lp
	if (lp->id != a->loc[a->curr])
		tw_error(TW_LOC, "epi_agent_add adding agent %d to lp %d but loc[%d] is %d",
			 a->id, lp->id, a->curr, a->loc[a->curr]);

	// if agent is contagious, increment number of contagious agents here
	if (abs(g_epi_stages[a->stage].ln_multiplier) > EPSILON)
		state->ncontagious++;

	a->ts_remove = tw_now(lp) + a->dur[a->curr];

	/* need to check stage time */
	if (a->ts_stage_tran <= a->ts_remove)
		a->ts_next = a->ts_stage_tran;
	else
		a->ts_next = a->ts_remove;

	pq_enqueue(state->pq, a);

	if(pq_get_size(state->pq) > state->max_queue_size)
		state->max_queue_size = pq_get_size(state->pq);

// If not at home, if agent is a worried well, if there are already enough symtomatic
// people here then starting tomorrow, this agent will stay home for the number
// of days remaining.  days_remaining is checked in agent_remove().
	
	if(!a->days_remaining && a->nloc > 1 && a->loc[a->curr] != a->loc[0] && 
		(a->behavior_flags & EPI_AGENT_WORRIED_WELL))
	{
		if ((float) state->ncontagious / (float) state->max_queue_size > 
				g_epi_worried_well_threshold)
		{
			a->days_remaining = g_epi_worried_well_duration;
		}
	}

	// Update the REMOVE timer.
	// in case of tie in PQ, the timer agent should match the PQ min agent
/*
	if(!state->tmr_remove || 
	   a->ts_next < state->tmr_remove->recv_ts ||
	   a == pq_top(state->pq))
*/
	timer_helper(state, lp);
}

	/*
	 * epi_agent_add_back: helper routine that adds an agent back to the
	 *                     queue from which it was just removed
	 *        Must call num_add if appropriate, since we are skipping
	 *        the EPI_ADD event
	 */
void
epi_agent_add_back(epi_state * state, tw_lp * lp, epi_agent * a)
{
	// Need to move curr but not change location.
	if(++a->curr >= a->nloc)
		a->curr = 0;

	a->ts_remove = tw_now(lp) + a->dur[a->curr];

	if (a->ts_stage_tran <= a->ts_remove)
		a->ts_next = a->ts_stage_tran;
	else
		a->ts_next = a->ts_remove;

	pq_enqueue(state->pq, a);
	epi_num_add(state, (tw_bf *) NULL, a, lp);

	timer_helper(state, lp);
}

	/*
	 * When the LPs REMOVE timer fires, we should dequeue the next agent
	 * and send it on to it's next location.  We must set the next remove
	 * timer appropriately for the agents remaining in our PQ.
	 *
	 * If the remove is from location 0, then this is the first move of the
	 * day, i.e., from home.  We activate agent logic to see if agent
	 * will stay home today, due to quarantine, being ill, or having
	 * ill spouse or children to care for.
	 *
	 * NOTE: We want the agent in the event to be consistent with the
	 * agent in the timer, even if they have the same remove timestamp.
	 * The reason for this is that in the reverse computation, I need to put
	 * the agent back into the PQ, and where else can I get it from but the
	 * timer event?
	 *
	 */
int
epi_agent_remove(epi_state * state, tw_bf * bf, epi_agent * a, tw_lp * lp)
{
	tw_event	*e;

	a = pq_dequeue(state->pq);

	// simple error checking
	if(NULL == a)
		tw_error(TW_LOC, "Empty PQ!");

	// if days_remaining > 0, then go through motions by moving curr, but
	// do not move agent.  Thus, a->loc[a->curr] may not be home.  Need
	// to do this in order to send network traffic.
	// only change days_remaining at midnight, so when days_remaining goes
	// to zero, curr will go to zero and agent will be at home.
	// Make sure agent is at home.  If agent becomes worried well at
	// work, must continue to move until he is at home.
	if(a->days_remaining && lp->id == a->loc[0])
	{
		if ((int) tw_now(lp) % TWENTY_FOUR_HOURS == 0)
		{
			if(--a->days_remaining)
			{
				epi_agent_add_back(state, lp, a);

				return 0;
			}
		} else
		{
			epi_agent_add_back(state, lp, a);

			return 0;
		}
	}

	// if agent is contagious, decrement number of contagious agents in this location
	if (abs(g_epi_stages[a->stage].ln_multiplier) > EPSILON)
		state->ncontagious--;

	if (state->ncontagious < 0)
		tw_error(TW_LOC,"Less than zero contagious agents in %d", lp->id);

	// increment location
	if(++a->curr >= a->nloc)
		a->curr = 0;

	e = rn_event_direct(tw_getlp(a->loc[a->curr]), 0.0, lp);
	e->memory = (tw_memory *) a;
	a->e_type = EPI_ADD;

	rn_event_send(e);

	timer_helper(state, lp);

	// Collect statistics
	state->stats->s_move_ev++;

	return 1;
}
