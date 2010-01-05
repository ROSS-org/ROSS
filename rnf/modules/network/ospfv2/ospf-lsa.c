#include <ospf.h>

void
ospf_lsa_print(FILE * log, ospf_lsa * lsa)
{
#if OSPF_LOG && 0
	tw_memory	*b;
	ospf_lsa_link	*l;

/*
	fprintf(log, 
		"adv: %d id: %d nlinks: %d ", 
		lsa->adv_r, lsa->id, lsa->links.size);
*/

	fprintf(log, "%-10d %-10d %-5d ", 
		lsa->id, lsa->adv_r, lsa->links.size);

	for(b = lsa->links.head; b; b = b->next)
	{
		l = tw_memory_data(b);
		fprintf(log, "%d %d ", l->dst, l->metric);
	}

	fprintf(log, "\n");
#endif
}

int
ospf_lsa_equals(ospf_lsa * old, ospf_lsa * lsa)
{
	tw_memory	*b;
	tw_memory	*b_old;

	ospf_lsa_link	*old_link;
	ospf_lsa_link	*lsa_link;

	if(old == lsa)
		return TW_TRUE;

	if(old->adv_r != lsa->adv_r)
		return TW_FALSE;
	else if(old->links.size != lsa->links.size)
		return TW_FALSE;
	else if(old->length != lsa->length)
		return TW_FALSE;

	if(!old->links.size || !lsa->links.size)
		return TW_FALSE;

	for(b = lsa->links.head, b_old = old->links.head; b && b_old; 
						b = b->next, b_old = b_old->next)
	{
		old_link = tw_memory_data(b_old);
		lsa_link = tw_memory_data(b);

		if(old_link->dst != lsa_link->dst)
			return TW_FALSE;
		else if(old_link->metric != lsa_link->metric)
			return TW_FALSE;
	}

	return TW_TRUE;
}

/*
 * The id determines which type of LSA we wish to find.
 *
 * If looking for router_lsa : compare g_ospf_lsa LSAs to current links in my Area
 * If looking for summary_lsa: compare g_ospf_lsa LSAs to current links to that Area
 * If looking for AS ext lsa : compare g_ospf_lsa LSAs to current links to that AS
 *
 * Check link status.. do not count down links!
 */
