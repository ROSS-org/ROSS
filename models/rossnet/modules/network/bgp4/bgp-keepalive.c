#include <bgp.h>

/*
 * Reset the keepalive timer
 */
void
bgp_keepalive_tmr(bgp_state * state, tw_lp * lp)
{
	if(0 == state->as->hold_interval)
		return;

	// keep the keepalive timer going
	state->tmr_keep_alive = bgp_timer_start(lp,
						state->tmr_keep_alive,
						state->as->keepalive_interval,
						KEEPALIVETIMER);
}

void
bgp_keepalive_send(bgp_nbr * n, tw_lp * lp)
{
	tw_event       *e;
	tw_memory      *hdr;

	bgp_message    *m;

	e = rn_event_new(n->id, 0.0, lp, DOWNSTREAM, BGP_KEEPALIVE);
		return;

	// create the BGP header
	hdr = tw_memory_alloc(lp, g_bgp_fd);
	m = tw_memory_data(hdr);

	m->b.type = KEEPALIVE;

	tw_event_memory_set(e, hdr, g_bgp_fd);
	rn_event_send(e);
}

// Any message received is treated as a KEEPALIVE message
void
bgp_keepalive(bgp_state * state, int src, int ttl, tw_lp * lp)
{
	rn_area		*d_ar;
	rn_area		*ar;

	bgp_route	*r;
	bgp_nbr		*n;

	int		 x_id;
	int		 i;

	n = bgp_getnbr(state, src);

	if(NULL == n)
	{
		printf("%lld: adding neighbor: %d \n", lp->id, src);
		n = &state->nbr[state->n_interfaces++];
		n->id = src;
		n->up = TW_FALSE;
	}

	ar = rn_getarea(state->m);
	d_ar = rn_getarea(rn_getmachine(n->id));

	if(ar == d_ar)
		x_id = n->id - ar->low;
	else if(state->as == d_ar->as)
		x_id = ar->nmachines + d_ar->id - state->as->low;
	else
		x_id = -1;

	if(-1 != x_id && x_id > ar->g_ospf_nlsa)
		tw_error(TW_LOC, "x_id out of range!");

	// Get the latest hop count to the neighbor off this message
	if(g_bgp_state->b.hot_potato && 
		state->as == d_ar->as && x_id != -1 && 
		n->hop_count != state->m->nhop[x_id])
	{
		n->hop_count = state->m->nhop[x_id];

		for(i = 0; i < g_rn_nas; i++)
		{
			if(!state->rib[i])
				continue;

			r = tw_memory_data(state->rib[i]);
			if(r->src == n->id)
			{
				r->bit = TW_TRUE;
				bgp_mrai_tmr(state, lp);
			}
		}
	}

	// If we receive a message from a down host, start the OPEN process
	if(n->up == TW_FALSE)
	{
		bgp_open_send(state, n, lp);
		n->up = TW_TRUE;

#if VERIFY_BGP
		fprintf(state->log, "\tbringing up nbr %d at %lf\n", 
			src, tw_now(lp));
#endif
	}

	n->last_update = floor(tw_now(lp));
}

void
bgp_keepalive_timer(bgp_state * state, tw_bf * bf, bgp_message * msg, tw_lp * lp)
{
	tw_event       *e;
	tw_memory      *hdr;

	bgp_message    *m;
	bgp_nbr        *n;

	unsigned int    i;

	state->stats->s_nkeepalivetimers++;

	// send a keepalive to each neighbor
	for (i = 0, n = state->nbr; i < state->n_interfaces; n = &state->nbr[++i])
	{
		// If the neighbor is down, attempt to send OPEN instead..
		// idea is to simulate transport connection layer attempt
		if (n->up == TW_FALSE)
		{
			bgp_open_send(state, n, lp);
			continue;
		}

		e = rn_event_new(n->id, 0.0, lp, DOWNSTREAM, BGP_KEEPALIVE);

		hdr = tw_memory_alloc(lp, g_bgp_fd);
		m = tw_memory_data(hdr);
		m->b.type = KEEPALIVE;

		tw_event_memory_set(e, hdr, g_bgp_fd);
		rn_event_send(e);

#if VERIFY_BGP
		fprintf(state->log, "\tsent KEEPALIVE msg to %d at %lf \n",
			n->id, e->recv_ts);
#endif
	}

	bgp_keepalive_tmr(state, lp);
}

// kill all routers with tw_now - last_update > state-hold_interval
void
deal_with_timed_out_routers(bgp_state * state, tw_bf * bf, bgp_message * msg,
							tw_lp * lp)
{
	tw_stime        now;

	tw_memory      *b;
	tw_memory      *r_next;

	bgp_route      *r;
	bgp_nbr        *n;
	bgp_nbr        *n1;

	int		cause;
	int             found;
	int             i;
	int             j;

	now = floor(tw_now(lp));
	//msg->rev_num_neighbors = 0;
	//msg->rc_asp = 0;

	for (i = 0, n = state->nbr; i < state->n_interfaces; n = &state->nbr[++i])
	{
		if (!n->up || ((now - n->last_update) <= state->as->hold_interval))
			continue;

#if VERIFY_BGP
		fprintf(state->log, "\tnbr %d timed out: %lf > %lf hold interval\n", 
			n->id, (now - n->last_update), state->as->hold_interval);
#endif

		n->up = TW_FALSE;
		found = TW_FALSE;

		// If nbr timed out due to failed eBGP link, then the
		// cause of the UPDATES is BGP.
		//
		// If the nbr timed out due to a failed iBGP path to the
		// nbr, then the cause of the UPDATES is the OSPF network.
		if(rn_getas(rn_getmachine(n->id)) != state->as)
			cause = 0;
		else
			cause = 1;

		//bgp_notify_send(state, n, lp);

		// Delete any routes originated by this timed out router
		for(i = 0; i < g_rn_nas; i++)
		{
			if(NULL == (b = state->rib[i]))
				continue;

			r = tw_memory_data(b);
			r_next = b->next;

			if (n->id != r->src)
				continue;

			for (j = 0; j < state->n_interfaces; j++)
			{
				n1 = &state->nbr[j];

				if (n1->up != TW_TRUE)
					continue;

				//msg->rev_num_neighbors++;

				msg->b.update = 0;
				bgp_update_send(state, n1, r, msg, 1, lp);
			}

			bgp_route_withdraw(state, r);
			bgp_route_free(state, b, lp);

			// Don't do for iBGP connections
			if (rn_getas(rn_getmachine(r->src)) != state->as)
				bgp_ospf_update(state, 
						rn_getas(rn_getmachine(r->dst)), 
						r, TW_FALSE, lp);
		}
	}
}
