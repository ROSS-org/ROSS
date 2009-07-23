#include <bgp.h>

// check to make sure we're not in the as path
int
bgp_route_inpath(bgp_state * state, bgp_route * r_in, tw_lp * lp)
{
	tw_memory	*b;
	int		*hop;
	int		 i;

	//for (b = r_in->as_path.head; b; b = b->next)
	for(i = 0, b = r_in->as_path.head; i < r_in->as_path.size; i++, b = b->next)
	{
		hop = tw_memory_data(b);
		if(*hop == lp->id)
		{
#if VERIFY_BGP
			fprintf(state->log, "\tfound myself in AS path: %ld\n",
				lp->id);
#endif

			return TW_TRUE;
		}
	}

	return TW_FALSE;
}

void
bgp_route_aspath_print(bgp_state * state, bgp_route * r)
{
#if VERIFY_BGP || !BGP_CONVERGED
	tw_memory	*b;

	fprintf(state->log, " %ld  ", r->as_path.size);
	for (b = r->as_path.head; b != NULL; b = b->next)
		fprintf(state->log, "%d ", * (int *) tw_memory_data(b));
	fprintf(state->log, "\n");
#endif
}

tw_memory	*
bgp_getroute(bgp_state * state, bgp_route * find)
{
	if(NULL != state->rib[find->dst])
		return state->rib[find->dst];

	return NULL;
}

int
bgp_route_free(bgp_state * state, tw_memory * route, tw_lp * lp)
{
	tw_memory	*b;
	bgp_route	*r;
	int		 rv;

	r = tw_memory_data(route);
	rv = r->as_path.size;

	while((b = tw_memoryq_pop(&r->as_path)))
		tw_memory_free(lp, b, g_bgp_fd_asp);

	if(r->as_path.head || r->as_path.tail)
		tw_error(TW_LOC, "AS Path not empty!");

	tw_memory_free(lp, route, g_bgp_fd_rtes);

	return rv;
}

void
bgp_route_add(bgp_state * state, tw_memory * b)
{
	rn_link		*l;
	rn_area		*ar;

	bgp_route	*r;

	int		 offset;
	int		 nhi;
	int		 dst;
	int		 i;

	ar = rn_getarea(state->m);
	r = tw_memory_data(b);

	nhi = -1;
	offset = ar->nmachines + ar->as->nareas;

	if(state->rib[r->dst] != NULL)
		tw_error(TW_LOC, "Over-writing route for AS %d!", r->dst);

	state->rib[r->dst] = b;

	// reset the MRAI timer as needed
	b->bit = TW_TRUE;
	bgp_mrai_tmr(state, tw_getlp(state->m->id));

	state->stats->s_nroute_adds++;

	// Self-originated route
	if(r->src == state->m->id)
		return;

	l = rn_getlink(state->m, r->src);

	// eBGP route being added, so must have interface to node
	// (could be iBGP w/o OSPF network in-between also)
	if(l)
	{
		for(i = 0; i < state->m->nlinks; i++)
			if(state->m->link[i].addr == r->src)
			{
				nhi = i;
				break;
			}

		dst = r->src;
		offset += rn_getas(rn_getmachine(r->src))->id;
	} else // must be iBGP route being added, so must get route to node
	{
		dst = r->src;
		nhi = rn_route(state->m, r->src);
		offset += r->dst;
	}

	rn_route_change(state->m, offset, nhi);

#if VERIFY_BGP
	fprintf(state->log, "\tadding route: ft[%d]=%d \n\t", 
		offset, rn_route(state->m, dst));
	bgp_route_print(state, r);
#endif
}

void
bgp_route_withdraw(bgp_state * state, bgp_route * r)
{
	rn_area		*ar;

	ar = rn_getarea(state->m);

	state->rib[r->dst] = NULL;
	rn_route_change(state->m, ar->nmachines + ar->as->nareas + r->dst, -1);
	state->stats->s_nroute_removes++;

#if VERIFY_BGP
	fprintf(state->log, "\twithdrawing route: ft[%d]=%d \n\t",
		ar->nmachines + ar->as->nareas + r->dst, 
		rn_route(state->m, ar->nmachines + ar->as->nareas + r->dst));
	bgp_route_print(state, r);
#endif
}

tw_memory	*
bgp_route_new(bgp_state * state, tw_lp * src, tw_lp * dst, tw_lp * lp)
{
	tw_memory	*b;

	bgp_route	*r;

	b = tw_memory_alloc(lp, g_bgp_fd_rtes);
	r = tw_memory_data(b);

	r->src = src->id;
	r->dst = rn_getas(rn_getmachine(dst->id))->id;
	r->next_hop = state->as->id;
	r->origin = 3;
	r->med = 0xffff;

	return b;
}

int
bgp_route_equals(bgp_route * r1, bgp_route * r2)
{
	tw_memory	*b1;
	tw_memory	*b2;

	int		*hop1;
	int		*hop2;

	if (r1->dst != r2->dst)
		return TW_FALSE;

	if (r1->next_hop != r2->next_hop)
		return TW_FALSE;

	if (r1->as_path.size != r2->as_path.size)
		return TW_FALSE;

	if (r1->med != r2->med)
		return TW_FALSE;

	// Since AS path order is preserved, we should have 
	// a one to one mapping between route AS paths
	b1 = r1->as_path.head;
	b2 = r2->as_path.head;
	for(; b1 && b2; b1 = b1->next, b2 = b2->next)
	{
		hop1 = tw_memory_data(b1);
		hop2 = tw_memory_data(b2);

		if(*hop1 != *hop2)
			return TW_FALSE;
	}

	return TW_TRUE;
}