int
ospf_lsa_find(ospf_state * state, ospf_nbr * n, int id, tw_lp * lp)
{
	tw_memory	*b;
	tw_memory	*b1;

	ospf_lsa_link	*ll;
	ospf_lsa	*lsa;
	ospf_nbr	*nbr;

	int		 index;
	int		 nlinks;
	int		 x_id;
	int	 	 i;

	nlinks = 0;
	nbr = NULL;

	if(id < state->ar->nmachines)
		x_id = state->ar->id;
	else if(id < state->ar->nmachines + state->ar->as->nareas)
	{
		x_id = id - state->ar->nmachines + state->ar->as->low;

		if(n->ar->id != x_id)
			nlinks = 1;
	} else
		x_id = id - state->ar->nmachines - state->ar->as->nareas;

	// Need to get nlinks FIRST, since # LSA links can be < # current
	// go by neighbor states, not topology, unless AS external LSA
	for(i = 0; i < state->n_interfaces; i++)
	{
		nbr = &state->nbr[i];

		if(nbr->state != ospf_nbr_full_st || 
		   nbr->istate != ospf_int_point_to_point_st)
			continue;

		// If router_lsa, only count links within my area
		if(id < state->ar->nmachines)
		{
			if(nbr->ar != state->ar)
				nlinks++;
		} else if(id >= state->ar->nmachines + state->ar->as->nareas)
		{
			// AS ext LSA, count link if to nbr AS == id's AS

			// If AS ext LSA, only thing I can do is count the
			// nbr which sent me the LSA
			nlinks = 1;
			break;

			if(nbr->ar->as->id == x_id)
				nlinks++;
		} else
		{
/*
			if(nbr->ar->id == x_id)
			{
				nlinks = 1;
				break;
			}
*/
			// Summary LSA, count link if to nbr Area == id's Area
			if(nbr->ar->id == x_id)
				nlinks++;
		}
	}

	b = state->ar->g_ospf_lsa[id]; 
	lsa = tw_memory_data(b);
	for(index = 0; NULL != lsa; index++)
	{
		if(lsa->links.size == nlinks && lsa->adv_r == lp->gid)
		{
			if(nlinks == 0)
			{
				i = nlinks;
			} else if(id >= state->ar->nmachines + state->ar->as->nareas)
			{
				ll = tw_memory_data(lsa->links.head);

				if(n->id == ll->dst)
					i = nlinks;
			} else
			{
				// foreach LSA link: if is up, good, otherwise, not the LSA!
				b1 = lsa->links.head;
				for(i = 0; i < lsa->links.size; i++, b1 = b1->next)
				{
					ll = tw_memory_data(b1);
					nbr = ospf_getnbr(state, ll->dst);
	
					if(nbr->state != ospf_nbr_full_st)
						break;
				}
			}

			if(i == nlinks)
				return index;
		}

		if(lsa->next == NULL)
			break;

		b = lsa->next;
		lsa = tw_memory_data(b);
	}

	if(lsa->next != NULL)
		tw_error(TW_LOC, "Not at end of LSA list!");

	index++;
	lsa->next = ospf_lsa_allocate(lp);
	lsa = tw_memory_data(lsa->next);

	lsa->id = id;
	lsa->adv_r = lp->gid;

	if(id < state->ar->nmachines)
	{
		lsa->type = ospf_lsa_router;
		ospf_lsa_link_allocate(state, lsa, lp);

		return index;
	} else if(id < state->ar->nmachines + state->ar->as->nareas)
	{
		lsa->type = ospf_lsa_summary3;
		ospf_lsa_link_allocate(state, lsa, lp);
	
		// Only add nbr to LSA if it does NOT match x_id, because
		// that is the case where we are originating an LSA for
		// an AS/area more than 1 hop away
		if(n->ar->id == x_id)
			return index;
	} else
		lsa->type = ospf_lsa_as_ext;


	b = tw_memory_alloc(lp, g_ospf_ll_fd);
	tw_memoryq_push(&lsa->links, b);

	ll = tw_memory_data(b);
	ll->dst = n->id;
	ll->metric = rn_getlink(state->m, n->id)->cost;

/*
	printf("%ld: Creating LSA: %d, advr %d, %lf \n",
		lp->gid, lsa->id, lsa->adv_r, lp->cur_event->recv_ts);
*/

	return index;
}

unsigned int
ospf_lsa_age(ospf_state * state, ospf_db_entry * dbe, tw_lp * lp)
{
	int	x;

	if(dbe->b.free || dbe->b.age == OSPF_LSA_MAX_AGE)
		return OSPF_LSA_MAX_AGE;

	if((tw_now(lp) - state->db_last_ts) < 4)
		return dbe->b.age;

	x = (floor((tw_now(lp) - state->db_last_ts) / 4) * 4) + dbe->b.age;

	return (unsigned int) x;
}

ospf_lsa *
getlsa(rn_machine * m, int index, int id)
{
	tw_memory	*b;
	ospf_lsa	*lsa;
	int i;

	if(NULL == (b = rn_getarea(m)->g_ospf_lsa[id]))
		return NULL;

	lsa = tw_memory_data(b);

	for(i = 0; i < index; i++)
	{
		b = lsa->next;

		if(!b)
			return NULL;

		lsa = tw_memory_data(b);
	}

	if(lsa == NULL)
		tw_error(TW_LOC, "Could not locate the request lsa!!");

	return lsa;
}

/*
 * Helper function for creating LSAs...
 */
