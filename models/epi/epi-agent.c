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
void
epi_agent_update_ts(epi_state * state, tw_bf * bf, epi_agent * a, tw_lp * lp)
{
	tw_memory	*b;

	epi_pathogen	*p;

	int		 i;

	a->ts_next = a->ts_remove;

	for(i = 0; i < g_epi_ndiseases; i++)
		state->ts_seir[i] = min(state->ts_seir[i], a->ts_next);

	// need to check stage time */
	for(b = a->pathogens; b; b = b->next)
	{
		p = tw_memory_data(b);

		if(p->ts_stage_tran <= a->ts_next)
			a->ts_next = p->ts_stage_tran;

		if(p->ts_stage_tran < state->ts_seir[p->index])
		{
			epi_ic_stage *s1 = &g_epi_diseases[p->index].stages[p->stage];
			epi_ic_stage *s2 = &g_epi_diseases[p->index].stages[p->stage+1];

			if(0.0 != s1->ln_multiplier || 0.0 != s2->ln_multiplier)
				state->ts_seir[p->index] = p->ts_stage_tran;
		}
	}
}

/*
 * epi_agent_send: create new event, and pack on the agent membuf and
 * associated pathogen membufs
 */
void
epi_agent_send(tw_lp * src, tw_memory * buf, tw_stime offset, tw_lpid dst)
{
	tw_event	*e;
	tw_memory	*b;
	tw_memory	*next;

	epi_agent	*a;

	e = tw_event_new(dst, 0.0, src);
	a = tw_memory_data(buf);

	printf("%lld: Sending agent to %lld (%d, %lld) at %lf\n", 
		src->id, dst, a->curr, a->loc[a->curr], tw_now(src));

	// place agent membuf on event, and pack pathogen membufs on after that
	// in case this event/agent is sent over the network.
	for(next = b = a->pathogens; b; b = next)
	{
		next = b->next;
		tw_event_memory_set(e, b, g_epi_pathogen_fd);
	}

	tw_event_memory_set(e, buf, g_epi_fd);
	a->pathogens = NULL;

	tw_event_send(e);
}

epi_ic_stage	*
epi_agent_infected(epi_agent * a, int disease)
{
	tw_memory	*b;
	epi_pathogen	*p;
	epi_ic_stage	*s;

	for(b = a->pathogens; b; b = b->next)
	{
		p = tw_memory_data(b);

		if(p->index == disease)
		{
			s = &g_epi_diseases[p->index].stages[p->stage];

			if(0.0 != s->ln_multiplier)
				return s;

#if SORTED_PATHOGENS
			if(p->index >= disease)
				return NULL;
#endif
		}
	}

	return NULL;
}

/*
 * Stage transition causes the agent to be removed from the
 * queue (it is at the head), increment the stage, collect
 * statistics, and place back in the queue with the minimum
 * of the next stage transition time and the remove time.
 */
