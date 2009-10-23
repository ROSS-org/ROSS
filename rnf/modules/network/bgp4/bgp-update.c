#include <bgp.h>

void update_add(bgp_state *, tw_bf *, tw_memory *, bgp_message *, int, tw_lp *);
void update_withdraw(bgp_state *, tw_bf *, bgp_route *, bgp_message *, int, tw_lp *);

void
bgp_update_send(bgp_state * state, bgp_nbr * n, bgp_route * r_send,
			bgp_message * msg, int cause, tw_lp * lp)
{
	tw_event	*e;

	tw_memory	*hdr;
	tw_memory	*b;

	//bgp_route	*test;
	bgp_message	*m;

	state->stats->s_nupdates_sent++;
	e = rn_event_new(n->id, 0.0, lp, DOWNSTREAM, BGP_UPDATE);

	hdr = tw_memory_alloc(lp, g_bgp_fd);
	m = tw_memory_data(hdr);

	m->b.type = UPDATE;
	m->b.update = msg->b.update;
	m->b.cause_bgp = m->b.cause_ospf = 0;

	if(cause)
		m->b.cause_ospf = 1;
	else
	{
		m->b.cause_bgp = 1;
		g_bgp_stats.s_cause_bgp++;
	}


	// Having problems with membufs being attached to membufs.. impossible
	// to free them if the pkt is dropped outside the BGP model.
	tw_event_memory_set(e, hdr, g_bgp_fd);
	rn_event_send(e);

	if(e->state.owner == TW_pe_free_q || e == lp->pe->abort_event)
		return;

	// if can be sent, get the hdr back off the event now so the route
	// can be put on first.
	e->memory = NULL;

	// Add the route to the UPDATE message
	if(msg->b.update == 0 || rn_getas(rn_getmachine(n->id)) == state->as)
		b = bgp_route_copy(state, r_send, n, 0, lp);
	else
		b = bgp_route_copy(state, r_send, n, 1, lp);

#if 0
	test = tw_memory_data(b);
	if(TW_FALSE == bgp_route_equals(test, r_send))
		tw_error(TW_LOC, "Invalid ROUTE copy!");
#endif

	if(m->b.update == REMOVE && state->rib[r_send->dst] == NULL)
		m->b.unreachable = 1;
	else
		m->b.unreachable = 0;

	tw_event_memory_set(e, b, g_bgp_fd_rtes);
	tw_event_memory_set(e, hdr, g_bgp_fd);

	// Had to be moved up to before membufs placed onto event.. because
	// event may be aborted, and then we do not want to copy the route
	//rn_event_send(e);

#if VERIFY_BGP
	fprintf(state->log, "\t\tsent UPDATE (%d) to %d at %f: \n\t\t", 
		m->b.update, n->id, tw_now(lp));
	bgp_route_print(state, tw_memory_data(b));
#endif
}

void
bgp_update(bgp_state * state, tw_bf * bf, bgp_message * msg, int src, tw_lp * lp)
{
	tw_memory	*b_in;
	bgp_route	*r;

	int		 cause;
	char		 temp[2048];

 	if(NULL == (b_in = tw_event_memory_get(lp)))
		tw_error(TW_LOC, "No UPDATE on event!");

	r = tw_memory_data(b_in);

	if(!r)
		tw_error(TW_LOC, "Invalid route on UDPATE!");

	sprintf(temp, "%d %d %d", r->src, r->dst, r->next_hop);

	// we're adding a route
	if(msg->b.update)
	{
		update_add(state, bf, b_in, msg, src, lp);
	} else
	{
		update_withdraw(state, bf, r, msg, src, lp);

		if(msg->b.unreachable == 0)
			return;

#if VERIFY_BGP
		fprintf(state->log, "\tgot UNREACHABLE UPDATE from %d, for AS %d\n",
			src, r->dst);
#endif

		if(state->rib[r->dst] != NULL)
		{
			msg->b.update = 1;
	
			if(msg->b.cause_ospf)
				cause = 1;
			else
				cause = 0;

			bgp_update_send(state, bgp_getnbr(state, src), 
					tw_memory_data(state->rib[r->dst]), 
					msg, cause, lp);
			state->stats->s_nunreachable++;
		}

		bgp_route_free(state, b_in, lp);
	}
}

