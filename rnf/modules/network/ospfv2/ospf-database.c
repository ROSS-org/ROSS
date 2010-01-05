#include <ospf.h>

void flood_summary(ospf_state * state, ospf_db_entry * dbe, int from, tw_lp * lp);

void
ospf_db_route(ospf_state * state, ospf_db_entry * curr, tw_lp * lp)
{
	tw_memory	*b;

	ospf_lsa_link	*l;
	ospf_lsa	*lsa;
	int		 d = -1;

	if(curr->b.free)
	{
#if VERIFY_DATABASE
		printf("\t%ld: removing rn_route to %d! \n", lp->gid, curr->b.entry);
#endif

		rn_route_change(state->m, curr->b.entry, -1);
		state->m->nhop[curr->b.entry] = -1;
		return;
	}

	lsa = getlsa(state->m, curr->lsa, curr->b.entry);

	if(lsa->adv_r == lp->gid)
	{
		for(b = lsa->links.head; b; b = b->next)
		{
			l = tw_memory_data(b);

			d = rn_route(state->m, l->dst);

			if(-1 == d)
				continue;

			rn_route_change(state->m, curr->b.entry, d);
			state->m->nhop[curr->b.entry] = 
					state->m->nhop[lp->gid - state->ar->low];
			break;
		}

		// Could not find route.. 
		if(-1 == d)
		{
			rn_route_change(state->m, curr->b.entry, -1);
			state->m->nhop[curr->b.entry] = -1;
		}
	} else
	{
		d = rn_route(state->m, lsa->adv_r);
		rn_route_change(state->m, curr->b.entry, d);
		state->m->nhop[curr->b.entry] = 
				state->m->nhop[lsa->adv_r - state->ar->low];
	}

#if VERIFY_DATABASE
	printf("\t%ld: adding rn_route for DB entry %d: adv_r %d, nhi %d \n", 
		lp->gid, curr->b.entry, lsa->adv_r, d);
#endif
}

void
ospf_db_print_raw(ospf_state * state, FILE * log, tw_lp * lp)
{
	ospf_db_entry	*dbe;
	int		 i;

	fprintf(log, "%lf\n", state->db_last_ts);

	for(i = 0; i < state->ar->g_ospf_nlsa; i++)
	{
		dbe = &state->db[i];

		fprintf(log, "%d %d %d %d %d %d %d\n",
			dbe->b.free, dbe->b.age, dbe->b.entry, 
			dbe->lsa, dbe->seqnum, dbe->cause_bgp, dbe->cause_ospf);
	}
}

void
ospf_db_read(ospf_state * state, tw_lp * lp)
{
	tw_memory	*b;
	//tw_stime	 db_last_ts;

	ospf_db_entry	*dbe;
	ospf_lsa	*lsa;
	ospf_lsa_link	*l;

	FILE		*fp;
	int		 age;
	int		 free;
	int		 entry;
	int	 	 id;
	int		 size;
	int		 i;
	int		 j;

	fp = g_ospf_lsa_input;

	state->db = (ospf_db_entry *) 
			calloc(sizeof(ospf_db_entry), state->ar->g_ospf_nlsa);

	if(!state->db)
		tw_error(TW_LOC, "Out of memory for local LSA database!");

	fscanf(fp, "%lf", &state->db_last_ts);

	if(!state->db_last_ts)
		state->db_last_ts = 1;

	if(state->ar->low == lp->gid)
	{
		//printf("%ld: Reading in LSAs for area: %d \n", lp->gid, state->ar->id);
		state->ar->g_ospf_lsa = (tw_memory **) calloc(sizeof(tw_memory *) *
						state->ar->g_ospf_nlsa, 1);

		if(!state->ar->g_ospf_lsa)
			tw_error(TW_LOC, "Unable to allocate global LSA table!");

		fscanf(fp, "%d", &id);
		for(i = 0; i < state->ar->g_ospf_nlsa; i++)
		{
			state->ar->g_ospf_lsa[i] = tw_memory_alloc(lp, g_ospf_lsa_fd);
			lsa = tw_memory_data(state->ar->g_ospf_lsa[i]);

			while(id == i)
			{
				lsa->type = ospf_lsa_router;
				lsa->id = id;

				fscanf(fp, "%d %d", &lsa->adv_r, &size);

				for(j = 0; j < size; j++)
				{
					b = tw_memory_alloc(lp, g_ospf_ll_fd);
					l = tw_memory_data(b);

					fscanf(fp, "%d %d", &l->dst, &l->metric);
					tw_memoryq_push(&lsa->links, b);
				}

				fscanf(fp, "%d", &id);

				if(id == i)
				{
#if VERIFY_OSPF_READ
					printf("\t");
					ospf_lsa_print(stdout, lsa);
#endif
					lsa->next = tw_memory_alloc(lp, g_ospf_lsa_fd);
					lsa = tw_memory_data(lsa->next);
				} else
				{
#if VERIFY_OSPF_READ
					printf("\t");
					ospf_lsa_print(stdout, lsa);
#endif
				}
			}
		}
	}

#if VERIFY_OSPF_READ || 1
	printf("%lld: Database: %lf \n", lp->gid, state->db_last_ts);
#endif

	for(i = 0; i < state->ar->g_ospf_nlsa; i++)
	{
		dbe = &state->db[i];

		fscanf(fp, "%d %d %d %d %d %c %c",
			&free, &age, &entry, &dbe->lsa, &dbe->seqnum, 
			&dbe->cause_bgp, &dbe->cause_ospf);

		dbe->b.free = free;
		//dbe->b.age = age;
		dbe->b.age = 0;
		dbe->b.entry = entry;
		dbe->cause_bgp = dbe->cause_ospf = 0;

#if VERIFY_OSPF_READ
		printf("\t%d %d %d %d %d %c %c \n",
			dbe->b.free, dbe->b.age, dbe->b.entry,
			dbe->lsa, dbe->seqnum, dbe->cause_bgp, dbe->cause_ospf);
#endif
	}
}

