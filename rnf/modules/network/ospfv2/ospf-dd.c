#include <ospf.h>

void
dd_process(ospf_state * state, ospf_nbr * nbr, tw_memory * b, tw_lp * lp)
{
	tw_memory	*last;
	tw_memory	*next_dbe;

	ospf_dd_pkt	*in_dd;
	ospf_db_entry	*in_dbe;
	ospf_lsa	*lsa;

	in_dd = tw_memory_data(b);

	if(!nbr->master)
		nbr->dd_seqnum++;
	else
		nbr->dd_seqnum = in_dd->seqnum;

	last = NULL;
	for(next_dbe = b->next; NULL != next_dbe; next_dbe = next_dbe->next)
	{
		if(last)
			tw_memory_free(lp, last, g_ospf_fd);

		last = next_dbe;

		in_dbe = tw_memory_data(next_dbe);
		lsa = getlsa(state->m, in_dbe->lsa, in_dbe->b.entry);

		if(lsa->type < 1 || lsa->type > 5 ||
		   (lsa->type == ospf_lsa_as_ext && 
		    nbr->ltype == ospf_link_stub))
		{
			ospf_nbr_event_handler(state,nbr, 
				ospf_nbr_seqnum_mismatch_ev, lp);

			continue;
		}

		if(ospf_lsa_isnewer(state, &state->db[in_dbe->b.entry], 
				in_dbe, lp))
		{
printf("%lld: LS REQUEST for next dbe %d \n", lp->id, in_dbe->b.entry);

			nbr->requests[in_dbe->b.entry] = 1;
			nbr->nrequests++;
		} //else
//printf("%ld: NO LS REQUEST for next dbe %d \n", lp->id, in_dbe->b.entry);
	}

	printf("%lld: LS REQUEST for %d lsas \n", lp->id, nbr->nrequests);

	if(last)
		tw_memory_free(lp, last, g_ospf_fd);
}

/*
 * Add the LSA headers to the DD packet being sent
 */
void
dd_exchange(ospf_state * state, ospf_nbr * nbr, tw_lp * lp)
{
	tw_event	*e;
	tw_memory	*head;
	tw_memory	*last;
	tw_memory	*next_dbe;

	ospf_dd_pkt	*out_dd;
	ospf_db_entry	*out_dbe;

	int	 i;
	int	 accum;

	// adds OSPF header
	e = ospf_event_new(state, tw_getlp(nbr->id), 0.0, lp);

	// the retrans_dd membuf is freed by the other router
	head = last = nbr->retrans_dd = tw_memory_alloc(lp, g_ospf_dd_fd);

	// add the DD pkt header to tail
	tw_event_memory_set(e, head, g_ospf_dd_fd);

	i = nbr->next_db_entry;
	out_dd = tw_memory_data(head);

	accum = state->gstate->mtu - OSPF_DD_HEADER;

	/*
	 * Add the LSA header info and increment nlsas
	 */
	out_dd->nlsas = 0;
	for(i = 0; accum > OSPF_LSA_UPDATE_HEADER && 
		   i < state->ar->nmachines; i++)
	{
		if((rn_route(state->m, i) == -1 && i != lp->id) || 
		     i == lp->id)
			continue;

		accum -= OSPF_LSA_UPDATE_HEADER;
		out_dd->nlsas++;
		
		next_dbe = tw_memory_alloc(lp, g_ospf_fd);

		// add the LSA hdr membuf to tail
		tw_event_memory_setfifo(e, next_dbe, g_ospf_fd);
		out_dbe = tw_memory_data(next_dbe);

		out_dbe->b.age = ospf_lsa_age(state, &state->db[i], lp);
		out_dbe->b.entry = state->db[i].b.entry;
		out_dbe->lsa = state->db[i].lsa;
		out_dbe->seqnum = state->db[i].seqnum;

#if VERIFY_LS || 1
		printf("%lld: Sending LSA %d with ages: old %d, "
			"new %d \n", lp->id, i, 
			ospf_lsa_age(state, &state->db[i], lp),
			out_dbe->b.age);
#endif

		nbr->next_db_entry = i + 1;
		last->next = next_dbe;
		last = next_dbe;
	}

	// if master, else slave
	if(!nbr->master)
	{
		out_dd->seqnum = nbr->dd_seqnum++;
		out_dd->b.init = 0;
		out_dd->b.master = 1;

		if(i < state->ar->g_ospf_nlsa)
			out_dd->b.more = 1;
		else
			out_dd->b.more = 0;

		nbr->retrans_timer = ospf_timer_start(nbr, 
					nbr->retrans_timer, OSPF_RETRANS_INTERVAL, 
					OSPF_RETRANS_TIMEOUT, lp);
	} else
	{
		out_dd->b.init = 0;
		out_dd->b.master = 0;
		out_dd->seqnum = nbr->dd_seqnum;

		if(i < state->ar->g_ospf_nlsa)
			out_dd->b.more = 1;
		else
			out_dd->b.more = 0;

		nbr->retrans_timer = ospf_timer_start(nbr, 
					nbr->retrans_timer, OSPF_RETRANS_INTERVAL, 
					OSPF_RETRANS_TIMEOUT, lp);
	}

	ospf_event_send(state, e, OSPF_DD_MSG, lp, 
				state->gstate->mtu - accum - OSPF_HEADER,
				NULL, state->ar->id);
	state->stats->s_sent_dds++;

	printf("%lld: send Exch to %d at %lf \n", lp->id, nbr->id, e->recv_ts);
}

