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
epi_seir_compute(epi_state * state, tw_bf * bf, tw_lp * lp)
{
	tw_stime	 draw;
	tw_stime	 delta_t;

	epi_agent	*a;
	epi_agent	*b;

	epi_ic_stage	*s;

	unsigned int	 q_sz;
	unsigned int	 i;
	unsigned int	 j;

	void		*pq = state->pq;

	delta_t = tw_now(lp) - state->last_event;

#if VERIFY_SEIR
	printf("%ld: SEIR_COMP delta_t %lf at %lf\n", lp->id, delta_t, tw_now(lp));
#endif

	for(i = 0, q_sz = pq_get_size(pq); i < q_sz; i++)
	{
		a = pq_next(pq, i);

		if(a->stage != EPI_SUSCEPTIBLE)
			continue;

		// do not go through total location population, define
		// contact pop as subset of overall, and walk through
		// iteratively, or randomly
		for (j = 0; j < q_sz; j++)
		{
			if(j == i)
				continue;

			b = pq_next(pq, j);
			s = &g_epi_stages[b->stage];

			if(s->ln_multiplier != 0.0)
			{
#if 0
				// tw_geometric draws from uniform until the draw 
				// is greater than P.
				// tw_geometric then returns the number of draws.
				// For large P (which is 1.0 - multiplier) this could 
				// be very large.  Also note that tw_geometric 
				// always returns an integer of 1 or greater.
				draw = tw_rand_geometric(lp->id, 
					(s->start_multiplier + s->stop_multiplier)/2);
				state->stats->s_ndraws += draw;
				state->stats->s_nchecked++;
#endif

				// Or, we can compute it directly as:
				// draw = log(random draw) / log(1 - prob of contagiousness)

				draw = tw_rand_unif(lp->rng);

				draw = log(draw);
				draw /= s->ln_multiplier;


				// multiply draw to factor in contact population
				draw *= q_sz;

				// Make agent sick
				if(draw <= delta_t)
				{
#if VERIFY_SEIR
					printf("%ld: pop %d, draw %lf, delta_t %lf \n",
						lp->id, q_sz, draw, delta_t);
#endif

					g_epi_ct[a->ct][a->stage]--;
					a->stage++; // = EPI_INCUBATING;
					g_epi_ct[a->ct][a->stage]++;
					//g_epi_exposed_today++;
					
					s = &g_epi_stages[a->stage];
					a->ts_stage_tran = tw_rand_unif(lp->rng) * 
							(s->max_duration - s->min_duration) + 
							s->min_duration + tw_now(lp);

					break;
				}
			}
		}
	}
}