void
ospf_db_print(ospf_state * state, FILE * log, tw_lp * lp)
{
	ospf_db_entry	*dbe;
	ospf_lsa	*lsa;

	int		 i;
	int		 x_id;

	fprintf(log, "LSA Database contains %d LSAs: db_last_ts %lf \n\n", 
		state->ar->g_ospf_nlsa, state->db_last_ts);

	fprintf(log, "\n\t%-15s %-10s %-10s # \n", 
		"Router Id", "index", "adv_r");
	for(i = 0; i < state->ar->nmachines; i++)
	{
		dbe = &state->db[i];
		x_id = i + state->ar->low;

		if(x_id == state->m->id)
		{
			fprintf(log, "\t%-15d %-15s \n", 
				x_id, "self");
		} else if(state->db[i].b.free)
		{
			fprintf(log, "\t%-15d %-15s \n", 
				x_id, "no entry");
		} else if(NULL != (lsa = getlsa(state->m, dbe->lsa, i)))
		{
			fprintf(log, "\t%-15d ", x_id);
			ospf_lsa_print(log, lsa);
		} else
		{
			fprintf(log, "\t%-15d %-15s", 
				x_id, "Fatal Error: NO LSA");
		}
	}

	fprintf(log, "\n\t%-15s %-10s %-10s # \n", 
		"Area Id", "index", "adv_r");
	for(; i < state->ar->nmachines + state->ar->as->nareas; i++)
	{
		dbe = &state->db[i];
		x_id = i - state->ar->nmachines + state->ar->as->low;

		if(x_id == state->ar->id)
		{
			fprintf(log, "\t%-15d %-15s \n", 
				x_id, "self");
		} else if(state->db[i].b.free)
		{
			fprintf(log, "\t%-15d %-15s \n", 
				x_id, "no entry");
		} else if(NULL != (lsa = getlsa(state->m, dbe->lsa, i)))
		{
			fprintf(log, "\t%-15d ", x_id);
			ospf_lsa_print(log, lsa);
		} else
		{
			fprintf(log, "\t%-15d %-15s \n", 
				x_id, "Fatal Error: NO LSA");
		}
	}

	fprintf(log, "\n\t%-15s %-10s %-10s # \n", 
		"AS Id", "index", "adv_r");
	for(; i < state->ar->g_ospf_nlsa; i++)
	{
		dbe = &state->db[i];
		x_id = i - state->ar->nmachines - state->ar->as->nareas;

		if(x_id == state->ar->as->id)
		{
			fprintf(log, "\t%-15d %-15s \n", 
				x_id, "self");
		} else if(state->db[i].b.free)
		{
			fprintf(log, "\t%-15d %-15s \n", 
				x_id, "no entry");
		} else if(NULL != (lsa = getlsa(state->m, dbe->lsa, i)))
		{
			fprintf(log, "\t%-15d ", x_id);
			ospf_lsa_print(log, lsa);
		} else
		{
			fprintf(log, "\t%-15d %-15s\n", 
				x_id, "Fatal Error: NO LSA");
		}
	}
}

