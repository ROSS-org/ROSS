#include <epi.h>

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
	if(s->ln_multiplier != 0.0)
		state->ncontagious--;

	if(state->ncontagious < 0)
		tw_error(TW_LOC, "Less than zero ncontagious!");

	g_epi_ct[a->ct][a->stage]--;
	a->stage++;
	g_epi_ct[a->ct][a->stage]++;

	if (a->stage >= g_epi_nstages)
		tw_error(TW_LOC, "Invalid stage: %d", a->stage);

	s = &g_epi_stages[a->stage];
	//a->ts_last_tran = tw_now(lp);
	a->ts_stage_tran = tw_now(lp);

	if (fabs(s->max_duration - s->min_duration) < EPSILON)
		a->ts_stage_tran += s->min_duration;
	else
		a->ts_stage_tran += s->min_duration + 
				tw_rand_unif(lp->rng) * (s->max_duration - s->min_duration);

	if(a->ts_stage_tran > tw_now(lp) + s->max_duration)
		tw_error(TW_LOC, "Too long!");

	// Perform mortality check
	if(s->mortality_rate > 0 && tw_rand_unif(lp->rng) < s->mortality_rate )
	{
		g_epi_ct[a->ct][a->stage]--;
		a->stage = g_epi_nstages;
		a->days_remaining = 0;
		g_epi_ct[a->ct][a->stage]++;

		// collect statistics
		state->stats->s_ndead++;
	} else
	{
		// if new stage has infectivity, 
		// then increment location number of contagious agents.
		if(s->ln_multiplier != 0.0)
		{
			state->ncontagious++;
			state->stats->s_ninfected++;

			if(a->behavior_flags == EPI_AGENT_WORRIED_WELL)
				a->behavior_flags = 0;
		
			// then must set number of days to stay home
			if(!(a->behavior_flags & EPI_AGENT_WORK_WHILE_SICK))
			{
				a->days_remaining = ((int) (a->ts_stage_tran - tw_now(lp)) / TWENTY_FOUR_HOURS);
				if (a->days_remaining < 1)
					a->days_remaining = 1;

				g_epi_hospital++;
			}
		}

		if (a->ts_stage_tran <= a->ts_remove)
			a->ts_next = a->ts_stage_tran;
		else
			a->ts_next = a->ts_remove;

		// re-enqueue with new ts_next
		pq_enqueue(state->pq, a);
	}
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
		tw_error(TW_LOC, "epi_agent_add adding agent to lp %d but loc[%d] is %d",
			 lp->id, a->curr, a->loc[a->curr]);

	// if agent is contagious, increment number of contagious agents here
	if(g_epi_stages[a->stage].ln_multiplier != 0.0)
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
			a->behavior_flags = 0;
		}
	}
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
void
epi_agent_remove(epi_state * state, tw_bf * bf, tw_lp * lp)
{
	tw_event	*e;

	epi_agent	*a;

	while(NULL != (a = pq_top(state->pq)) && a->ts_next == tw_now(lp))
	{
		if(a->ts_stage_tran == tw_now(lp))
		{
			epi_agent_stage_tran(state, bf, a, lp);
			continue;
		}

		a = pq_dequeue(state->pq);

		// if stay-at-home flag is set, then set days_remaining > 0.  This
		// causes everyone to stay at home

		if (g_epi_stay_home > 0)
			a->days_remaining = 180;

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
					continue;
				}
			} else
			{
				epi_agent_add_back(state, lp, a);
				continue;
			}
		}

		epi_num_remove(state, bf, a, lp);

		// if agent is contagious, decrement number of 
		// contagious agents in this location
		if(g_epi_stages[a->stage].ln_multiplier != 0.0)
			state->ncontagious--;
	
		if (state->ncontagious < 0)
			tw_error(TW_LOC,
				"Less than zero contagious agents in %d", lp->id);
	
		// increment location
		if(++a->curr >= a->nloc)
			a->curr = 0;
	
		e = rn_event_direct(a->loc[a->curr], 0.0, lp);
		e->memory = (tw_memory *) a;
	
		rn_event_send(e);
	}
}
