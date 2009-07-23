#include <ospf.h>

int
noNeighborsInDataExchange(ospf_state * state)
{
	int             i;

	for (i = 0; i < state->n_interfaces; i++)
	{
		if((state->nbr[i].state == ospf_nbr_loading_st) ||
		   (state->nbr[i].state == ospf_nbr_exchange_st))
		{
			return 0;
		}
	}

	return 1;
}

/**
 * This function processes a received LSA from one of our neighbors.
 * If the lsa is newer than the one that our DB contains, we need to
 * flood it to our neighbors.
 */
int
process_lsa_recv(ospf_state *state, ospf_nbr * nbr, ospf_db_entry *r_dbe, tw_lp *lp)
{
	tw_event	*e;
	tw_memory	*send;
	tw_memory	*b;

	ospf_db_entry	*dbe;
	ospf_db_entry	*out_dbe;

	ospf_lsa_link	*l;
	ospf_lsa	*lsa;
	ospf_lsa	*r_lsa;

	int	is_newer;

	dbe = &state->db[r_dbe->b.entry];

	// incoming LSAs have the correct AGE!
	if(r_dbe->b.age-1 == OSPF_LSA_MAX_AGE && noNeighborsInDataExchange(state))
	{
		if(TW_FALSE == ospf_list_contains(nbr->requests, dbe->b.entry))
			ospf_lsa_flush(state, nbr, r_dbe, lp);

		return 1;
	}

	if(dbe->b.entry == 0)
		lsa = getlsa(state->m, dbe->lsa, dbe->b.entry);

	r_lsa = getlsa(state->m, r_dbe->lsa, r_dbe->b.entry);

	is_newer = ospf_lsa_isnewer(state, dbe, r_dbe, lp);

	// If I already have the lsa from a higher id router,
	// then ACK, but do nothing.. the other router must
	// flush the LSA if the network becomes unavailable
	if(dbe->b.entry == 0 && 
	   r_dbe->b.entry >= state->ar->nmachines)
	{
		b = r_lsa->links.head;
		l = tw_memory_data(b);

		if(nbr->id < l->dst)
		{
			send = tw_memory_alloc(lp, g_ospf_fd);
			out_dbe = tw_memory_data(send);
			out_dbe->lsa = r_dbe->lsa;
			out_dbe->b.age = r_dbe->b.age;
			out_dbe->b.entry = r_dbe->b.entry;

			ospf_ack_direct(nbr, send, r_lsa->length, lp);

			return 1;
		}
	}

	// May receive identical LSA from different sources,
	// source with higher router id is newer
	if(dbe->b.entry == 0 && 
	   0 == is_newer && 
	   r_dbe->b.entry >= state->ar->nmachines)
	{
		b = r_lsa->links.head;
		l = tw_memory_data(b);

		if(nbr->id > l->dst)
			is_newer = 1;

#if VERIFY_LS
		printf("\trecv identical LSA %d from diff source!: old %d new %d\n", 
			r_lsa->id, l->dst, nbr->id);
#endif
	}

#if 0
	if(r_dbe->b.entry == 0)
	{
		printf("%ld: recvd LSA %d, is_newer == %d \n", 
			lp->id, r_dbe->b.entry, is_newer);
		printf("%ld: old age %d, new age %d \n", lp->id, dbe->b.age,
					r_dbe->b.age);
		printf("%ld: old seqnum %d, new seqnum %d \n", lp->id,
					dbe->seqnum, r_dbe->seqnum);
	}
#endif

	if(1 == is_newer)
	{
		if(r_lsa->adv_r == lp->id)
		{
			if(r_dbe->b.entry < state->ar->nmachines)
				state->lsa_seqnum = r_dbe->seqnum + 1;
			else
				dbe->seqnum = r_dbe->seqnum;

			// Self originated lsa
			ospf_lsa_create(state, nbr, r_dbe->b.entry, lp);

#if VERIFY_LS
			printf("\tcreating LSA %d: old (%d %d) > new (%d %d)\n",
				r_dbe->b.entry, 
				ospf_lsa_age(state, dbe, lp), dbe->seqnum,
				r_dbe->b.age, r_dbe->seqnum);
#endif
			return 1;
		}

		// LSA already present in the database... 
		// install this newer lsa in its place.
		ospf_db_update(state, nbr, r_dbe, lp);

		state->stats->s_cause_ospf += r_dbe->cause_ospf;
		state->stats->s_cause_bgp += r_dbe->cause_bgp;

#if VERIFY_LS
		printf("\tupdate LSA %d: old (%d %d) > new (%d %d)\n",
			r_dbe->b.entry, ospf_lsa_age(state, dbe, lp), dbe->seqnum,
			r_dbe->b.age, r_dbe->seqnum);
#endif
		ospf_lsa_flood(state, r_dbe, nbr->id, lp);

		if(0 == nbr->requests[r_lsa->id])
		{
			send = tw_memory_alloc(lp, g_ospf_fd);
			out_dbe = tw_memory_data(send);
			out_dbe->lsa = r_dbe->lsa;
			out_dbe->b.age = r_dbe->b.age;
			out_dbe->b.entry = r_dbe->b.entry;

			ospf_ack_direct(nbr, send, r_lsa->length, lp);

			return 1;
		}

		return 1;
	}

	if(1 == nbr->requests[r_lsa->id])
		return 0;

	if(0 == is_newer)
	{
		/*
		 * This is an implied ACK.. because the nbr just sent me 
		 * back the update I sent it..
		 */
#if VERIFY_LS
		printf("\trecvd identical LSA %d: old (%d %d) new (%d %d)\n", 
				r_lsa->id,
				ospf_lsa_age(state, dbe, lp), dbe->seqnum,
				ospf_lsa_age(state, r_dbe, lp), r_dbe->seqnum);

#endif
		/*
		 * This is the case where we recv'd a duplicate ACK.
		 */
		send = tw_memory_alloc(lp, g_ospf_fd);
		out_dbe = tw_memory_data(send);

		out_dbe->lsa = r_dbe->lsa;
		out_dbe->seqnum = ++state->lsa_seqnum;
		out_dbe->b.age = r_dbe->b.age;
		out_dbe->b.entry = r_dbe->b.entry;
		out_dbe->b.free = 0;

		ospf_ack_direct(nbr, send, r_lsa->length, lp);

		return 1;
	} else if(-1 == is_newer)
	{
		/* ignore LSA */
		if(ospf_lsa_age(state, dbe, lp) == OSPF_LSA_MAX_AGE && 
		   dbe->seqnum == OSPF_MAX_LSA_SEQNUM)
		{
			return 1;
		}
		else
		{
#if VERIFY_LS
			printf("\trecvd older LSA %d: old (%d %d) new (%d %d)\n", 
					r_lsa->id,
					ospf_lsa_age(state, dbe, lp), dbe->seqnum,
					ospf_lsa_age(state, r_dbe, lp), 
					r_dbe->seqnum);
#endif

			/*
			 * Database copy more recent. check whether LSA has 
			 * not been sent out within last MinLSArrival 
			 * seconds(todo), send the database copy back to 
			 * the sending neighbor 
			 */
			send = tw_memory_alloc(lp, g_ospf_fd);

			out_dbe = tw_memory_data(send);
			out_dbe->lsa = dbe->lsa;
			out_dbe->seqnum = dbe->seqnum;
			out_dbe->b.free = 0;
			out_dbe->b.entry = dbe->b.entry;
			out_dbe->b.age = ospf_lsa_age(state, dbe, lp);

			e = ospf_event_new(state, tw_getlp(nbr->id), 0.0, lp);

			ospf_event_send(state, e, OSPF_LS_UPDATE, lp, 
					r_lsa->length, 
					send, state->ar->id);
		}
	}

	return 1;
}