/*

	A Database Description Packet (dd packet) has 3 parts:

	1) OSPF message header which ospf_event_send adds on and contains:
		a) message type (OSPF_DD_MSG)
		b) OSPF area id
	2) DD Packet which contains:
		a) init, more, master bits
		b) master status (1 = master, 0 = slave)
		c) nlsas which is # of lsa headers in this DD packet
	3) LSA Headers, no more than OSPF_MTU will allow, and contain:
		a) LSA age
		b) options
		c) LSA type
		d) link state id
		e) advertising router
		f) LS sequence number
		g) LS checksum
		h) length

1, 2, and 3 are each represented by membufs.  1 uses FD g_ospf_dd_fd and 2 uses FD 
g_ospf_fd and part 3 is not sent.  This is because part 2 is enough for the recvr 
to lookup the LSA from the global LSA table and figure out what it needs.  The 
recvr should only need to compare the pointers o the sender and recvr to the table 
to know if an LS Request should be made for the LSA.  So, we don't really need to 
send too much information.  

However, in order to do the simulation correctly, we must compute the if() block
on the recvr side which determines if we should create an LS request pkt.  If things
go screwy and the if fails, even though the sender's LSA is newer, then we will
NOT make the request, even though we should.
*/

tw_memory	*
ospf_dd_copy(tw_memory *b2, tw_lp * lp)
{
	tw_memory	*b;
	tw_memory	*head;
	tw_memory	*last;
	tw_memory	*next_b;

	ospf_dd_pkt	*h1;
	ospf_dd_pkt	*h2;
	ospf_db_entry	*dbe;
	ospf_db_entry	*h2_dbe;

	int	i;

	b = tw_memory_alloc(lp, g_ospf_dd_fd);
	h1 = tw_memory_data(b);
	h2 = tw_memory_data(b2);

	h1->b.init = h2->b.init;
	h1->b.more = h2->b.more;
	h1->b.master = h2->b.master;
	h1->seqnum = h2->seqnum;
	h1->nlsas = h2->nlsas;

	head = last = b;
	next_b = b2->next;
	for(i = 0; i < h2->nlsas; i++, next_b = next_b->next)
	{
		b = tw_memory_alloc(lp, g_ospf_fd);

		dbe = tw_memory_data(b);
		h2_dbe = tw_memory_data(next_b);

		dbe->b = h2_dbe->b;
		dbe->lsa = h2_dbe->lsa;
		dbe->seqnum = h2_dbe->seqnum;

		last->next = b;
		last = b;
	}

	return head;
}

/*
 * We need to copy the entire retrans_dd and resend.  If we were to try
 * and re-use the membuf(s) by just re-sending a pointer, then the dest
 * would likely end up attempting to free the membuf twice, which would be
 * a problem -- especially after the first free, and the membuf gets 
 * recycled.
 *
 * If we have moved into the loading_st, then we are actually retranmistting LS
 * requests.
 */
void
ospf_dd_retransmit(ospf_state * state, ospf_nbr * nbr, tw_bf * bf, tw_lp * lp)
{
	tw_event	*e;
	tw_memory	*b;
	ospf_dd_pkt	*dd;

	b = ospf_dd_copy(nbr->retrans_dd, lp);
	dd = tw_memory_data(b);

	printf("%lld OSPF: retransmit DD to %d, ts %lf \n",
		lp->id, nbr->id, tw_now(lp) + OSPF_RETRANS_INTERVAL);

	e = ospf_event_new(state, tw_getlp(nbr->id), 0.0, lp);
	ospf_event_send(nbr->router, e, OSPF_DD_MSG, lp,
			OSPF_DD_HEADER + (dd->nlsas * OSPF_LSA_UPDATE_HEADER), 
			b, nbr->router->ar->id);
	
	nbr->retrans_timer =
		ospf_timer_start(nbr, nbr->retrans_timer,
					OSPF_RETRANS_INTERVAL, 
				 	OSPF_RETRANS_TIMEOUT, 
					lp);
}