void
agent_stage_tran(epi_state * state, tw_bf * bf, tw_memory * buf, epi_pathogen * p, tw_lp * lp)
{
	epi_ic_stage	*s;
	epi_agent	*a;

	a = tw_memory_data(buf);
	s = &g_epi_diseases[p->index].stages[p->stage];

	if(5 == a->id || 1)
		printf("%lld: transition %d at %lf \n", lp->gid, a->id && 0, tw_now(lp));

	// if this agent is contagious in the old stage, then reduce the location number of
	// contagious agents by one.
	if(s->ln_multiplier != 0.0)
	{
		state->ncontagious[p->index]--;
		state->ncontagious_tot--;

		if(a->behavior_flags == EPI_AGENT_WORRIED_WELL)
			a->behavior_flags = 0;
	}

	if(state->ncontagious[p->index] < 0)
		tw_error(TW_LOC, "Less than zero ncontagious!");

	g_epi_regions[a->region][p->index][p->stage]--;
	p->stage++;
	g_epi_regions[a->region][p->index][p->stage]++;

	if(p->stage >= g_epi_diseases[p->index].nstages)
		tw_error(TW_LOC, "Invalid stage: %d", p->stage);

	s = &g_epi_diseases[p->index].stages[p->stage];
	p->ts_stage_tran = tw_now(lp);

	if(fabs(s->max_duration - s->min_duration) < EPSILON)
		p->ts_stage_tran += s->min_duration;
	else
		p->ts_stage_tran += s->min_duration + 
				tw_rand_unif(lp->rng) * (s->max_duration - s->min_duration);

	if(p->ts_stage_tran > tw_now(lp) + s->max_duration)
		tw_error(TW_LOC, "Too long!");

	if(p->ts_stage_tran < tw_now(lp))
		tw_error(TW_LOC, "Backwards stage tran!");

	if(p->ts_stage_tran < s->min_duration)
		tw_error(TW_LOC, "Too short!");

	// Perform mortality check
	if (s->mortality_rate > 0 && tw_rand_unif(lp->rng) < s->mortality_rate )
	{
		a->behavior_flags = EPI_AGENT_DEAD;
		a->days_remaining = 0;
		g_epi_regions[a->region][p->index][p->stage]--;
		p->stage = g_epi_diseases[p->index].nstages;
		g_epi_regions[a->region][p->index][p->stage]++;

		// collect statistics
		state->stats->s_ndead++;
	} else
	{
		// if new stage has infectivity, 
		// then increment number of contagious agents in location.
		if(s->ln_multiplier != 0.0)
		{
			state->ncontagious[p->index]++;
			state->ncontagious_tot++;
			state->stats->s_ninfected++;

			// then must set number of days to stay home
			if(!(a->behavior_flags & EPI_AGENT_WORK_WHILE_SICK))
			{
				a->days_remaining = ((int) (p->ts_stage_tran - tw_now(lp)) / TWENTY_FOUR_HOURS);

				if (a->days_remaining < 1)
					a->days_remaining = 1;

				g_epi_hospital[state->hospital][0]++;
			}
		}

		epi_agent_update_ts(state, bf, a, lp);

		// re-enqueue with new ts_next
		pq_enqueue(g_epi_pq[lp->id], buf);
	}
}

	/*
	 * epi_agent_contagious: determine if an agent is contagious,
	 * and update state ncontagious variables for each disease
	 */
void
epi_agent_contagious(epi_state * state, tw_memory * buf, signed int offset)
{
	tw_memory	*b;

	epi_agent	*a;
	epi_pathogen	*p;

	a = tw_memory_data(buf);

	for(b = buf->next; b; b = b->next)
	{
		p = tw_memory_data(b);

		if(g_epi_diseases[p->index].stages[p->stage].ln_multiplier != 0.0)
		{
			state->ncontagious_tot += offset;
			state->ncontagious[p->index] += offset;
		}
	}
}

