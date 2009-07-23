#include <ospf.h>

/*
	Note: if there are multiple links to neighbor, neighbor will
	originate another LSA and send it to us, as will we.  So it
	doesn't matter that I am taking down this neighbor when only
	one of the links change status between 2 routers/areas.
*/

void
ospf_nbr_reset(ospf_nbr * nbr, tw_lp * lp)
{
	rn_machine	*m;
	rn_area		*ar;

	ospf_lsa	*lsa;
	int		 id;

	m = nbr->router->m;
	nbr->state = ospf_nbr_down_st;
	nbr->inactivity_timer = ospf_timer_cancel(nbr->inactivity_timer, lp);

	nbr->retrans_accum = 0;
	nbr->next_retransmit = -1;
	nbr->retrans_timer = NULL;
	nbr->flood = NULL;
	nbr->dd_seqnum = tw_rand_integer(lp->rng, 0, INT_MAX);

	ar = rn_getarea(rn_getmachine(lp->id));
	if(ar == rn_getarea(nbr->m))
		id = nbr->id - ar->low;
	else if(ar->as == rn_getas(nbr->m))
		id = ar->nmachines + nbr->ar->id - rn_getas(nbr->m)->low;
	else
		id = ar->nmachines + ar->as->nareas + rn_getas(nbr->m)->id;

	// Remove the neighbor LSA
	ospf_db_remove(nbr->router, id, lp);

	// Remove any other LSAs originated by this neighbor ..
	for(id = ar->nmachines; id < ar->g_ospf_nlsa; id++)
	{
		lsa = getlsa(nbr->router->m, nbr->router->db[id].lsa, id);

		if(m->link[m->ft[lsa->adv_r]].addr == nbr->id)
		{
			ospf_lsa_flush(nbr->router, nbr, &nbr->router->db[id], lp);
			ospf_db_remove(nbr->router, id, lp);
		}
	}

	ospf_rt_build(nbr->router, lp->id);
}

void
ospf_nbr_reset_rc(ospf_nbr * nbr, tw_lp * lp)
{
	tw_error(TW_LOC, "Unable to rollback nbr state yet!");
	//nbr->state = nbr->rc_state;
}

/*

An adjacency should be established with a bidirectional neighbor
        when at least one of the following conditions holds:

        o   The underlying network type is point-to-point

        o   The underlying network type is Point-to-MultiPoint

        o   The underlying network type is virtual link

        o   The router itself is the Designated Router

        o   The router itself is the Backup Designated Router

        o   The neighboring router is the Designated Router

        o   The neighboring router is the Backup Designated Router
*/

/*
 * Only working with p2p right now, so this is
 * always returning true
 */
int
ospf_nbr_establish_adjacency(ospf_nbr * nbr, tw_lp * lp)
{
	return 1;
}

/*
State(s):  Exchange

            Event:  ExchangeDone

        New state:  Depends upon action routine.

           Action:  If the neighbor Link state request list is empty,
                    the new neighbor state is Full.  No other action is
                    required.  This is an adjacency's final state.

                    Otherwise, the new neighbor state is Loading.  Start
                    (or continue) sending Link State Request packets to
                    the neighbor (see Section 10.9).  These are requests
                    for the neighbor's more recent LSAs (which were
                    discovered but not yet received in the Exchange
                    state).  These LSAs are listed in the Link state
                    request list associated with the neighbor.

*/

void
ospf_nbr_exchange_done(ospf_state * state, ospf_nbr * nbr, tw_lp * lp)
{
	if(nbr->nrequests == 0)
	{
		nbr->state = ospf_nbr_full_st;

		if(nbr->ar == nbr->router->ar)
		{
			//state->stats->s_cause_ospf++;
			ospf_lsa_create(state, nbr, 
					lp->id - state->m->subnet->area->low, lp);
		} else
		{
			//state->stats->s_cause_ospf++;
			ospf_lsa_create(nbr->router, nbr, 
				rn_getarea(nbr->m)->id + 
				nbr->router->m->subnet->area->nmachines, lp);
		}

		printf("%lld: Exchange done: nbr %d full \n", lp->id, nbr->id);

		// Cancel the retrans timer now that I have recv'd the 
		// response to my LS_REQUEST
		nbr->retrans_timer = ospf_timer_cancel(nbr->retrans_timer, lp);
	} else
	{
		nbr->state = ospf_nbr_loading_st;

		ospf_ls_request(state, nbr, lp);

		printf("%lld: Exchange done: nbr %d loading \n", lp->id, nbr->id);
	}
}

/*
 * Set the state to attempt from down, and set the inactive timer
 */