/**
 * This function should only be called when we get an update message
 * from an adjacency where we are in the exchange or loading state.
 */
void
ls_update_process(ospf_state *state, ospf_nbr * nbr, tw_lp *lp)
{
	tw_memory	*b;
	
	ospf_db_entry	*dbe;
	ospf_lsa	*lsa;

	// Process all LSA's recv'd
	for(b = tw_event_memory_get(lp); b; b = tw_event_memory_get(lp))
	{
		dbe = tw_memory_data(b);
		lsa = getlsa(state->m, dbe->lsa, dbe->b.entry);

		printf("\t%lld: process lsa %d \n", lp->id, lsa->adv_r);
		if(ospf_list_contains(nbr->requests, lsa->id) &&
			process_lsa_recv(state, nbr, dbe, lp))
		{
			ospf_list_unlink(nbr->requests, 
					 lsa->id,
					 &nbr->nrequests);
		}

		tw_memory_free(lp, b, g_ospf_fd);
	}

	// This will send more LS_REQUESTS if necessary, otherwise we
	// transition to full_st from loading/exchange
	ospf_nbr_event_handler(state, nbr, ospf_nbr_exchange_done_ev, lp);
}

void
ospf_ls_update_recv(ospf_state *state, tw_bf *bf, ospf_nbr * nbr, tw_lp *lp)
{
	tw_memory	*b;

	ospf_db_entry	*dbe;

	if(nbr->state == ospf_nbr_exchange_st || nbr->state == ospf_nbr_loading_st)
	{
		ls_update_process(state, nbr, lp);
		return;
	}

	if (nbr->state != ospf_nbr_full_st)
	{
		printf("%lld: dropping LS update, nbr is %d (down=%d) \n",
			lp->id, nbr->state, ospf_nbr_down_st);

		
		while(NULL != (b = tw_event_memory_get(lp)))
			tw_memory_free(lp, b, g_ospf_fd);

		return;
		//tw_error(TW_LOC, "Unhandled neigbor state!");
	}

	while(NULL != (b = tw_event_memory_get(lp)))
	{
		dbe = tw_memory_data(b);

		// normal flood, LSA correctly set already
		if(state->ar == nbr->ar)
		{
			//lsa = getlsa(state->m, dbe->lsa, dbe->b.entry);
		} else
		{
			// re-point dbe for my AREA
			dbe->b.entry = dbe->b.entry - nbr->ar->nmachines + 
							state->ar->nmachines;
			dbe->lsa = ospf_lsa_find(state, nbr, dbe->b.entry, lp);
		}

#if VERIFY_LS
		printf("\tprocess LSA %d from nbr %d \n", 
			dbe->b.entry - state->ar->nmachines, nbr->id);
#endif

		process_lsa_recv(state, nbr, dbe, lp);
		tw_memory_free(lp, b, g_ospf_fd);
	}
}

