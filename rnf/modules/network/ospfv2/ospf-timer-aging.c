#include <ospf.h>

#if 0
int int
aging_timer_increment(ospf_state * state, tw_lp * lp)
{
	int             x = (tw_now(lp) - state->db_last_ts) *
		OSPF_AGING_INCREMENT / OSPF_AGING_INTERVAL;

	if (x > 0xffffffff)
		tw_error(TW_LOC, "Rollover is going to occur!");

	return (int int)x;
}
#endif

/*
 * If the new oldest LSA is still less than the REFRESH TIME,
 * then we only need to reset the aging timer to be the
 * earlier time.  If it is not, then we need to reset the aging
 * timer for the next aging interval from now.
 */
void
ospf_aging_timer_set(ospf_state * state, ospf_db_entry * dbe, int age, tw_lp * lp)
{
	tw_stime        ts;

	int	i;
	int	incr;

#if VERIFY_AGING
	printf("%ld: Aging DB for new LSA: new last ts: %f \n", lp->gid, tw_now(lp));
#endif

	/*
	 * Have to subtract the over-aging that will occur on this lsa
	 * in the next aging timer interval.
	 */
	//dbe->b.age = age - aging_timer_increment(state, lp);

	// age them all immediately, they cannot be past the timer!
	ts = DBL_MAX;
	if(state->aging_timer)
		ts = state->aging_timer->recv_ts - tw_now(lp);

	incr = (floor(tw_now(lp) - state->db_last_ts) / 4) * 4;

	state->db_last_ts = tw_now(lp);

	// Age our LSAs, and increment the other LSA ages to the current time
	if(incr > 0)
	{
		for(i = 0; i < state->ar->g_ospf_nlsa; i++)
		{
			if(dbe->b.free)
				continue;
	
			if(dbe->b.entry == i)
				state->db[i].b.age = age;
			else
				state->db[i].b.age += incr;
	
#if VERIFY_AGING
			printf("\tLSA %d new age: %d\n", i, state->db[i].b.age);
#endif
		}
	} else
	{
		state->db[dbe->b.entry].b.age = age;
	}

	// now move the timer up, if the new LSA is going to cause the next
	// timer firing
	if(dbe->b.entry == lp->gid)
	{
		if(ts > OSPF_LSA_REFRESH_AGE - dbe->b.age)
			ts = OSPF_LSA_REFRESH_AGE - dbe->b.age;
	} else
	{
		if(ts > OSPF_LSA_MAX_AGE - dbe->b.age)
			ts = OSPF_LSA_MAX_AGE - dbe->b.age;
	}

#if VERIFY_AGING
	printf("\tdbe age now : %d \n", dbe->b.age);
	printf("\tospf_lsa_age: %d \n", ospf_lsa_age(state, dbe, lp));
	printf("\tage increment: %d \n", incr);
	printf("\tnew age: %d \n", age);
#endif

	/*
	 * Set the timer for the next aging timer interval so we can compute the
	 * new ages and reset the timer appropriately for REFRESH_AGE and MAX_AGE
	 */
	ts = (floor(tw_now(lp) / OSPF_AGING_INTERVAL) + 1) * OSPF_AGING_INTERVAL;
	ts -= tw_now(lp);
	state->aging_timer = ospf_timer_start(NULL, state->aging_timer,
				  ts, OSPF_AGING_TIMER, lp);

#if VERIFY_AGING
	if(state->aging_timer)
		printf("\tnext aging timer will fire at %lf, now %lf \n", 
				state->aging_timer->recv_ts, tw_now(lp));
	else
		printf("\tnext aging timer past end time (%f)!! \n",
					tw_now(lp) + ts);
#endif
}

/*
 * Helper function to determine if the LSA should be removed from the 
 * DB according to the RFC protocol.
 */
int
can_be_removed(ospf_state * state, ospf_db_entry * dbe, tw_lp * lp)
{
	ospf_nbr       *nbr;
	int             i;

	/*
	 * has jsut been aged! 
	 */
	if (dbe->b.age != OSPF_LSA_MAX_AGE)
		return TW_FALSE;

	for (i = 0; i < state->n_interfaces; i++)
	{
		nbr = &state->nbr[i];

		if (nbr->state == ospf_nbr_exchange_st ||
			nbr->state == ospf_nbr_loading_st)
		{
			return TW_FALSE;
		}
	}

	return TW_TRUE;
}

/*
 * When this timer fires, we should actually age the database to bring
 * it up to date.  Otherwise, the LSAs will continue to have the same age,
 * but we will move into the next refresh "window", and not be able to set
 * the aging timer properly.
 */