void
ospf_nbr_start(ospf_nbr * nbr, tw_lp * lp)
{
	if (nbr->state == ospf_nbr_down_st)
	{
		printf("%lld: setting nbr %d to attempt!\n", lp->id, nbr->id);
		nbr->state = ospf_nbr_attempt_st;
	}

	nbr->inactivity_timer =
		ospf_timer_start(nbr, nbr->inactivity_timer, nbr->router_dead_interval,
						 OSPF_HELLO_TIMEOUT, lp);
}

/*
State(s):  Any state

            Event:  InactivityTimer

        New state:  Down

           Action:  The Link state retrans_timermission list, Database summary
                    list and Link state request list are cleared of
                    LSAs.
*/

void
ospf_nbr_inactivity_timer(ospf_nbr * nbr, tw_lp * lp)
{
#if VERIFY_LS
	printf("%ld OSPF: removing nbr %d from full st, HELLO timeout!\n",
			lp->id, nbr->id);
#endif


	if (nbr->state == ospf_nbr_full_st)
	{
		nbr->state = ospf_nbr_down_st;

		// this will also rebuild routing table for me..
		if(nbr->ar == nbr->router->ar)
		{
			ospf_lsa_create(nbr->router, nbr, 
				lp->id - nbr->router->m->subnet->area->low, lp);
		} else
		{
			ospf_lsa_create(nbr->router, nbr, 
				rn_getarea(nbr->m)->id + 
				nbr->router->m->subnet->area->nmachines - 
				nbr->router->m->subnet->area->as->low, lp);
		}

		ospf_nbr_reset(nbr, lp);
	} else
		ospf_nbr_reset(nbr, lp);
}

void
ospf_nbr_inactivity_timer_rc(ospf_nbr * nbr, tw_bf * bf, tw_lp * lp)
{
	ospf_nbr_reset_rc(nbr, lp);

	if(bf->c1 == 1)
	{
		//ospf_lsa_create_rc(nbr->router, nbr, lp->id - nbr->router->m->subnet->area->low, lp);
	}
}

/*
State(s):  Any state

            Event:  KillNbr

        New state:  Down

           Action:  The Link state retrans_timermission list, Database summary
                    list and Link state request list are cleared of
                    LSAs.  Also, the Inactivity Timer is disabled.
*/

void
ospf_nbr_kill_nbr(ospf_nbr * nbr, tw_lp * lp)
{
	//printf("%ld: KILLING NBR %d !!! \n", lp->id, nbr->id);
	ospf_nbr_reset(nbr, lp);
}


/*
State(s):  2-Way or greater

            Event:  1-WayReceived

        New state:  Init

           Action:  The Link state retrans_timermission list, Database summary
                    list and Link state request list are cleared of
                    LSAs.
*/

void
ospf_nbr_one_way_recv(ospf_nbr * nbr, tw_lp * lp)
{
	if (nbr->state >= ospf_nbr_two_way_st)
		nbr->state = ospf_nbr_init_st;
}

/*
State(s):  Any state

            Event:  LLDown

        New state:  Down

           Action:  The Link state retrans_timermission list, Database summary
                    list and Link state request list are cleared of
                    LSAs.  Also, the Inactivity Timer is disabled.
*/

void
ospf_nbr_ll_down(ospf_nbr * nbr, tw_lp * lp)
{
	ospf_nbr_reset(nbr, lp);
}

/*

         State(s):  Attempt

            Event:  HelloReceived

        New state:  Init

           Action:  Restart the Inactivity Timer for the neighbor, since
                    the neighbor has now been heard from.
*/

void
ospf_nbr_hello_recv(ospf_nbr * nbr, tw_lp * lp)
{
	switch (nbr->state)
	{
	case ospf_nbr_attempt_st:
	case ospf_nbr_down_st:
		nbr->state = ospf_nbr_init_st;

		/*
		 * all other states 
		 */
	default:

		nbr->inactivity_timer =
			ospf_timer_start(nbr, nbr->inactivity_timer,
					 nbr->router_dead_interval,
					 OSPF_HELLO_TIMEOUT, lp);
	}
}

/*
        New state:  Depends upon action routine.

           Action:  Determine whether an adjacency should be established
                    with the neighbor (see Section 10.4).  If not, the
                    new neighbor state is 2-Way.

                    Otherwise (an adjacency should be established) the
                    neighbor state transitions to ExStart.  Upon
                    entering this state, the router increments the DD
                    sequence number in the neighbor data structure.  If
                    this is the first time that an adjacency has been
                    attempted, the DD sequence number should be assigned
                    some unique value (like the time of day clock).  It
                    then declares itself master (sets the master/slave
                    bit to master), and starts sending Database
                    Description Packets, with the initialize (I), more
                    (M) and master (MS) bits set.  This Database
                    Description Packet should be otherwise empty.  This
                    Database Description Packet should be retransmitted
                    at intervals of RxmtInterval until the next state is
                    entered (see Section 10.8).
*/