void
ospf_dd_event_handler(ospf_state * state, ospf_nbr * nbr, tw_bf *bf, tw_lp *lp)
{
	tw_event	*e;
	tw_memory	*b;
	tw_memory	*send;

	ospf_dd_pkt	*in_dd;
	ospf_dd_pkt	*out_dd;

	b = lp->pe->cur_event->memory; //tw_event_memory_get(lp);
	lp->pe->cur_event->memory = NULL;
	in_dd = tw_memory_data(b);

	printf("%lld OSPF: DD from %d, ts %lf, state %d, I %d, M %d, Master %d \n",
		lp->id, nbr->id, tw_now(lp), nbr->state, in_dd->b.init, in_dd->b.more,
		in_dd->b.master);

	switch(nbr->state)
	{
		case ospf_nbr_down_st:
			state->stats->s_drop_dd++;
			break;
		case ospf_nbr_attempt_st:
			state->stats->s_drop_dd++;
			break;
		case ospf_nbr_init_st:
			printf("%lld OSPF: DD sending nbr %d 2-way recv event \n",
						lp->id, nbr->id);

			ospf_nbr_event_handler(state, nbr, ospf_nbr_two_way_recv_ev, lp);

			if(nbr->state != ospf_nbr_exstart_st)
				break;
		case ospf_nbr_exstart_st:
			if(in_dd->b.init && in_dd->b.more && in_dd->b.master &&
			   in_dd->nlsas == 0 && nbr->id > lp->id)
			{
				nbr->master = 1;
				nbr->dd_seqnum = in_dd->seqnum;
			} else 
				nbr->master = 0;

			state->stats->s_proc_dd++;
			nbr->last_recv_dd = ospf_dd_copy(b, lp);
			ospf_nbr_event_handler(state, nbr, ospf_nbr_neg_done_ev, lp);
			break;
		case ospf_nbr_two_way_st:
			state->stats->s_drop_dd++;
			break;
		case ospf_nbr_exchange_st:
			state->stats->s_proc_dd++;

			/*
			 * Verify DD packet received is in proper format
			 */
			if(in_dd->b.init)
			{
				ospf_nbr_event_handler(state,nbr, 
						ospf_nbr_seqnum_mismatch_ev, lp);

				break;
			} else if(in_dd->b.master != nbr->master)
			{
				ospf_nbr_event_handler(state,nbr, 
						ospf_nbr_seqnum_mismatch_ev, lp);

				break;
			} else if((nbr->master && in_dd->seqnum != nbr->dd_seqnum+1) ||
				  (!nbr->master && in_dd->seqnum != nbr->dd_seqnum))
			{
				ospf_nbr_event_handler(state,nbr, 
						ospf_nbr_seqnum_mismatch_ev, lp);

				break;
			}

			/*
			 * If the slave recvs a duplicate, it resends it
			 */
			if(nbr->master &&
			   ospf_dd_compare(in_dd, tw_memory_data(nbr->last_recv_dd)))
			{
				ospf_dd_retransmit(state, nbr, bf, lp);
				
				break;
			}

printf("%lld: got valid DD in exchange from %d, more = %d \n", lp->id, nbr->id, in_dd->b.more);

			nbr->last_recv_dd = ospf_dd_copy(b, lp);
			dd_process(state, nbr, b, lp);
			dd_exchange(state, nbr, lp);

			if(in_dd->b.more == 0)
				ospf_nbr_event_handler(state, nbr, 
					ospf_nbr_exchange_done_ev, lp);

			break;
		case ospf_nbr_loading_st:
		case ospf_nbr_full_st:

			if(ospf_dd_compare(in_dd, tw_memory_data(nbr->last_recv_dd)))
			{
				/// if master
				if(!nbr->master)
				{
					send = ospf_dd_copy(nbr->retrans_dd, lp);
					out_dd = tw_memory_data(send);

					out_dd->b.master = 0;
					out_dd->b.init = 0;
					out_dd->b.more = 0;

					e = ospf_event_new(state, tw_getlp(nbr->id),
							   0.0, lp);

					ospf_event_send(state, e,
						OSPF_DD_MSG, lp, 
						OSPF_DD_HEADER + 
							(out_dd->nlsas * 
							 OSPF_LSA_UPDATE_HEADER),
						send, state->ar->id);

					state->stats->s_sent_dds++;
				}
			} else
				ospf_nbr_event_handler(state,nbr, 
					ospf_nbr_seqnum_mismatch_ev, lp);
			break;
		default:
			printf("%lld: Received DD with unknown type\n", lp->id);
	}

	tw_memory_free(lp, b, g_ospf_dd_fd);
}

int
ospf_dd_compare(ospf_dd_pkt *h1, ospf_dd_pkt *h2)
{
	if (h1->b.init == h2->b.init &&
	    h1->b.more == h2->b.more &&
	    h1->b.master == h2->b.master &&
	    h1->seqnum == h2->seqnum+1)
	{
		return 1;
	}

	return 0;
}