/*
 * This function creates and sends an LS_REQUEST pkt to our nbr.
 *
 * Only one LS_REQUEST per nbr is allowed outstanding.
 * The rxmt timer is set and if fires, then resends the previous LS_REQUEST.
 *
 * NOTE: This function should not be called if nbr->nrequests == 0, but we will
 *       check for it anyway!
 */
void
ospf_ls_request(ospf_state * state, ospf_nbr * nbr, tw_lp * lp)
{
	tw_event	*e;
	tw_memory	*b;

	ospf_db_entry	*dbe;
	ospf_db_entry	*out_dbe;
	ospf_lsa	*lsa;

	int	 accum;
	int	 i;

	if(0 == nbr->nrequests)
		return;

	e = ospf_event_new(state, tw_getlp(nbr->id), 0.0, lp);

	accum = state->gstate->mtu - OSPF_HEADER;

	for(i = 0; accum >= OSPF_LSA_UPDATE_HEADER && i < state->ar->nmachines; i++)
	{
		if(!nbr->requests[i])
			continue;

		printf("%lld: Adding LSA %d to LS_REQUEST \n", lp->id, i);
		accum -= OSPF_LSA_UPDATE_HEADER;

		dbe = &nbr->router->db[i];
		lsa = getlsa(state->m, dbe->lsa, i);

		b = tw_memory_alloc(lp, g_ospf_fd);
		out_dbe = tw_memory_data(b);

		out_dbe->lsa = dbe->lsa;
		out_dbe->b.age = ospf_lsa_age(nbr->router, dbe, lp);
		out_dbe->b.entry = dbe->b.entry;

		tw_event_memory_setfifo(e, b, g_ospf_fd);
	}

	ospf_event_send(nbr->router, e, OSPF_LS_REQUEST, lp, 
			state->gstate->mtu - accum - OSPF_HEADER, 
			NULL, nbr->router->ar->id);

	printf("%lld: sent LS_REQUEST to %d, ts %lf \n", 
		lp->id, nbr->id, e->recv_ts);

	// cannot remember how I was going to eliminate this timer.
	//nbr->next_retransmit = tw_now(lp) + OSPF_RETRANS_TIMEOUT;

	nbr->retrans_timer = ospf_timer_start(nbr, 
				nbr->retrans_timer, OSPF_RETRANS_INTERVAL, 
				OSPF_RETRANS_TIMEOUT, lp);

	if(nbr->retrans_timer)
		printf("%lld: Setting retransmit timer for %d at %lf \n",
			lp->id, nbr->id, nbr->retrans_timer->recv_ts);
}