void
update_add(bgp_state * state, tw_bf * bf, tw_memory * b_in, 
			bgp_message * msg, int src, tw_lp * lp)
{
	tw_memory	*b;

	bgp_route	*r_in;
	bgp_route	*r;

	r_in = tw_memory_data(b_in);

#if VERIFY_BGP
	fprintf(state->log, "\tupdate add from %d: AS %d, ", src, r_in->dst);
	bgp_route_aspath_print(state, r_in);
#endif

	if(!r_in)
		tw_error(TW_LOC, "No Route on UPDATE!");

	// check to make sure this route is not to our AS
	if(r_in->dst == state->as->id || bgp_route_inpath(state, r_in, lp))
	{
		bgp_route_free(state, b_in, lp);
		return;
	}

	// If cannot get to route next_hop, do not add!
	if(state->as == rn_getas(rn_getmachine(src)) &&
	   NULL == rn_getlink(state->m, r_in->next_hop) &&
	   -1 == rn_route(state->m, r_in->next_hop))
	{
		bgp_route_free(state, b_in, lp);
		return;
	}

	/*
	 * If I already have a route to this destination, I need to decide
	 * whether to keep the current route, or swap it with the new one!
	 */
	r = NULL;
	if(NULL != (b = bgp_getroute(state, r_in)))
	{
		r = tw_memory_data(b);
		bf->c4 = 1;

		// bgp_decision returns TW_TRUE if I should keep existing route
		if ((bf->c3 = !bgp_decision(state, r_in, r, lp)))
		{
#if VERIFY_BGP
			fprintf(state->log, "\tDecision is to not add route! \n");
#endif
			bgp_route_free(state, b_in, lp);
			return;
		} else
		{
			// if this is an UPDATE from an iBGP rtr, and 
			// we are not the originator, then we cannot
			// remove it!  We are the originator if our
			// id is > the route->src
			if(rn_getas(rn_getmachine(src)) == state->as &&
			   lp->id < r->src)
			{
				bgp_route_free(state, b_in, lp);
				return;
			}

#if 0
			// Supposed to IMPLICITLY remove these routes..
			// meaning, let the UPDATE add msg handle removing
			// these routes elsewhere in the network
			msg->b.update = 0;
			bgp_update_all(state, msg, src, r, lp);
			msg->b.update = 1;
#endif
			// if it was an iBGP message, don't add it to OSPF
			if(state->as != rn_getas(rn_getmachine(src)))
				bgp_ospf_update(state, &g_rn_as[r_in->dst], 
						r_in, TW_FALSE, lp);

			bgp_route_withdraw(state, r);
			bgp_route_free(state, b, lp);
		}
	}

	bgp_route_add(state, b_in);

	// if it was an iBGP message, don't add it to OSPF
	if(state->as != rn_getas(rn_getmachine(src)))
		bgp_ospf_update(state, &g_rn_as[r_in->dst], r_in, TW_TRUE, lp);

	// Delayed now until MRAI timer fires.
	//bgp_update_all(state, msg, src, r_in, lp);
}

void
update_withdraw(bgp_state * state, tw_bf * bf, bgp_route * r_in, 
				bgp_message * msg, int src, tw_lp * lp)
{
	tw_memory	*b;
	bgp_route	*r;

	if(NULL == (b = bgp_getroute(state, r_in)))
		return;

	r = tw_memory_data(b);

#if VERIFY_BGP
	fprintf(state->log, "\tupdate withdraw route: \n\t");
	bgp_route_print(state, r_in);
#endif

	if ((bf->c5 = (TW_TRUE == bgp_route_equals(r_in, r))))
	{
		bgp_route_withdraw(state, r);

		// if it was an iBGP message, don't add it to ospf
		if(state->as != rn_getas(rn_getmachine(src)))
			bgp_ospf_update(state, &g_rn_as[r_in->dst], r_in, 
					TW_FALSE, lp);

		// Do not delay withdraws until MRAI timer fires!
		bgp_update_all(state, msg, src, r_in, lp);

		// Free the route memory buffers
		bgp_route_free(state, b, lp);
	}
}

// Just a helper function to "flood" update to further nodes..
void
bgp_update_all(bgp_state * state, bgp_message * msg, int src, 
					bgp_route * r, tw_lp * lp)
{
	bgp_nbr		*n;
	int	 	 i;

	for(i = 0, n = state->nbr; i < state->n_interfaces; n = &state->nbr[++i])
	{
		if (n->up == TW_FALSE || (src == n->id))
			continue;

		if(r->dst == rn_getas(rn_getmachine(n->id))->id)
			continue;

		// If from iBGP peer, do not send to other iBGP peers
		if(state->as == rn_getas(rn_getmachine(r->src)) &&
		   state->as == rn_getas(rn_getmachine(n->id)))
			continue;

		if(msg->b.cause_bgp == msg->b.cause_ospf)
			tw_error(TW_LOC, "Unmarked UPDATE!");

		if(msg->b.cause_bgp)
			bgp_update_send(state, n, r, msg, 0, lp);
		else
			bgp_update_send(state, n, r, msg, 1, lp);

		//msg->b.rev_num_neighbors++;
	}
}