/*
 * The new dbe has the correct age of the LSA, from the sender's side.
 * We must figure out what our DB entry age is.
 */
void
ospf_db_update(ospf_state * state, ospf_nbr * nbr, ospf_db_entry * new_dbe, tw_lp * lp)
{
#if VERIFY_OSPF_CONVERGENCE
	ospf_lsa	*lsa;
#endif
	ospf_db_entry	*curr;

	int		 last_lsa;
	int		 curr_age;

	curr = &state->db[new_dbe->b.entry];

	//curr_age = new_dbe->b.age;
	if(curr->b.free != 1)
		curr_age = ospf_lsa_age(state, curr, lp);
	else
		curr_age = OSPF_LSA_MAX_AGE;

	last_lsa = curr->lsa;

	curr->lsa = new_dbe->lsa;
	curr->b.free = 0;
	curr->seqnum = new_dbe->seqnum;
	ospf_db_route(state, curr, lp);
	ospf_aging_timer_set(state, curr, new_dbe->b.age, lp);

	if((new_dbe->b.age == OSPF_LSA_MAX_AGE &&
	    curr_age != OSPF_LSA_MAX_AGE) ||
	   (new_dbe->b.age != OSPF_LSA_MAX_AGE &&
	    curr_age == OSPF_LSA_MAX_AGE) ||
	   (new_dbe->lsa != last_lsa))
	{
		ospf_rt_build(state, lp->gid);

#if VERIFY_OSPF_CONVERGENCE
		lsa = getlsa(state->m, new_dbe->lsa, new_dbe->b.entry);
		lsa->count--;

		if(lsa->count == 0)
		{
			if(lsa->adv_r == 0)
			{
				g_route[0] = lp->gid;
				g_route[1] = nbr->id;
				gr1 = 1;
			} else
			{
				g_route1[0] = lp->gid;
				g_route1[1] = nbr->id;
				gr2 = 1;
			}

			printf("%ld: Convergence time for lsa %d: %f - %f = %f seconds\n",
				lp->gid, lsa->adv_r, 
				tw_now(lp), lsa->start,
				tw_now(lp) - lsa->start);
/*
			printf("%ld: link down at %f, time now %f \n",
				lp->gid, lsa->start, tw_now(lp));
*/
			state->c_time = tw_now(lp) - 
					lsa->start + 
					state->gstate->rt_interval;
		} 
/*
		else
			printf("%d: lsa %d remaining: %d \n",
				lp->gid, lsa->adv_r, lsa->count);
*/

#endif
	}
}

/*
 * Add this LSA to my lsa DB
 */
ospf_db_entry *
ospf_db_insert(ospf_state * state, int index, ospf_db_entry * curr, tw_lp * lp)
{
	ospf_db_route(state, curr, lp);

	curr->lsa = index;
	curr->b.age = 0;
	curr->b.free = 0;

	ospf_rt_build(state, lp->gid);

	return curr;
}

void
ospf_db_remove(ospf_state * state, int index, tw_lp * lp)
{
	ospf_db_entry	*curr;

	curr = &state->db[index];
	
	curr->b.free = 1;
	curr->b.age = OSPF_LSA_MAX_AGE;

	ospf_db_route(state, curr, lp);
}