char
lsa_create(ospf_state * state, int entry, tw_lp * lp)
{
	tw_memory	*b;

	ospf_lsa	*lsa;
	ospf_lsa_link	*l;
	ospf_nbr	*nbr;

	int		 nlinks;
	int		 i;
	int		 x_id;
	char		 index;

	nlinks = 0;

	if(entry < state->ar->nmachines)
		x_id = state->ar->id;
	else if(entry >= state->ar->nmachines + state->ar->as->nareas)
		x_id = entry - state->ar->nmachines - state->ar->as->nareas;
	else
		x_id = entry - state->ar->nmachines + state->ar->as->low;

	// Need to get nlinks FIRST, since # LSA links can be < # current
	// go by neighbor states, not topology, unless AS external LSA
	for(i = 0; i < state->n_interfaces; i++)
	{
		nbr = &state->nbr[i];

		if(nbr->state != ospf_nbr_full_st || 
		   nbr->istate != ospf_int_point_to_point_st)
			continue;

		// If router_lsa, only count links within my area
		if(entry < state->ar->nmachines)
		{
			if(nbr->ar != state->ar)
				nlinks++;
		} else if(entry > state->ar->nmachines + state->ar->as->nareas)
		{
			// AS ext LSA, count link if to nbr AS == id's AS
			if(nbr->ar->as->id == x_id)
				nlinks++;
		} else
		{
			// Summary LSA, count link if to nbr Area == id's Area
			if(state->ar->id == x_id)
				nlinks++;
		}
	}

	// Get the head of the LSA array for this index
	b = rn_getarea(state->m)->g_ospf_lsa[entry];
	lsa = tw_memory_data(b);

	for(index = 0; NULL != lsa; index++)
	{
		if(lsa->links.size == nlinks)
		{
			b = lsa->links.head;
			for(i = 0; i < lsa->links.size; i++)
			{
				l = tw_memory_data(b);
				nbr = ospf_getnbr(state, l->dst);

				if(!nbr || (nbr->state != ospf_nbr_full_st))
					break;

				b = b->next;
			}

			if(nlinks == i && lsa->adv_r == lp->gid)
			{
#if VERIFY_LS
				printf("%ld OSPF: Found LSA for %d in graph! \n", 
					lp->gid, lsa->id);
#endif
				return index;
			}
		}

		if(lsa->next == NULL)
		{
			index++;
			break;
		}

		b = lsa->next;
		lsa = tw_memory_data(b);
	}

	lsa->next = ospf_lsa_allocate(lp);
	lsa = tw_memory_data(lsa->next);

	lsa->id = entry;
	lsa->adv_r = lp->gid;

	if(x_id < state->ar->nmachines)
		lsa->type = ospf_lsa_router;
	else if(x_id < state->ar->nmachines + state->ar->as->nareas)
		lsa->type = ospf_lsa_summary3;
	else
		lsa->type = ospf_lsa_as_ext;

	ospf_lsa_link_allocate(state, lsa, lp);

	return index;
}

tw_memory	*
ospf_lsa_allocate(tw_lp * lp)
{
	tw_memory	*b;

	ospf_lsa	*lsa;

	//tw_printf(TW_LOC, "%d: Allocating a new LSA! \n", lp->gid);

	b = tw_memory_alloc(lp, g_ospf_lsa_fd);

	if(!b) 
		tw_error(TW_LOC, "Could not dynamically allocate any new LSAs! \n");

	lsa = tw_memory_data(b);

	return	b;
}