void
ospf_nbr_two_way_recv(ospf_nbr * nbr, tw_lp * lp)
{
	tw_event	*e;
	tw_memory	*b;
	ospf_dd_pkt	*dd;

	if (nbr->state != ospf_nbr_init_st)
		return;

	if (ospf_nbr_establish_adjacency(nbr, lp))
	{
		// send dd pkt
		b = tw_memory_alloc(lp, g_ospf_dd_fd);
		dd = tw_memory_data(b);

		nbr->state = ospf_nbr_exstart_st;
		nbr->master = 0;

		dd->b.init = 1;
		dd->b.more = 1;
		dd->b.master = 1;
		dd->seqnum = ++nbr->dd_seqnum;
		dd->nlsas = 0;

		nbr->retrans_dd = ospf_dd_copy(b, lp);

		e = ospf_event_new(nbr->router, tw_getlp(nbr->id), 0.0, lp);
		ospf_event_send(nbr->router, e, OSPF_DD_MSG,
				lp, OSPF_DD_HEADER, b, nbr->router->ar->id);

		printf("%lld OSPF: send ExStart to %d, %lf: I %d, M %d, Master %d \n",
			lp->id, nbr->id, e->recv_ts, dd->b.init, dd->b.more, dd->b.master);

		nbr->retrans_timer =
			ospf_timer_start(nbr, nbr->retrans_timer,
						OSPF_RETRANS_INTERVAL, 
					 	OSPF_RETRANS_TIMEOUT, 
						lp);
	} else
	{
		tw_error(TW_LOC, "Only simulating broadcast networks!");
	}
}

/*
         State(s):  ExStart

            Eventid :  NegotiationDone

        New state:  Exchange

           Action:  The router must list the contents of its entire area
                    link state database in the neighbor Database summary
                    list.  The area link state database consists of the
                    router-LSAs, network-LSAs and summary-LSAs contained
                    in the area structure, along with the AS-external-
                    LSAs contained in the global structure.  AS-
                    external-LSAs are omitted from a virtual neighbor's
                    Database summary list.  AS-external-LSAs are omitted
                    from the Database summary list if the area has been
                    configured as a stub (see Section 3.6).  LSAs whose
                    age is equal to MaxAge are instead added to the
                    neighbor's Link state retrans_timermission list.  A
                    summary of the Database summary list will be sent to
                    the neighbor in Database Description packets.  Each
                    Database Description Packet has a DD sequence
                    number, and is explicitly acknowledged.  Only one
                    Database Description Packet is allowed outstanding
                    at any one time.  For more detail on the sending and
                    receiving of Database Description packets, see
                    Sections 10.8 and 10.6.
*/
void
ospf_nbr_neg_done(ospf_state * state, ospf_nbr * nbr, tw_lp * lp)
{
	tw_event	*e;
	tw_memory	*b;

	ospf_dd_pkt	*dd;

	if (nbr->state != ospf_nbr_exstart_st)
		tw_error(TW_LOC, "Invalid state for nbr_neg_done!");

	nbr->state = ospf_nbr_exchange_st;

	// master must wait for slave to reply with master's seqnum
	if(0 == nbr->master)
		return;

	// send dd pkt
	b = tw_memory_alloc(lp, g_ospf_dd_fd);
	dd = tw_memory_data(b);

	dd->b.init = 0;
	dd->b.more = 1;

	if(nbr->master)
	{
		//nbr->dd_seqnum++;
		dd->b.master = 0;
	} else
		dd->b.master = 1;

	dd->seqnum = nbr->dd_seqnum;
	dd->nlsas = 0;

	e = ospf_event_new(nbr->router, tw_getlp(nbr->id), 0.0, lp);
	ospf_event_send(nbr->router, e, OSPF_DD_MSG, lp, 
			OSPF_DD_HEADER, b, nbr->router->ar->id);

	printf("%lld OSPF: send Exchange to %d %lf, seqnum %d, nbr %d master %d \n",
		lp->id, nbr->id, e->recv_ts, dd->seqnum, nbr->id, nbr->master);

	nbr->retrans_dd = ospf_dd_copy(b, lp);

	nbr->retrans_timer =
			ospf_timer_start(nbr, nbr->retrans_timer,
						OSPF_RETRANS_INTERVAL, 
					 	OSPF_RETRANS_TIMEOUT, 
						lp);
}

