#include <ospf.h>

/*
 * Send an actual hello message:
 *
 * Argh, should not be doing this unless a link state changes.. we can
 * just create the hello packet once and keep sending the same one..
 */
void
ospf_h_send(ospf_state * state, ospf_nbr * nbr, tw_lp * src, tw_lp * dst)
{
	tw_event	*e;
	tw_memory	*b;

	ospf_message	*m;

	int             i;
	int		j;
	int		accum;
	int		start;

	accum = OSPF_HELLO_HEADER + OSPF_HELLO_HEADER;
	i = j = 0;
	start = 0;

#if 0
	if((src->id == 2527 && dst->id == 2514) ||
	   (dst->id == 2527 && src->id == 2514))
	{
		printf("%ld OSPF: sending hello to %ld, ts %lf\n", 
			src->id, dst->id, tw_now(src));
	}
#endif

//#if 0
	while(i < state->n_interfaces)
	{
		if(state->nbr[i].state >= ospf_nbr_init_st)
		{
#if VERIFY_HELLO && 0
			printf("%ld OSPF: adding nbr %d to HELLO in %d\n", 
					src->id, state->nbr[i].id, j);
#endif
			nbr->hello->neighbors[j] = state->nbr[i].id;
			j++;
		}

		accum += 4;

		if(accum > state->gstate->mtu || i == state->n_interfaces - 1)
		{
//#endif
/*
			if(nbr->state < ospf_nbr_init_st)
				return;
*/

			b = tw_memory_alloc(src, g_ospf_fd);
			m = tw_memory_data(b);

			nbr->hello->hello_interval = state->gstate->hello_sendnext;
			nbr->hello->poll_interval = state->gstate->poll_interval;
			nbr->hello->priority = state->priority;
			nbr->hello->designated_r = state->designated_r;
			nbr->hello->b_designated_r = state->b_designated_r;

//#if 0
			nbr->hello->offset = start;

			if(i == state->n_interfaces - 1)
				nbr->hello->end = i + 1;
			else
				nbr->hello->end = i;
			nbr->hello->end = j;

//#endif
			m->data = nbr->hello;

			e = ospf_event_new(state, tw_getlp(nbr->id), 0.0, src);
			ospf_event_send(state, e, OSPF_HELLO_MSG,
					src, accum - OSPF_HEADER, b, state->ar->id);

			if(e->recv_ts < g_tw_ts_end)
				state->stats->s_e_hello_out++;
//#if 0
			//start = i + 1;
			accum = OSPF_HEADER + OSPF_HELLO_HEADER;
		}	

		i++;
	}
//#endif
}

/*
 * This function gets called every OSPF_HELLO_SENDNEXT so that we
 * send the HELLO_MSGs to our neighbors, and the next OSPF_HELLO_SEND
 * to ourselves.
 */
void
ospf_hello_send(ospf_state * state, ospf_nbr * nbr, tw_bf * bf, tw_lp * lp)
{
	nbr->hello_timer = ospf_timer_start(nbr, nbr->hello_timer,
					    nbr->hello_interval,
					    OSPF_HELLO_SEND, lp);

	if(nbr->istate >= ospf_int_loopback_st)
		ospf_h_send(state, nbr, lp, tw_getlp(nbr->id));
}

/*
 * This function gets called when we receive a hello packet
 */
void
ospf_hello_packet(ospf_state * state, ospf_nbr * nbr, ospf_hello * r,
							tw_bf * bf, tw_lp * lp)
{
	tw_memory	*b;

	ospf_message	*m;
	int		 i;
	int		 priority;

	if(NULL == nbr)
	{
		tw_error(TW_LOC, "Do not have nbr!");
		state->stats->s_drop_hello_int++;
		bf->c1 = 1;
#if VERIFY_HELLO
		printf("%ld: Dropping nbr HELLO since I have no state for it! "
			"(%f)\n", lp->id, tw_now(lp));
#endif
		return;
	}

	b = tw_event_memory_get(lp);
	m = tw_memory_data(b);
	r = m->data;

	if(!r)
		tw_error(TW_LOC, "Unable to read HELLO message!");

	/*
	 * Check the packet integrity
	 */
	if (r->hello_interval != state->gstate->hello_sendnext)
	{
		state->stats->s_drop_hello_int++;
		bf->c1 = 1;

#if VERIFY_HELLO
		printf("%ld: dropped hello due to hello interval! %g != %g \n",
				lp->id, r->hello_interval, 
				state->gstate->hello_sendnext);
#endif
		return;
	}

	if (r->poll_interval != state->gstate->poll_interval)
	{
		state->stats->s_drop_poll_int++;
		bf->c2 = 1;

		tw_memory_free(lp, b, g_ospf_fd);

#if VERIFY_HELLO
		printf("%ld: dropped hello due to poll interval! %g != %g \n",
				lp->id, r->poll_interval, state->gstate->poll_interval);
#endif
		return;
	}

	nbr->priority = r->priority;
	nbr->designated_r = r->designated_r;
	nbr->b_designated_r = r->b_designated_r;

	ospf_nbr_event_handler(state, nbr, ospf_nbr_hello_recv_ev, lp);

	/*
	 * If I am in the list of neighbors of the sending router,
	 * then send a two_way_recv event
	 */
	for (i = r->offset; i < r->end; i++)
	{
		if(r->neighbors[i] == lp->id)
		{
#if VERIFY_HELLO && 0
			printf("%d: found myself in HELLO message! \n", lp->id);
#endif

			priority = nbr->priority;
			ospf_nbr_event_handler(state, nbr, ospf_nbr_two_way_recv_ev, lp);

			if (priority != nbr->priority)
				ospf_interface_event_handler(nbr, ospf_int_nbrchange_ev, lp);

			if (r->designated_r == nbr->id &&
				r->b_designated_r == 0xffffffff && 
				nbr->istate == ospf_int_waiting_st)
			{
				ospf_interface_event_handler(nbr, ospf_int_backupseen_ev, lp);
			} else
			{
/*
	    Otherwise, if the
            neighbor is declaring itself to be Designated Router and it
            had not previously, or the neighbor is not declaring itself
            Designated Router where it had previously, the receiving
            interface's state machine is scheduled with the event
            NeighborChange.
*/
				if((r->designated_r == nbr->id &&
					r->designated_r != state->designated_r) ||
				   (r->designated_r != nbr->id &&
					r->designated_r == state->designated_r))
				{
					ospf_interface_event_handler(nbr, 
							ospf_int_nbrchange_ev, lp);
				}
			}

			if (r->b_designated_r == nbr->id &&
				nbr->istate == ospf_int_waiting_st)
			{
				ospf_interface_event_handler(nbr, ospf_int_backupseen_ev, lp);
			} else
			{
/*
	    Otherwise, if the
            neighbor is declaring itself to be Backup Designated Router and it
            had not previously, or the neighbor is not declaring itself Backup
            Designated Router where it had previously, the receiving
            interface's state machine is scheduled with the event
            NeighborChange.
*/
				if((r->b_designated_r == nbr->id &&
					r->b_designated_r != state->b_designated_r) ||
				   (r->b_designated_r != nbr->id &&
					r->b_designated_r == state->b_designated_r))
				{
					ospf_interface_event_handler(nbr, 
							ospf_int_nbrchange_ev, lp);
				}
			}

			tw_memory_free(lp, b, g_ospf_fd);

			return;
		}
	}

	/*
	 * Start one-way event with neighbor which sent this message
	 */
	ospf_nbr_event_handler(state, nbr, ospf_nbr_one_way_ev, lp);
	tw_memory_free(lp, b, g_ospf_fd);
}