void
ospf_lsa_link_allocate(ospf_state * state, ospf_lsa * lsa, tw_lp * lp)
{
	tw_memory	*b;

	ospf_lsa_link	*l;
	ospf_nbr	*nbr;

	int		 add;
	int		 x_id;
	int		 i;

#if VERIFY_OSPF_CONVERGENCE
	lsa->count = state->ar->nmachines;
#endif

	if(lsa->id < state->ar->nmachines)
		x_id = lsa->id;
	else if(lsa->id > state->ar->nmachines + state->ar->as->nareas)
		x_id = lsa->id - state->ar->nmachines - state->ar->as->nareas;
	else
		x_id = lsa->id - state->ar->nmachines + state->ar->as->low;

	// go by neighbor states, not topology, unless AS external LSA
	for(i = 0; i < state->n_interfaces; i++)
	{
		nbr = &state->nbr[i];

		if(nbr->state != ospf_nbr_full_st || 
		   nbr->istate != ospf_int_point_to_point_st)
			continue;

		add = 0;

		// If router_lsa, only count links within my area
		if(lsa->id < state->ar->nmachines)
		{
			// router LSA, count if nbr area == my area
			if(nbr->ar == state->ar)
			{
				b = tw_memory_alloc(lp, g_ospf_ll_fd);
				tw_memoryq_push(&lsa->links, b);

				l = tw_memory_data(b);
				l->dst = nbr->id;

				l->metric = state->m->link[i].cost;
			}
		} else
		{
			// summary LSA, count if nbr area == LSA area
			if(nbr->ar->id == x_id)
			{
				b = tw_memory_alloc(lp, g_ospf_ll_fd);
				tw_memoryq_push(&lsa->links, b);

				l = tw_memory_data(b);
				l->dst = nbr->id;

				l->metric = state->m->link[i].cost;
			}
		}
	}

	lsa->length = OSPF_LSA_UPDATE_HEADER + 
			(lsa->links.size * OSPF_LSA_LINK_LENGTH);
}

/*
 * No need to delete, and then recreate an LSA each time.  Simply refresh it.
 * This may create havoc when we go to retransmit this LSA.. although, why we
 * we ever retransmit an old LSA for ourselves?  We would always retransmit
 * the newest we have, ie, the old one would not show up in the DB summary list
 * anymore, and that is what we use to build the retransmit list.
 */
void
ospf_lsa_refresh(ospf_state * state, ospf_db_entry * dbe, tw_lp * lp)
{
	int		wrapping = 0;

	ospf_db_entry	 temp_dbe;
	ospf_lsa	*lsa;

	lsa = getlsa(state->m, dbe->lsa, dbe->b.entry);

	if (state->lsa_seqnum == OSPF_MAX_LSA_SEQNUM)
	{
		if(state->lsa_wrapping)
		{
			dbe->b.free = 1;
		} else
		{
			dbe->seqnum = OSPF_MAX_LSA_SEQNUM;
			wrapping = 1;

			// Prematurely age the LSA
			ospf_aging_timer_set(state, &state->db[lsa->id],
							OSPF_LSA_MAX_AGE, lp);

			/* Make a copy for flooding with the correct AGE */
			temp_dbe.b = dbe->b;
			temp_dbe.lsa = dbe->lsa;

			ospf_lsa_flood(state, &temp_dbe, lp->gid, lp);
			state->lsa_wrapping = 1;
		}

		return;
	}

	//printf("%d: old LSA %d seqnum: %d \n", lp->gid, lp->gid, dbe->seqnum);
	dbe->seqnum++;
	//printf("%d: new LSA %d seqnum: %d \n", lp->gid, lp->gid, dbe->seqnum);
	ospf_lsa_flood(state, dbe, lp->gid, lp);
}

/*
 * Create a new LSA because one of our links went up/down.  If this particular
 * configuration of interfaces has already been created in the global LSA list,
 * use that, otherwise, make a new LSA and link it in.
 */