/*
 * This function is the handler for LS_REQUEST packets from our neighbors.
 *
 * The proper action is to create an update with the requested LSAs.
 */
void
ospf_ls_request_recv(ospf_state * state, tw_bf * bf, ospf_nbr * nbr, tw_lp * lp)
{
	tw_event		*e;
	tw_memory		*b;
	tw_memory		*recv;

	ospf_db_entry		*dbe = NULL;
	ospf_db_entry		*in_dbe;
	ospf_db_entry		*out_dbe = NULL;
	ospf_lsa		*lsa;

	int			 accum;

	if(nbr->state < ospf_nbr_exchange_st)
		return;

	e = ospf_event_new(state, tw_getlp(nbr->id), 0.0, lp);

	accum = state->gstate->mtu - OSPF_HEADER;

	if(lp->pe->cur_event->memory == NULL)
		tw_error(TW_LOC, "Nothing on event to recv!");

	// Should probably reuse the membuf, and simply attach LSA, but that
	// would probably be a nightmare ro RC!

	// There better be at least one lsa hdr!
	while(NULL != (recv = tw_event_memory_get(lp)))
	{
		in_dbe = tw_memory_data(recv);
		lsa = getlsa(state->m, in_dbe->lsa, in_dbe->b.entry);
		dbe = &state->db[lsa->adv_r];

		// If we have filled this event, create a new one
		if(accum < lsa->length)
		{
			// can get here since update are > requests
			tw_error(TW_LOC, "Should not be here!");
/*
			e = ospf_event_send(state, tw_getlp(nbr->id), OSPF_LS_UPDATE,
					lp, LINK_TIME, NULL, nbr->router->area);
*/
			accum = state->gstate->mtu - OSPF_HEADER;
		}

		accum -=  lsa->length;

		b = tw_memory_alloc(lp, g_ospf_fd);

		out_dbe = tw_memory_data(b);
		out_dbe->lsa = dbe->lsa;
		out_dbe->b.age = ospf_lsa_age(state, dbe, lp);
		out_dbe->b.entry = in_dbe->b.entry;

		printf("%lld: adding LSA %d ages: old %d, new %d \n", 
			lp->id, lsa->adv_r, dbe->b.age, out_dbe->b.age);

		tw_event_memory_setfifo(e, b, g_ospf_fd);
		tw_memory_free(lp, recv, g_ospf_fd);
	}

	ospf_event_send(state, e, OSPF_LS_UPDATE, lp, 
			state->gstate->mtu - accum - OSPF_HEADER,
			NULL, nbr->router->ar->id);

	printf("%lld: Sending LSA UPDATE to %d, %lf ages: old %d, new %d \n", 
				lp->id, nbr->id, e->recv_ts,
				dbe->b.age, out_dbe->b.age);
}
