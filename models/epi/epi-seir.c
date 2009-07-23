#include <epi.h>

/*
 * epi-seir.c:
 */
/*
 * determine if agent is susceptible to a given disease
 * an agent is susceptible if they do not have a stage for the disease
 * in their pathogens array, which is sorted
 */
int
agent_susceptible(epi_agent * a, int disease, tw_lp * lp)
{
	tw_memory	*b;
	tw_memory	*last;
	epi_pathogen	*p;

	for(b = a->pathogens, last = NULL; b; last = b, b = b->next)
	{
		p = tw_memory_data(b);

		if(p->index == disease && p->stage == EPI_SUSCEPTIBLE)
		{
			// remove from pathogen list and free buffer
			if(last)
				last->next = b->next;
			else
				a->pathogens = b->next;

			tw_memory_free(lp, b, g_epi_pathogen_fd);

			return 1;
		}

		if(p->index == disease)
			return 0;
	}

	return 1;
}


/*
 * epi_seir_compute: compute SEIR model for all agents in location LPn
 *
 * Assume that the probability of catching the disease is constant over time.
 * If the probability of catching the disease in one hour is P, then the
 * geometric distribution gives the number of hours before the disease is
 * caught.  We do a draw on the random number generator from the geometric
 * distribution and if the result is less than the time interval, then the
 * agent gets sick at that time.
 */
void
epi_seir_compute(epi_state * state, tw_bf * bf, int disease, tw_lp * lp)
{
	tw_stime	 draw;

	tw_memory	*sus_b;
	tw_memory	*b;

	epi_agent	*sus;
	epi_agent	*inf;

	epi_pathogen	*p;
	epi_ic_stage	*s;

	unsigned int	 i;
	unsigned int	 j;

	void		*pq = g_epi_pq[lp->id];
	unsigned int	 q_sz = pq_get_size(pq);;

#if VERIFY_SEIR
	printf("%ld: SEIR_COMP at %lf\n", lp->id, tw_now(lp));
#endif

	if(1)
		return;

	if(0 == state->ncontagious[disease])
		tw_error(TW_LOC, "in epi_seir_compute, but not infectious!");

	// foreach person susceptible to this disease,
	// compute the time when they would become infected
	for(i = 0; i < q_sz; i++)
	{
		sus_b = pq_next(pq, i);
		sus = tw_memory_data(sus_b);

		if(agent_susceptible(sus, disease, lp))
			continue;

		for(j = 0; j < q_sz; j++)
		{
			inf = tw_memory_data(pq_next(pq, j));

			if(NULL == (s = epi_agent_infected(inf, disease)))
				continue;

#if 0
			direct computation, high level, draw is ts
			of future infection

			draw = log(random draw) / 
				log(1 - prob of contagiousness)
#endif

			draw = tw_rand_unif(lp->rng);

			draw = log(draw);
			draw /= s->ln_multiplier;

			// multiply draw to factor in contact population
			draw *= q_sz;

			// do nothing if draw TS is > pq_min element
			if(draw > state->ts_seir[disease])
				continue;

			// Make agent sick

			// allocate pathogen and attach to agent
			if(!b)
			{
				b = tw_memory_alloc(lp, g_epi_pathogen_fd);
				p = tw_memory_data(b);
			}

			b->next = sus->pathogens;
			sus->pathogens = b;

			// fill in pathogen
			p->index = disease;
			p->stage = EPI_SUSCEPTIBLE;
			p->ts_stage_tran = draw;

			// now must remove agent from PQ, and send
			// stage transition event for SUS->EXP

			if(sus->ts_remove <= p->ts_stage_tran)
				tw_error(TW_LOC, "foobar");

			if(sus->ts_next > p->ts_stage_tran)
				sus->ts_next = p->ts_stage_tran;

			//pq_delete_any(pq, &sus_b);
			//epi_agent_send(lp, sus_b, draw, lp);

#if VERIFY_SEIR
			printf("%ld: pop %d, draw %lf, delta_t %lf \n",
				lp->id, q_sz, draw, delta_t);
#endif
		}
	}
}