void
ospf_lsa_create(ospf_state * state, ospf_nbr * nbr, int entry, tw_lp * lp)
{
	rn_link *l = NULL;

	ospf_lsa	*old_lsa;
	ospf_lsa	*lsa;
	ospf_db_entry	*dbe;
	int		 wrapping = 0;
	int		 index;

#if VERIFY_OSPF_CONVERGENCE
	int		 i;
#endif

	index = ospf_lsa_find(state, nbr, entry, lp);

	dbe = &state->db[entry];
	lsa = getlsa(state->m, index, entry);
	old_lsa = getlsa(state->m, dbe->lsa, entry);

	// If LSA links drops to zero, flush LSA from network
	if(lsa->links.size == 0)
	{
		if(dbe->b.free || ospf_lsa_equals(old_lsa, lsa) == TW_TRUE)
			return;

#if VERIFY_LS
		printf("%ld: flushing LSA %d: no links \n",
			lp->gid, lsa->id);
#endif

		dbe->lsa = index;
		ospf_db_remove(state, entry, lp);

		ospf_lsa_flood(state, dbe, nbr->id, lp); 

		return;
	}

	// Highest adv_r creates LSAs
	if(old_lsa->adv_r != 0xffffffff && 
	   old_lsa->adv_r > lsa->adv_r &&
	   rn_getarea(rn_getmachine(old_lsa->adv_r)) != rn_getarea(rn_getmachine(lsa->adv_r)))
		return;

	if(state->lsa_seqnum == OSPF_MAX_LSA_SEQNUM)
	{
		if(state->lsa_wrapping)
		{
			tw_error(TW_LOC, "Not sure what to do here now!");
			return;
		} else
		{
			dbe->seqnum = OSPF_MAX_LSA_SEQNUM;
			wrapping = 1;
		}
	} else
	{
		if(dbe->b.entry < state->ar->nmachines)
			dbe->seqnum = ++state->lsa_seqnum;
	}

	dbe->cause_bgp = 0;
	//dbe->cause_ospf = 1;
	//state->stats->s_cause_bgp++;

	if(dbe->b.free || TW_FALSE == ospf_lsa_equals(old_lsa,lsa) || 
	   state->lsa_seqnum > OSPF_MAX_LSA_SEQNUM)
	{
		if(!lsa->links.size)
			tw_error(TW_LOC, "Insert LSA with no links into DB!");

		dbe = ospf_db_insert(state, index, dbe, lp);
		l = rn_getlink(rn_getmachine(lp->gid), nbr->id);

#if VERIFY_OSPF_CONVERGENCE
		lsa->count = 0;
		for(i = 0; i < state->ar->nmachines; i++)
		{
			if(rn_route(state->m, i) == -1)
				continue;

			int int_id = state->nbr[rn_route(state->m, i)].id;

			if(int_id != -1 &&
				int_id != l->wire->id &&
				i != lp->gid)
			{
				lsa->count++;
			}
		}

		lsa->start = l->last_status;

		printf("%ld: lsa start time: %f hop count %d \n", 
			lp->gid, lsa->start, lsa->count);
#endif
	}

	if (wrapping)
	{
		tw_error(TW_LOC, "I did not expect to be here!");

		/*
		 * Prematurely age the LSA -- have to reset the aging timer
		 * so that this LSA can be handled at the next LSA timer
		 * interval
		 */
		ospf_aging_timer_set(state, &state->db[lsa->id], 
							OSPF_LSA_MAX_AGE, lp);
		ospf_lsa_flood(state, dbe, lp->gid, lp);
		state->lsa_wrapping = 1;
	} else
	{
		ospf_lsa_flood(state, dbe, nbr->id, lp); 
	}
}