unsigned int
ospf_db_init(ospf_state * state, tw_lp * lp)
{
	rn_area		*ar;

	ospf_db_entry	*dbe;
	ospf_lsa	*lsa;

	unsigned int 	next_timer;
	unsigned int	j;
	unsigned int	i;

	ar = rn_getarea(state->m);	
	state->db = (ospf_db_entry *) 
			calloc(sizeof(ospf_db_entry), state->ar->g_ospf_nlsa);

	if(!state->db)
		tw_error(TW_LOC, "Out of memory for local LSA database!");

#if VERIFY_DATABASE
	printf("%ld: INIT DB: nentries %d \n", lp->gid, state->ar->g_ospf_nlsa);
#endif

	/*
	 * Starting converged, setup LSA database
	 */
	state->db_last_ts = 0;
	next_timer = OSPF_LSA_MAX_AGE;
	j = 0;
	for(i = 0; i < state->ar->g_ospf_nlsa - g_rn_nas; i++)
	{
		dbe = &state->db[i];

		dbe->b.age = 0;
		dbe->b.free = 1;
		dbe->b.entry = i;
		dbe->seqnum = OSPF_MIN_LSA_SEQNUM + 1;

		// get the FIRST entry for the LSA
		if((NULL == (lsa = getlsa(state->m, dbe->lsa, i))) ||
		   lsa->adv_r == 0xffffffff)
		{
			if(i < state->ar->nmachines)
				continue;

			// May need to create the LSA from a prior run
			if(0 == g_ospf_rt_write && state->m->ft[i] != -1)
			{
				dbe->lsa = ospf_lsa_find(state, 
							&state->nbr[state->m->ft[i]],
							 i, lp);
				lsa = getlsa(state->m, dbe->lsa, i);
			}
		}

		// I think this is ok, since an LSA with no links is not
		// part of the database.. at that point it is simply removed
		if(lsa->links.size == 0)
			continue;

		dbe->b.free = 0;

#if VERIFY_DATABASE
		printf("\t%ld: lsa %d: age %d, adv_r %d\n", 
			lp->gid, i, dbe->b.age, lsa->adv_r);
#endif

		if(lsa->adv_r == lp->gid)
		{

			// If mine, variable so not all flood simulataneously.
			//dbe->b.age = tw_rand_integer(lp->gid, 0, OSPF_LSA_REFRESH_AGE - 1);

			if(i >= state->ar->nmachines && 
				i < state->ar->g_ospf_nlsa - g_rn_nas)
			{
				flood_summary(state, dbe, i, lp);
			}

			if(dbe->b.age >= OSPF_LSA_REFRESH_AGE)
				tw_error(TW_LOC, "LSA %d too old to start off with, "
					 "should be less than %d "
					 "(OSPF_LSA_REFRESH_AGE) \n", 
					 dbe->b.age,
					 OSPF_LSA_REFRESH_AGE);

			next_timer = min(OSPF_LSA_REFRESH_AGE - dbe->b.age, next_timer);

#if VERIFY_DATABASE
			printf("\tLSA %d, next timer: %d (mine)\n", i, next_timer);
#endif
		} else
		{
			if(i >= state->ar->nmachines && 
				i < state->ar->g_ospf_nlsa - g_rn_nas)
			{
				flood_summary(state, dbe, i, lp);
			}

			if(dbe->b.age >= OSPF_LSA_MAX_AGE)
				tw_error(TW_LOC, "LSA %d too old to start off with, "
					 "should be less than %d "
					 "(OSPF_LSA_MAX_AGE) \n", 
					 dbe->b.age,
					 OSPF_LSA_MAX_AGE);

			next_timer = min(OSPF_LSA_MAX_AGE - dbe->b.age, next_timer);

#if VERIFY_DATABASE
			printf("\tLSA %d, next timer: %d \n", i, next_timer);
#endif
		}
	}

	for(; i < state->ar->g_ospf_nlsa; i++)
	{
		state->db[i].b.entry = i;
		state->db[i].b.free = 1;
	}

	//ospf_db_print(state, stdout, lp);

	return next_timer;
}

void
flood_summary(ospf_state * state, ospf_db_entry * dbe, int entry, tw_lp * lp)
{
	ospf_lsa	*lsa;
	ospf_nbr 	*nbr;
	int	  	 i;

	if(g_rn_converge_ospf == 0)
		return;

	lsa = getlsa(state->m, dbe->lsa, entry);

	if(!lsa || lsa->links.size == 0)
		return;

	for (i = 0; i < state->n_interfaces; i++)
	{
		nbr = &state->nbr[i];

		if (nbr->state < ospf_nbr_exchange_st)
			continue;

		/* Don't send LSAs to neighbors in other ASes, or my Area, ever */
		if(state->ar == nbr->ar || state->ar->as != nbr->ar->as)
			continue;

		// Don't send LSA for nbr's area to nbr
		if(nbr->ar->id == 
			dbe->b.entry - state->ar->nmachines + state->ar->as->low)
			continue;

		// Don't send my area LSA to nbr, should already have it!
		if(state->ar->id == 
			dbe->b.entry - state->ar->nmachines + state->ar->as->low)
			continue;

#if VERIFY_LS
		printf("%ld: Starting flood LSA %d to %d: \n", 
			lp->gid, dbe->b.entry, nbr->id);
		printf("\tsn: %d ", dbe->seqnum);
		ospf_lsa_print(state->log, getlsa(state->m, dbe->lsa, dbe->b.entry));
#endif

		ospf_lsa_start_flood(nbr, dbe, lp);
	}
}