int
epi_agent_stage_tran(epi_state * state, tw_bf * bf, tw_memory * buf, tw_lp * lp)
{
	tw_memory	*b;

	epi_agent	*a;
	epi_pathogen	*p;

	a = tw_memory_data(buf);
	for(b = a->pathogens; b; b = b->next)
	{
		p = tw_memory_data(b);

		if(p->ts_stage_tran == tw_now(lp))
		{
			agent_stage_tran(state, bf, buf, p, lp);
			return 1;
		}
	}

	return 0;
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
epi_agent_add(epi_state * state, tw_bf * bf, tw_memory * buf, tw_lp * lp)
{
	epi_agent	*a;

	a = tw_memory_data(buf);

	if(5 == a->id && 0)
		printf("%lld: added %d at %lf \n", lp->gid, a->id && 0, tw_now(lp));

	// Test to make sure this location is this lp
	if (lp->gid != a->loc[a->curr])
		tw_error(TW_LOC, "epi_agent_add adding agent to lp %d but loc[%d] is %d",
			 lp->gid, a->curr, a->loc[a->curr]);

	// if agent is contagious, increment number of contagious agents here
	epi_agent_contagious(state, buf, +1);

	a->ts_remove = tw_now(lp) + a->dur[a->curr];
	epi_agent_update_ts(state, bf, a, lp);

	pq_enqueue(g_epi_pq[lp->id], buf);

	if(pq_get_size(g_epi_pq[lp->id]) > state->max_queue_size)
		state->max_queue_size = pq_get_size(g_epi_pq[lp->id]);

	// If not at home, if agent is a worried well, if there are already enough symtomatic
	// people here then starting tomorrow, this agent will stay home for the number
	// of days remaining.  days_remaining is checked in agent_remove().
	
	if(!a->days_remaining && a->nloc > 1 && a->loc[a->curr] != a->loc[0] && 
		(a->behavior_flags & EPI_AGENT_WORRIED_WELL))
	{
		if ((float) state->ncontagious_tot / (float) state->max_queue_size > 
				g_epi_ww_threshold)
		{
			a->days_remaining = g_epi_ww_duration;
			a->behavior_flags = 0;
			g_epi_hospital_ww[state->hospital][0]++;
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
epi_agent_add_back(epi_state * state, tw_lp * lp, tw_memory * buf)
{
	tw_memory	*b;

	epi_agent	*a;
	epi_pathogen	*p;

	a = tw_memory_data(buf);

	if(5 == a->id && 0)
		printf("%lld: added back %d at %lf \n", lp->gid, a->id && 0, tw_now(lp));

	// Need to move curr but not change location.
	if(++a->curr >= a->nloc)
		a->curr = 0;

	a->ts_next = a->ts_remove = tw_now(lp) + a->dur[a->curr];

	for(b = a->pathogens; b; b = b->next)
	{
		p = tw_memory_data(b);

		if (p->ts_stage_tran <= a->ts_remove)
			a->ts_next = p->ts_stage_tran;
	}

	pq_enqueue(g_epi_pq[lp->id], buf);

	//epi_num_add(state, (tw_bf *) NULL, a, lp);
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
	tw_memory	*buf;

	epi_agent	*a;

	while(NULL != (buf = pq_dequeue(g_epi_pq[lp->id])))
	{
		a = tw_memory_data(buf);

		if(a->ts_next != tw_now(lp))
		{
			pq_enqueue(g_epi_pq[lp->id], buf);
			break;
		}

		if(epi_agent_stage_tran(state, bf, buf, lp))
			continue;


		// if days_remaining > 0, then go through motions by moving curr, but
		// do not move agent.  Thus, a->loc[a->curr] may not be home.  Need
		// to do this in order to send network traffic.
		// only change days_remaining at midnight, so when days_remaining goes
		// to zero, curr will go to zero and agent will be at home.
		// Make sure agent is at home.  If agent becomes worried well at
		// work, must continue to move until he is at home.

		if(a->days_remaining && lp->gid == a->loc[0])
		{
			if ((int) tw_now(lp) % TWENTY_FOUR_HOURS == 0)
			{
				if(--a->days_remaining)
				{
					epi_agent_add_back(state, lp, buf);
					continue;
				}
			} else
			{
				epi_agent_add_back(state, lp, buf);
				continue;
			}
		}

		if(5 == a->id || 1)
			printf("%lld: REM %d at %lf \n", lp->gid, a->id, tw_now(lp));

		//epi_num_remove(state, bf, a, lp);

		// if agent is contagious, decrement number of 
		// contagious agents in this location
		epi_agent_contagious(state, buf, -1);
	
		if (state->ncontagious < 0)
			tw_error(TW_LOC,
				"Less than zero contagious agents in %lld", lp->gid);
	
		// increment location
		if(++a->curr >= a->nloc)
			a->curr = 0;

		epi_agent_send(lp, buf, 0.0, a->loc[a->curr]);
	}
}