void
ospf_lsa_start_flood(ospf_nbr * nbr, ospf_db_entry * dbe, tw_lp * lp)
{
	tw_event	*e;
	tw_memory	*send;

	ospf_db_entry	*out_dbe;
	ospf_lsa	*lsa;

	if(ospf_lsa_age(nbr->router, dbe, lp) + 1 > OSPF_LSA_MAX_AGE)
		ospf_aging_timer_set(nbr->router, dbe, OSPF_LSA_MAX_AGE, lp);

	/*
	 * If the flood timer has already gone off, just send the LSA in a
	 * single pkt.
	 */
	if(nbr->next_flood > tw_now(lp))
	{
		send = tw_memory_alloc(lp, g_ospf_fd);

		out_dbe = tw_memory_data(send);
		out_dbe->lsa = dbe->lsa;
		out_dbe->b.entry = dbe->b.entry;
		out_dbe->seqnum = dbe->seqnum;
		out_dbe->b.age = ospf_lsa_age(nbr->router, dbe, lp) + 1;
		out_dbe->b.free = 0;
		out_dbe->cause_bgp = dbe->cause_bgp;
		out_dbe->cause_ospf = dbe->cause_ospf;

		lsa = getlsa(nbr->router->m, out_dbe->lsa, out_dbe->b.entry);

		e = ospf_event_new(nbr->router, 
				nbr->id, nbr->next_flood - tw_now(lp), lp);
		ospf_event_send(nbr->router, e, OSPF_LS_UPDATE, lp, 
				lsa->length, send, nbr->router->ar->id);

#if VERIFY_LS
		printf("\tflood adding LSA %d to %d: mine (%d %d) send (%d %d) at %lf\n",
				dbe->b.entry, nbr->id, 
				dbe->b.age, dbe->seqnum, 
				out_dbe->b.age, out_dbe->seqnum, e->recv_ts);
#endif
	} else
	{
		/*
		 * To start flood, and no flood has already started, set the flood
		 * timer and place all requests onto the retranmit list.  When
		 * the flood timer goes off, that is when you will send the pkts.
		 */
		nbr->next_flood = nbr->router->gstate->flood_timer + tw_now(lp);
	
		lsa = getlsa(nbr->router->m, dbe->lsa, dbe->b.entry);

		if(!lsa)
		{
			printf("%lld: aborting flood of LSA: %d \n",
				lp->gid, dbe->b.entry);
			return;
		}

		send = tw_memory_alloc(lp, g_ospf_fd);
		out_dbe = tw_memory_data(send);
	
		out_dbe->b.age = dbe->b.age + 1;
		out_dbe->b.entry = dbe->b.entry;
		out_dbe->b.free = 0;
		out_dbe->lsa = dbe->lsa;
		out_dbe->seqnum = dbe->seqnum;
		out_dbe->cause_bgp = dbe->cause_bgp;
		out_dbe->cause_ospf = dbe->cause_ospf;
	
		e = ospf_event_new(nbr->router, nbr->id, 
					nbr->router->gstate->flood_timer, lp);
	
		nbr->flood = ospf_event_send(nbr->router, e, OSPF_LS_UPDATE, lp, 
					lsa->length, send, nbr->router->ar->id);
	
#if VERIFY_LS
		printf("\t\t\tflood LSA %d to %d: send (%d %d) at %lf\n",
				dbe->b.entry, nbr->id, 
				out_dbe->b.age, out_dbe->seqnum, e->recv_ts);
#endif
	}
}

void
ospf_lsa_flood(ospf_state * state, ospf_db_entry * dbe, int from, tw_lp * lp)
{
	ospf_nbr *nbr;
	int	  i;

	for (i = 0; i < state->n_interfaces; i++)
	{
		nbr = &state->nbr[i];

		if (nbr->state < ospf_nbr_exchange_st)
			continue;

		/* Don't send it to myself, or the neighbor who sent it to me */
		if ((from != -1 || from != lp->gid) && nbr->id == from)
			continue;

		/* Don't send router LSAs to neighbors in other areas */
		if(state->ar != nbr->ar && dbe->b.entry < state->ar->nmachines)
			continue;

		/* Don't send LSAs to neighbors in other ASes, ever */
		if(state->ar->as != nbr->ar->as)
			continue;

		// Don't send LSA for nbr's area to nbr
		if(nbr->ar->id == dbe->b.entry - state->ar->nmachines)
			continue;

		// Don't send my area LSA to nbr, should already have it!
		if(state->ar->id == dbe->b.entry - state->ar->nmachines)
			continue;

		if(nbr->state == ospf_nbr_exchange_st ||
		   nbr->state == ospf_nbr_loading_st)
		{
/*
Here we are not supposed to start flooding if the LSA in the request list
is newer than this one.. 

 This case shouldn't really come up, where the outstanding LSA is newer
 than the one we want to flood.. 

			if(dbe->state.b.updated = 
				continue;
*/

			tw_error(TW_LOC, "Unhandled ospf flooding case!");
		}

#if VERIFY_LS
		printf("\t\tstarting flood LSA %d to %d: \n", dbe->b.entry, nbr->id);
		printf("\t\t\tsn: %d ", dbe->seqnum);
		ospf_lsa_print(state->log, getlsa(state->m, dbe->lsa, dbe->b.entry));
#endif

		ospf_lsa_start_flood(nbr, dbe, lp);
	}
}

