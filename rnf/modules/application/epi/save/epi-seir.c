#include <epi.h>

/*
 * epi-seir.c:
 */

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
epi_seir_compute(epi_state * state, tw_bf * bf, epi_agent * in_a, tw_lp * lp)
{
	/*
	 * For each susceptible agent, for each contagious agent, determine
	 * the susceptible agent caught the disease.
	 */

	double		 draw,
			 delta_t;
	epi_agent	*a, *ai;
	epi_ic_stage	*s, *sn;
	unsigned int	 queue_size;
	unsigned int	 i, j;

	void		*pq;

	queue_size = pq_get_size(state->pq);
	if (queue_size <= 1)
		return;

	delta_t = tw_now(lp) - state->last_event;

	//printf(" lp %2d SEIR delta_t %f at %f\n", (int) lp->id, delta_t, tw_now(lp));

	if (delta_t < EPSILON) // already done an seir check at this time.
		return;

	//printf("SEIR Check %f\n", tw_now(lp));

	pq = state->pq;

	//printf( "Queue size: %d  ", queue_size);

	for ( i = 0; i < queue_size; i++)
	{
		a = (epi_agent *) pq_next( pq, i);
		//printf("\ta: %d, stage: %d\n ", a->id, a->stage);
		if (a->stage == EPI_SUSCEPTIBLE)
		{
			for (j = 0; j < queue_size; j++)
			{
				if ( j == i ) continue;
				ai = (epi_agent *) pq_next(pq, j);
				s = &g_epi_stages[ai->stage];
				//printf("a: %d, ai: %d, ln_multiplier: %f\n", a->id, ai->id, s->ln_multiplier);
				if (abs(s->ln_multiplier) > EPSILON)
				{
					// tw_geometric draws from uniform until the draw is greater than P.
					// tw_geometric then returns the number of draws.
					// For large P (which is 1.0 - multiplier) this could be very large.
					// Also note that tw_geometric always returns an integer of 1 or
					// greater.
					draw = tw_rand_unif(lp->id);

					draw = log(draw);
					draw /= g_epi_stages[a->stage].ln_multiplier;

					//draw = tw_rand_geometric(lp->id, (s->start_multiplier + s->stop_multiplier)/2);
					//state->stats->s_ndraws += draw;
					state->stats->s_nchecked++;
					//printf("SEIR agent %d, stage %d, agent %d, stage %d, draw: %g\n",
					//	a->id, a->stage, ai->id, ai->stage, draw);
					if (draw <= delta_t) //Make agent sick
					{
#if EPI_DEBUG
						printf("%7.3f, lp, %7d, agent_infect, , , , %7d, %5.3f, %5.2f\n",
							tw_now(lp), (int) lp->id, a->id, delta_t, draw);
#endif
						a->stage = EPI_INCUBATING;
						g_epi_ct[a->ct][0]--;
						g_epi_ct[a->ct][1]++;
						if (g_epi_position_f)
							fprintf(g_epi_position_f, "%.3f,%d,%d,%d\n", tw_now(lp), a->id, (int) lp->id, a->stage);

						sn = &g_epi_stages[a->stage];
						a->ts_infected = tw_now(lp);
						a->ts_stage_tran = tw_rand_unif(lp->id)*(sn->max_duration - sn->min_duration)
								+ sn->min_duration + tw_now(lp);
						a->ts_last_tran = tw_now(lp);
						// collect statistics
						state->stats->s_ninfected++;
						break;
					}
#if EPI_DEBUG
					else
					{
						printf("%7.3f, lp, %7d, agent_not_infect, , , , %7d, %5.3f, %5.2f\n",
							tw_now(lp), (int) lp->id, a->id, delta_t, draw);
					}
#endif
				}
			}
		}
	}	
 
	//printf("SEIR compputed at %f ", state->last_event);
}
