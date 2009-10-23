#include <bgp.h>
#include <ospf.h>

int  bgp_lsa_find(bgp_state *, ospf_state * state, bgp_route *, int id, tw_lp * lp);
void bgp_lsa_link_allocate(bgp_state *, ospf_state *, ospf_lsa *, int, tw_lp *);

void
bgp_ospf_init(bgp_state * bs, bgp_route * r, tw_lp * lp)
{
	rn_lp_state	*rn_state;
	rn_stream	*s;
	rn_area		*ar;

	ospf_state	*state;

	ospf_db_entry	*dbe;

	int		 entry;

	ar = rn_getarea(rn_getmachine(lp->id));
	entry = ar->nmachines + ar->as->nareas + r->dst;

	rn_state = lp->cur_state;
	s = rn_getstream_byport(rn_state, 23);

	if(!s || s->port != 23)
		return;

	state = s->layers[0].lp.cur_state;
	dbe = &state->db[entry];

	/*** CANNOT RE-BUILD ROUTING TABLES !!! ***/

	if(NULL == r)
	{
		// remove from database
		ospf_db_remove(state, entry, lp);
	} else if(1 == dbe->b.free)
	{
		// insert into DB
		dbe->b.age = 0;
		dbe->b.free = 0;
		dbe->b.entry = entry;
		dbe->seqnum = OSPF_MIN_LSA_SEQNUM;
		dbe->lsa = bgp_lsa_find(bs, state, r, entry, lp);

		ospf_db_route(state, dbe, lp);
	} else
	{
		// update database
		dbe = &state->db[entry];

		dbe->b.age = 0;
		dbe->b.free = 0;
		dbe->b.entry = entry;
		dbe->seqnum = OSPF_MIN_LSA_SEQNUM;
		dbe->lsa = bgp_lsa_find(bs, state, r, entry, lp);

		ospf_db_route(state, dbe, lp);
	}

#if VERIFY_BGP
	fprintf(bs->log, "\tinitialized OSPF lsa: AS %d, LSA %d \n", 
		r->dst, entry);
#endif
}

// LSA for AS must have same number of links as FULL neighbors?
void
bgp_ospf_update(bgp_state * bs, rn_as * dest_as, bgp_route * r, int add, tw_lp * lp)
{
#ifdef HAVE_OSPF_H
	tw_lp		*cur_lp;

	rn_lp_state	*rn_state;
	rn_area		*ar;
	rn_stream	*s;

	ospf_state	*state;

	ospf_db_entry	*dbe;
	ospf_lsa	*old_lsa;
	ospf_nbr	 nbr;

	int		 entry;
	int		 cur_stream;

	ar = rn_getarea(rn_getmachine(lp->id));
	entry = ar->nmachines + ar->as->nareas + dest_as->id;

	// Get the OSPF router state
	rn_state = lp->cur_state;
	cur_stream = rn_state->cur_stream;
	cur_lp = rn_state->cur_lp;

	s = rn_getstream_byport(rn_state, 23);

	if(!s || s->port != 23)
	{
		printf("%ld: Did not send update (%d) to OSPF! \n", lp->id, add);
		rn_state->cur_stream = cur_stream;

		return;
	}

	state = s->layers[0].lp.cur_state;
	rn_state->cur_lp = &s->layers[0].lp;

	// Get and fill in the new LSA
	dbe = &state->db[entry];

/*
	You *MUST* restore the cur_stream and cur_lp pointers
	BEFORE return-ing from this function!!!!!!!!!!!!!!!!!

	If you don't, future BGP pkts will go to OSPF on the
	other side of the connection!
*/

	if(add && dbe->b.free == 0)
	{
		// If multiple possible originators,
		// rtr w/ > id is originator.  
		// Eliminates oscillation / duplicate effort
		old_lsa = getlsa(state->m, dbe->lsa, entry);

		if(old_lsa && lp->id < old_lsa->adv_r)
		{
			rn_state->cur_lp = cur_lp;
			rn_state->cur_stream = cur_stream;

			return;
		}

		// No need to change LSA is route src has not changed!
		if(old_lsa)
		{
			ospf_lsa_link *l = tw_memory_data(old_lsa->links.head);

			if(l->dst == r->src && old_lsa->adv_r == lp->id)
			{
				rn_state->cur_lp = cur_lp;
				rn_state->cur_stream = cur_stream;

				return;
			}
		}
	}

	/*
	 * Construct the DB entry we need, and have OSPF process it.
	 * If we wanted to connect BGP to OSPF on different routers,
	 * we would simply need to send the OSPF node an UPDATE message
	 */
	//dbe->b.entry = entry;
	dbe->cause_bgp = 1;
	dbe->lsa = bgp_lsa_find(bs, state, r, entry, lp);

	nbr.id = r->src;
	nbr.state = ospf_nbr_full_st;

	if(add)
	{
		dbe->b.age = 0;
		dbe->b.free = 0;
		dbe->seqnum = state->lsa_seqnum;
		ospf_lsa_create(state, &nbr, entry, lp);
	} else
	{
		if(dbe->b.free)
		{
			rn_state->cur_stream = cur_stream;
			rn_state->cur_lp = cur_lp;
			return;
		}

		dbe->b.age = OSPF_LSA_MAX_AGE;
		ospf_lsa_create(state, &nbr, entry, lp);
		dbe->b.free = 1;
	}

	state->stats->s_cause_bgp++;

#if VERIFY_BGP
	if(add)
		fprintf(bs->log, "\tinstalled lsa into OSPF! AS %d, LSA %d \n", 
			dest_as->id, entry);
	else
		fprintf(bs->log, "removed lsa from OSPF! \n");
#endif

	rn_state->cur_stream = cur_stream;
	rn_state->cur_lp = cur_lp;
#endif
}