// Returns 0 if new identical to old
// Returns -1 if new older than old
int
ospf_lsa_isnewer(ospf_state * state, ospf_db_entry * old, 
					ospf_db_entry * new, tw_lp * lp)
{
	unsigned int old_age;

	old_age = ospf_lsa_age(state, old, lp);

	if(old->b.free || old_age == OSPF_LSA_MAX_AGE)
	{
#if VERIFY_LS
	printf("\tis_newer: old NONE new (%d %d)\n", new->b.age, new->seqnum);
#endif

		return 1;
	}


#if VERIFY_LS
	printf("\tis_newer: old (%d %d) new (%d %d)\n",
			old_age, old->seqnum, new->b.age, new->seqnum);
#endif

	/*
	 * Make sure we are in the right packet sequence
	 */
	if(new->seqnum > old->seqnum)
	{
		return 1;
	} else if (old->seqnum > new->seqnum)
	{
		return -1;
	} else
	{
		// I am going into my old age writing this function..
		if(new->b.age == OSPF_LSA_MAX_AGE && old_age != OSPF_LSA_MAX_AGE)
		{
			return 1;
		} else if(old_age == OSPF_LSA_MAX_AGE && 
			new->b.age != OSPF_LSA_MAX_AGE)
		{
			return -1;
		} else
		{
			if(abs(old_age - new->b.age) > OSPF_LSA_MAX_AGE_DIFF)
			{
				if (new->b.age < old_age)
					return 1;
				else
					return -1;
			}
		}
	}

	return 0;
}

void
ospf_lsa_flush(ospf_state * state, ospf_nbr * nbr, ospf_db_entry * r_dbe, tw_lp * lp)
{
	tw_memory	*send;

	ospf_lsa	*r_lsa;
	ospf_db_entry	*dbe;
	ospf_db_entry	*out_dbe;

	int		 originator;
	int		 x_id = -1;
	int		 i = -1;

	dbe = &state->db[r_dbe->b.entry];
	r_lsa = getlsa(state->m, r_dbe->lsa, r_dbe->b.entry);

	originator = 0;
	if(r_lsa->adv_r == lp->gid)
	{
		if(r_lsa->type == ospf_lsa_router)
		{
			originator = 1;
		} else if(r_lsa->type == ospf_lsa_summary3)
		{
			x_id = r_dbe->b.entry - state->ar->nmachines;
			for(i = 0; i < state->n_interfaces; i++)
			{
				if(state->nbr[i].ar->id == x_id &&
				   state->nbr[i].state == ospf_nbr_full_st)
				{
					originator = i;
					break;
				}
			}
		}

		// if AS ext, do nothing, we do not originate them ever,
		// the BGP layer/model does
	}

	if(-1 == originator)
	{
		if(dbe->b.free)
			return;

#if VERIFY_LS
		printf("\tflushing LSA %d \n", r_dbe->b.entry);
#endif

		r_dbe->b.age--;
		ospf_db_remove(state, dbe->b.entry, lp);
		ospf_lsa_flood(state, r_dbe, nbr->id, lp);
	
		send = tw_memory_alloc(lp, g_ospf_fd);
		out_dbe = tw_memory_data(send);
	
		out_dbe->lsa = r_dbe->lsa;
		out_dbe->b.age = OSPF_LSA_MAX_AGE;
		out_dbe->b.entry = r_dbe->b.entry;
	
		ospf_ack_direct(nbr, send, r_lsa->length, lp);
	} else
		ospf_lsa_create(state, &state->nbr[originator], r_dbe->b.entry, lp);
}