/*
 * We copy the route, add ourselves to the AS path, and send it in an
 * UPDATE to our neighbors.
 *
 * Need to check global route list for this AS destination for existing
 * route.  If we find it, then return that route rather than the newly
 * created one.
 */
tw_memory	*
bgp_route_copy(bgp_state * state, bgp_route * to_be_copied, bgp_nbr * n, 
				bgp_update_type t, tw_lp * lp)
{
	rn_as		*as;
	rn_as		*d_as;

	tw_memory	*new_route;
	tw_memory	*b;
	tw_memory	*b1;

	bgp_nbr		*nbr;
	bgp_route	*r;

	int		*hop;

	int		 last;
	int		 size;
	int		 i;

	as = rn_getas(rn_getmachine(lp->id));
	d_as = rn_getas(rn_getmachine(n->id));

	new_route = tw_memory_alloc(lp, g_bgp_fd_rtes);
	new_route->bit = 0;
	r = tw_memory_data(new_route);

	// If IP drops pkt, it does not know to free these bufs
	if(r->as_path.head || r->as_path.tail)
	{
		bgp_route_free(state, new_route, lp);
		new_route = tw_memory_alloc(lp, g_bgp_fd_rtes);

		if(r->as_path.head || r->as_path.tail)
			tw_error(TW_LOC, "STILL!");
	}

	/*
	 5.1.4   MULTI_EXIT_DISC

		The MULTI_EXIT_DISC attribute may be used on external (inter-AS)
		links to discriminate among multiple exit or entry points to the same
		neighboring AS.  The value of the MULTI_EXIT_DISC attribute is a four
		octet unsigned number which is called a metric.  All other factors
		being equal, the exit or entry point with lower metric should be
		preferred.  If received over external links, the MULTI_EXIT_DISC
		attribute may be propagated over internal links to other BGP speakers
		within the same AS.  The MULTI_EXIT_DISC attribute is never
		propagated to other BGP speakers in neighboring AS's.

		Cisco MED example: http://www.cisco.com/warp/public/459/37.html
	 */
	if(d_as == as)
	{
		r->med = to_be_copied->med;
	} else if(to_be_copied->src == lp->id)
	{
		// Should only be executed when we are creating route for
		// the first time, which should only be once per connection
		for(size = 0, i = 0; i < state->n_interfaces; i++)
		{
			nbr = &state->nbr[i];
			if(rn_getas(rn_getmachine(nbr->id))->id == d_as->id)
				size++;
		}

		if(size > 1)
			r->med = g_bgp_as[as->id].med;
		else
			r->med = 0xffff;
	} else
		r->med = 0xffff;

	r->src = lp->id;
	r->dst = to_be_copied->dst;
	
	if(d_as != state->as)
		r->origin = 2;
	else
		r->origin = 1;

	// Copy the AS path
	// have to preserve the AS path order, so go backwards through list!
	size = to_be_copied->as_path.size;
	last = -1;
	for(i = 0, b = to_be_copied->as_path.tail; i < size; i++, b = b->prev)
	{
		b1 = tw_memory_alloc(lp, g_bgp_fd_asp);

		hop = tw_memory_data(b1);
		*hop = *(int *) tw_memory_data(b);
		last = *hop;
	
		tw_memoryq_push(&r->as_path, b1);
	}

	if(d_as == as)
	{
		r->next_hop = to_be_copied->next_hop;

		if(rn_getlink(state->m, n->id))
			r->next_hop = n->id;
		else
			r->next_hop = state->m->link[rn_route(state->m, n->id)].addr;
	} else
	{
		r->next_hop = as->id;

		// Add myself to the AS path only on UPDATE adds
		if(t == ADD)
		{
/*
			printf("Padding by %d for AS %d \n", 
				g_bgp_as[as->id].path_padding, as->id);
*/
			for(i = 0; i < g_bgp_as[as->id].path_padding; i++)
			{
				b1 = tw_memory_alloc(lp, g_bgp_fd_asp);
				hop = tw_memory_data(b1);
				*hop = lp->id;

				tw_memoryq_push(&r->as_path, b1);
			}
		}
	}

	return new_route;
}

void
bgp_route_print(bgp_state * state, bgp_route * r)
{
#if VERIFY_BGP || !BGP_CONVERGED
	fprintf(state->log, "%-10hd%-10hd%-10d%-10hd%-10hd", 
		r->dst, r->next_hop, r->src, r->med, r->origin);
	bgp_route_aspath_print(state, r);
#endif
}

void
bgp_rib_print(bgp_state * state, tw_lp * lp)
{
#if VERIFY_BGP
	tw_memory	*b;

	rn_as		*as;
	rn_machine	*m;

	bgp_route	*r;

	int		 offset;
	int		 i;

	m = rn_getmachine(lp->id);
	as = rn_getas(m);

	fprintf(state->log, "\nRouting Information Base (RIB): Node %ld, AS %d\n", 
		lp->id, as->id);
	fprintf(state->log, "%-10s%-10s%-10s%-10s%-10s%-10s\n", 
		"Dest", "NHI", "NHI Node", "MED", "ORIGIN", " #  AS Path");

	offset = m->subnet->area->nmachines + as->nareas;

	for(i = 0; i < g_rn_nas; i++)
	{
		if(NULL == (b = state->rib[i]))
			continue;

		r = tw_memory_data(b);

		if(r->dst == as->id)
		{
			bgp_route_print(state, r);
			fprintf(state->log, "\n");
		} else
		{
			bgp_route_print(state, r);
			fprintf(state->log, "\n");
		}
	}
#endif
}