/*
 * The id determines which type of LSA we wish to find.
 *
 * If looking for AS ext lsa : compare g_ospf_lsa LSAs to current links to that AS
 */
int
bgp_lsa_find(bgp_state * bstate, ospf_state * state, bgp_route * r, int id, tw_lp * lp)
{
	tw_memory	*b;

	ospf_lsa_link	*ll;
	ospf_lsa	*lsa;

	int		 index;
	int		 nlinks;
	int		 x_id;

	nlinks = 0;
	x_id = id - state->ar->nmachines - state->ar->as->nareas;

	b = state->ar->g_ospf_lsa[id];
	lsa = tw_memory_data(b);
	for(index = 0; NULL != lsa; index++)
	{
		if(lsa->links.size == 1 && lsa->adv_r == lp->id)
		{
			// foreach LSA link: if is up, good, otherwise, not the LSA!
			b = lsa->links.head;
			ll = tw_memory_data(b);

			if(ll->dst == r->src)
				return index;
		}

		if(lsa->next == NULL)
		{
			index++;
			break;
		}

		b = lsa->next;
		lsa = tw_memory_data(b);
	}

	if(index > 1)
	{
	b = state->ar->g_ospf_lsa[id];
	lsa = tw_memory_data(b);
	for(index = 0; NULL != lsa; index++)
	{
		if(lsa->links.size == 1 && lsa->adv_r == lp->id)
		{
			// foreach LSA link: if is up, good, otherwise, not the LSA!
			b = lsa->links.head;
			ll = tw_memory_data(b);

			if(ll->dst == r->src)
				return index;
		}

		if(lsa->next == NULL)
		{
			index++;
			break;
		}

		b = lsa->next;
		lsa = tw_memory_data(b);
	}
	}

	if(lsa->next != NULL)
		tw_error(TW_LOC, "Not at end of list!");

	lsa->next = ospf_lsa_allocate(lp);
	lsa = tw_memory_data(lsa->next);

	lsa->id = id;
	lsa->adv_r = lp->id;
	lsa->type = ospf_lsa_as_ext;

	b = tw_memory_alloc(lp, g_ospf_ll_fd);
	tw_memoryq_push(&lsa->links, b);

	ll = tw_memory_data(b);
	ll->dst = r->src;

	if(rn_getas(rn_getmachine(r->src)) == bstate->as)
		ll->metric = state->m->link[rn_route(state->m, r->src)].cost;
	else
		ll->metric = rn_getlink(state->m, r->src)->cost;

	//printf("%ld: created new LSA: %d, ll->dst %d \n", lp->id, lsa->id, ll->dst);
	//bgp_lsa_link_allocate(bstate, state, r, lsa, x_id, lp);

	return index;
}

void
bgp_lsa_link_allocate(bgp_state * bstate, ospf_state * state, 
			ospf_lsa * lsa, int x_id, tw_lp * lp)
{
	tw_memory	*b;

	rn_as		*d_as;

	ospf_lsa_link	*l;
	bgp_nbr		*nbr;

	int		 i;

	// go by neighbor states, not topology, unless AS external LSA
	for(i = 0; i < bstate->n_interfaces; i++)
	{
		nbr = &bstate->nbr[i];
		d_as = rn_getas(rn_getmachine(nbr->id));

		if(d_as->id == x_id)
		{
			b = tw_memory_alloc(lp, g_ospf_ll_fd);
			tw_memoryq_push(&lsa->links, b);

			l = tw_memory_data(b);
			l->dst = nbr->id;
			l->metric = state->m->link[i].cost;
		}
	}

	lsa->length = OSPF_LSA_UPDATE_HEADER + 
			(lsa->links.size * OSPF_LSA_LINK_LENGTH);
}

void
bgp_ospf_flush(bgp_state * bs, rn_as * dest_as, bgp_route * r, int add, tw_lp * lp)
{
	tw_error(TW_LOC, "argh");
}