void
ospf_aging_timer(ospf_state * state, tw_bf * bf, tw_lp * lp)
{
	tw_stime        ts;

	ospf_db_entry  *dbe;
	ospf_lsa       *lsa;

	int             i;
	int             next_timer;
	char		rebuild_rt;
	int		aging_increment;

	ts = -1.0;
	rebuild_rt = 0;
	//aging_increment = aging_timer_increment(state, lp);
	aging_increment = (floor(tw_now(lp) - state->db_last_ts) / 4) * 4;

	state->db_last_ts = tw_now(lp);

#if VERIFY_AGING
	printf("%ld: Aging DB: new last ts: %f \n", lp->gid, tw_now(lp));
#endif

	next_timer = OSPF_LSA_MAX_AGE;
	for (i = 0; i < state->ar->g_ospf_nlsa; i++)
	{
		dbe = &state->db[i];

		if (dbe->b.free)
			continue;

		lsa = getlsa(state->m, dbe->lsa, i);

#if VERIFY_AGING
		printf("%ld: Age before aging lsa %d: %d (incr = %d), after %d\n",
			   lp->gid, lsa->id, dbe->b.age, aging_increment,
			   dbe->b.age + aging_increment);
#endif

		dbe->b.age += aging_increment;

		if (TW_TRUE == can_be_removed(state, dbe, lp))
		{
#if VERIFY_AGING
			printf("\tchecking db entry for LSA %d REMOVED (age = %d)\n",
				i, dbe->b.age);
#endif

			if (lsa->adv_r == lp->gid)
			{
				//printf("%d: my LSA reached MAX AGE! \n", lp->gid);
				state->lsa_wrapping = 0;
				state->lsa_seqnum = OSPF_MIN_LSA_SEQNUM + 1;

				dbe->b.age = 0;
				ospf_lsa_refresh(state, dbe, lp);
			} else
			{
				dbe->b.free = 1;
				//ospf_rt_build(state, lp->gid);
				rebuild_rt = 1;
			}
		} else
		{
			if (lsa->adv_r == lp->gid)
			{
				if (dbe->b.age >= OSPF_LSA_REFRESH_AGE)
				{
					dbe->b.age = 0;
					printf("\n%lld: refresh LSA %d: now %lf \n", 
						lp->gid, dbe->b.entry, tw_now(lp));
					ospf_lsa_refresh(state, dbe, lp);

					ts = OSPF_LSA_REFRESH_AGE;

					continue;
#if VERIFY_AGING
					printf("set ts: %lf, now %lf > REFRESH \n",
						   ts, tw_now(lp));
#endif
				} else
				{
					next_timer = min(OSPF_LSA_REFRESH_AGE -
							 dbe->b.age, next_timer);

#if VERIFY_AGING
					printf("\tset next_timer: %d, < REFRESH \n",
						next_timer);
#endif
				}

				if (dbe->b.age >= OSPF_LSA_REFRESH_AGE)
				{
					printf("%lld: FLOODING LSA %d for refresh!\n", 
						lp->gid, dbe->b.entry);

					ospf_lsa_flood(state, dbe, -1, lp);
					//ospf_rt_build(state, lp->gid);
					//rebuild_rt = 1;
				}
			} else
			{
				/*
				 * Do not overwrite TS, if set already by either
				 * a self-originated LSA, or otherwise.
				 */
				if (dbe->b.age >= OSPF_LSA_MAX_AGE && ts < 0.0)
				{
					dbe->b.age = OSPF_LSA_MAX_AGE;

					ts = floor(tw_now(lp) / OSPF_AGING_INTERVAL) *
						OSPF_AGING_INTERVAL;
					//printf("%d: setting (ns) ts: %f, > MAX AGE\n", lp->gid, ts);
				} else if (ts < 0.0)
				{
					next_timer = min(OSPF_LSA_MAX_AGE -
							dbe->b.age, next_timer);

/*
					printf("%d: setting (ns)next_timer: %d, < REFRESH \n",
						   lp->gid, next_timer);
*/
				}

				if (dbe->b.age >= OSPF_LSA_MAX_AGE)
				{
/*
					printf("%d: FLOODING %d LSA\n", 
								lp->gid, lsa->adv_r);
*/
					ospf_lsa_flood(state, dbe, -1, lp);
					//ospf_rt_build(state, lp->gid);
					rebuild_rt = 1;
				}
			}

		}
	}

	if(rebuild_rt)
		ospf_rt_build(state, lp->gid);

	if(ts < 0.0)
	{
		state->aging_timer = ospf_timer_start(NULL, state->aging_timer,
						  	next_timer,
						  	OSPF_AGING_TIMER, lp);
	}
	else
	{
		state->aging_timer = ospf_timer_start(NULL, state->aging_timer,
						  	ts,
						  	OSPF_AGING_TIMER, lp);
	}

#if VERIFY_AGING
	if(state->aging_timer)
		printf("\tnext aging timer to fire at %lf\n",
			state->aging_timer->recv_ts);
#endif
}