/*

         State(s):  2-Way

            Event:  AdjOK?

        New state:  Depends upon action routine.

           Action:  Determine whether an adjacency should be formed with
                    the neighboring router (see Section 10.4).  If not,
                    the neighbor state remains at 2-Way.  Otherwise,
                    transition the neighbor state to ExStart and perform
                    the actions associated with the above state machine
                    entry for state Init and event 2-WayReceived.


         State(s):  ExStart or greater

            Event:  AdjOK?

        New state:  Depends upon action routine.

           Action:  Determine whether the neighboring router should
                    still be adjacent.  If yes, there is no state change
                    and no further action is necessary.

                    Otherwise, the (possibly partially formed) adjacency
                    must be destroyed.  The neighbor state transitions
                    to 2-Way.  The Link state retrans_timermission list,
                    Database summary list and Link state request list
                    are cleared of LSAs.

*/

void
ospf_nbr_adj_ok(ospf_nbr * nbr, tw_lp * lp)
{
	if (nbr->state == ospf_nbr_two_way_st)
	{
		if (ospf_nbr_establish_adjacency(nbr, lp))
		{
			nbr->state = ospf_nbr_init_st;
			ospf_nbr_two_way_recv(nbr, lp);
		}
	} else if (nbr->state >= ospf_nbr_exstart_st)
	{
		if (ospf_nbr_establish_adjacency(nbr, lp))
			return;

		nbr->state = ospf_nbr_two_way_st;
	}
}

/*

         State(s):  Exchange or greater

            Event:  SeqNumberMismatch

        New state:  ExStart

           Action:  The (possibly partially formed) adjacency is torn
                    down, and then an attempt is made at
                    reestablishment.  The neighbor state first
                    transitions to ExStart.  The Link state
                    retransmission list, Database summary list and Link
                    state request list are cleared of LSAs.  Then the
                    router increments the DD sequence number in the
                    neighbor data structure, declares itself master
                    (sets the master/slave bit to master), and starts
                    sending Database Description Packets, with the
                    initialize (I), more (M) and master (MS) bits set.
                    This Database Description Packet should be otherwise
                    empty (see Section 10.8).
*/
void
ospf_nbr_seqnum_mismatch(ospf_nbr * nbr, tw_lp * lp)
{
	nbr->state = ospf_nbr_init_st;
	ospf_nbr_two_way_recv(nbr, lp);

	tw_error(TW_LOC, "Should not be getting mismatched seqnums on DD pkts!");
}

/*
 * Process nbr events
 */
void
ospf_nbr_event_handler(ospf_state * state, ospf_nbr * nbr, ospf_nbr_event event, tw_lp * lp)
{
	switch (event)
	{
	case ospf_nbr_hello_recv_ev:
		ospf_nbr_hello_recv(nbr, lp);
		break;
	case ospf_nbr_start_ev:
		ospf_nbr_start(nbr, lp);
		break;
	case ospf_nbr_one_way_ev:
		ospf_nbr_one_way_recv(nbr, lp);
		break;
	case ospf_nbr_two_way_recv_ev:
		ospf_nbr_two_way_recv(nbr, lp);
		break;
	case ospf_nbr_neg_done_ev:
		ospf_nbr_neg_done(state, nbr, lp);
		break;
	case ospf_nbr_exchange_done_ev:
		ospf_nbr_exchange_done(state, nbr, lp);
		break;
	case ospf_nbr_bad_ls_request_ev:
		if (nbr->state >= ospf_nbr_exchange_st)
			ospf_nbr_seqnum_mismatch(nbr, lp);
		break;
	case ospf_nbr_load_done_ev:
		if (nbr->state == ospf_nbr_loading_st)
			nbr->state = ospf_nbr_full_st;

		printf("%lld: new neighbor at full_st: %d \n", lp->id, nbr->id);
		break;
	case ospf_nbr_adj_ok_ev:
		ospf_nbr_adj_ok(nbr, lp);
		break;
	case ospf_nbr_seqnum_mismatch_ev:
		ospf_nbr_seqnum_mismatch(nbr, lp);
		break;
	case ospf_nbr_kill_nbr_ev:
		ospf_nbr_kill_nbr(nbr, lp);
		break;
	case ospf_nbr_inactivity_timer_ev:
		ospf_nbr_inactivity_timer(nbr, lp);
		break;
	case rc_ospf_nbr_inactivity_timer_ev:
		ospf_nbr_inactivity_timer(nbr, lp);
		break;
	case ospf_nbr_ll_down_ev:
		ospf_nbr_ll_down(nbr, lp);
		break;
	default:
		tw_error(TW_LOC, "Unknown nbr event: %d", event);
	}
}
