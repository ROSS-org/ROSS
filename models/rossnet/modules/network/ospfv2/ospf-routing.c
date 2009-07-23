#include <ospf.h>

void
ospf_rt_build(ospf_state * state, int start_vertex)
{
	if(!state->gstate->rt_interval)
	{
		ospf_rt_timer(state, start_vertex);
		return;
	}

	if(NULL == state->rt_timer)
	{
		state->rt_timer = ospf_timer_start(NULL,
						state->rt_timer,
						state->gstate->rt_interval,
						OSPF_RT_TIMER,
						tw_getlp(start_vertex));

#if VERIFY_ROUTING
		if(state->rt_timer)
			printf("%d: RT Timer set for %lf \n",
				start_vertex, state->rt_timer->recv_ts);
#endif
	}
}

void
ospf_rt_timer(ospf_state * state, int start_vertex)
{
	tw_memory	*b;

	rn_machine	*node;
	rn_machine	*dst;
	rn_area		*dst_ar;

	ospf_db_entry	*dbe;
	ospf_lsa	*lsa;
	ospf_lsa_link	*l;
	ospf_nbr	*nbr;

	int             i, j, q, v, w, low;
	int		nhop = 0;

	/*
	 * d[] - distance to vertices. * front[] - stores the index to vertices
	 * that are in the frontier. * p[] - position of vertices in front[]
	 * array. * f[] - boolean frontier set array. * s[] - boolean solution
	 * set array. * * timing - timing results structure for returning the
	 * time taken. * dist - used in distance computations. * vertices, n -
	 * Graph details: vertices array, size. * out_n - Current vertex's OUT
	 * set size. 
	 */

	int	 dist;
	int	*parent;

	w = 0;
	low = state->ar->low;
	parent = state->m->ft;

#if VERIFY_ROUTING
	printf("%d: Rebuilding routing table \n", start_vertex);
#endif

	if(state->ar->nmachines == 0)
		tw_error(TW_LOC, "No forwarding table for %d!", start_vertex);

	for (i = 0; i < state->ar->nmachines; i++)
	{
		parent[i] = -1;
		f[i] = 0;
		s[i] = 0;
	}

	// The start vertex is part of the solutions set. 
	start_vertex -= low;
	s[start_vertex] = 1;
	d[start_vertex] = 0;

	// j is used as the size of the frontier set. 
	j = 0;

	// Put out set of the starting vertex into the frontier and update the
	// distances to vertices in the out set.  k is the index for the out set.
	for(i = 0; i < state->n_interfaces; i++)
	{
		nbr = &state->nbr[i];

		if(nbr->state != ospf_nbr_full_st)
			continue;

		if(nbr->id >= state->ar->low && nbr->id <= state->ar->high)
		{
			w = nbr->id - low;
		} else if(nbr->ar->id >= state->ar->as->low && 
			nbr->ar->id <= state->ar->as->high)
		{
			w = state->ar->nmachines + nbr->ar->id - nbr->ar->as->low;
		} else
			continue;

		dbe = &state->db[w];

		if(dbe->b.free)
			continue;

		if(f[w])
			continue;

		lsa = getlsa(state->m, dbe->lsa, dbe->b.entry);
		b = lsa->links.head;

		if(!b)
			continue;

		l = tw_memory_data(b);
		d[w] = l->metric;
		state->m->nhop[w] = 0;

		//printf("adding %d (w=%d), dist %d \n", nbr->id, w, l->metric);

		front[j] = w;
		p[w] = j;
		f[w] = 1;
		parent[w] = i;
		j++;
	}

	// At this point we are assuming that all vertices are reachable from
	// the starting vertex and N > 1 so that j > 0.
	while (j > 0)
	{
		// Find the vertex in frontier that has minimum distance. 
		v = front[0];
		for (q = 1; q < j; q++)
			if (d[front[q]] < d[v])
				v = front[q];

		// Move this vertex from the frontier to the solution set.  It's old
		// position in the frontier array becomes occupied by the element
		// previously at the end of the frontier.

		j--;
		front[p[v]] = front[j];
		p[front[j]] = p[v];
		s[v] = 1;
		f[v] = 0;

		// Values in p[v] and front[j] no longer used, so are now meaningless
		// at this point.

		// Update distances to vertices, w, in the out set of v.
		dbe = &state->db[v];
		lsa = getlsa(state->m, dbe->lsa, dbe->b.entry);
		//node = rn_getmachine(v + low);
		node = rn_getmachine(lsa->adv_r);
		state->m->nhop[lsa->adv_r - state->ar->low] = nhop;

		//printf("considering node: %d \n", node->id);

		b = lsa->links.head;
		for (q = 0; q < lsa->links.size; q++, b = b->next)
		{
			l = tw_memory_data(b);
			dst = rn_getmachine(l->dst);
			dst_ar = rn_getarea(dst);

			// all w should be q
			if(l->dst >= state->ar->low && l->dst <= state->ar->high)
				w = l->dst - low;
			else if(dst_ar->id >= state->ar->as->low && 
				dst_ar->id <= state->ar->as->high)
				w = state->ar->nmachines + dst_ar->id - 
					dst_ar->as->low;
			else
				continue; //tw_error(TW_LOC, "Unknown case!");

			// Only update if w is not already in the solution set.
			if (s[w])
				continue;

			// If w is in the frontier the new distance to w 
			// is the minimum of its current distance and the 
			// distance to w via v.
			dist = d[v] + l->metric;
			if (f[w] == 1)
			{
				if (dist < d[w])
				{
					//printf("up: dist to w %d: %d, nhop %d \n", w, dist, nhop);
					d[w] = dist;
					parent[w] = parent[v];
					state->m->nhop[w] = nhop+1;
				}
			} else
			{
				//printf("add: dist to w %d: %d, nhop %d \n", w, dist, nhop);
				parent[w] = parent[v];
				state->m->nhop[w] = nhop++;
				d[w] = dist;
				front[j] = w;
				p[w] = j;
				f[w] = 1;
				j++;
			}
		}
	}

	// Correct the AS external LSA entries in the routing table
	for(w = state->ar->nmachines; w < state->ar->g_ospf_nlsa; w++)
	{
		if(state->db[w].b.free == 0)
		{
			//printf("Attempting to update route to LSA %d \n", w);
			ospf_db_route(state, &state->db[w], tw_getlp(state->m->id));
		}
	}

	//ospf_rt_print(state, tw_getlp(start_vertex + low), stdout);
}

void
ospf_rt_print(ospf_state * state, tw_lp *lp, FILE * log)
{
	rn_machine	*m;
	rn_area		*ar;

	ospf_lsa	*lsa;
	ospf_db_entry	*dbe;

	int	d;
	int	i;
	int	q;

	m = state->m;
	ar = rn_getarea(m);

	fprintf(log, "\nRouting table for %lld: \n\n"
		"\t%-20s %-20s %-20s\n", lp->id, "Host id", 
		"next_hop_interface", "next_hop_node_id");

	for(i = 0; i < g_tw_nlp; i++)
	{
		if(i == lp->id)
		{
			fprintf(log, "\t%-20d %-20s\n", i, "self");
			continue;
		}

		if(rn_route(m, i) == -1)
		{
			// May be that I am the originator of this LSA,
			// which would have an FT of -1, so check LSA
			// Check LSA for route
			rn_area *dst = rn_getarea(rn_getmachine(i));
			dbe = &state->db[i];

			// if in same area, the only use FT
			if(state->ar == dst)
			{
				fprintf(log, "\t%-20d %-20s\n", i, "no route");
				continue;
			} else if(state->ar->as == dst->as)
			{
				dbe = &state->db[state->ar->nmachines + 
						 dst->id - dst->as->low];
			} else
			{
				dbe = &state->db[state->ar->nmachines +
					state->ar->as->nareas + dst->as->id];
			}

			if(dbe->b.free)
			{
				fprintf(log, "\t%-20d %-20s\n", i, "no route");
				continue;
			}

			lsa = getlsa(state->m, dbe->lsa, dbe->b.entry);

			if(!lsa)
			{
				fprintf(log, "\t%-20d %-20s\n", i, "no route (NO LSA)");
				continue;
			} else
				d = rn_route(state->m, lsa->adv_r);

			if(lsa->adv_r == lp->id)
			{
				if(lsa->links.size != 0)
				{
					ospf_lsa_link * l = 
						tw_memory_data(lsa->links.head);
					fprintf(log, "\t%-20d %-20s %-20d\n", 
						i, "ORIGIN", l->dst);
				} else
					fprintf(log, "\t%-20d %-20s \n", 
						i, "ORIGIN");
			} else if(d == -1)
				fprintf(log, "\t%-20d %-20s\n", i, "no route");
			else
				fprintf(log, "\t%-20d %-20d %-25lld\n", 
					i, d,
					state->m->link[d].addr);
			//fprintf(log, "\t%-20d %-20s\n", i, "no route");
		} else
			fprintf(log, "\t%-20d %-20d %-25lld\n", 
					i, rn_route(m, i), 
					state->m->link[rn_route(m, i)].addr);
	}

	q = ar->nmachines;

	fprintf(log, "\n\n\t%-20s %-20s %-20s\n", "Area id", 
		"next_hop_interface", "next_hop_node_id");

	for(i = 0; q < ar->nmachines + ar->as->nareas; q++, i++)
	{
		if(i + state->ar->as->low == state->ar->id)
		{
			fprintf(log, "\t%-20lld %-20s\n", i + ar->as->low, "self");
			continue;
		}

		dbe = &state->db[q];
		if(dbe->b.free)
		{
			d = rn_route(state->m, q);

			if(-1 == d)
				fprintf(log, "\t%-20lld %-20s\n", 
					i + ar->as->low, "no route");
			else
				fprintf(log, "\t%-20lld %-20d %-15lld (no lsa)\n", 
					i + ar->as->low, state->m->ft[q],
					state->m->link[state->m->ft[q]].addr);
			continue;
		}
		
		// Check LSA for route
		lsa = getlsa(state->m, dbe->lsa, dbe->b.entry);
		d = rn_route(state->m, lsa->adv_r);

		if(lsa->adv_r == lp->id)
		{
			if(lsa->links.size)
			{
				ospf_lsa_link * l = tw_memory_data(lsa->links.head);
				fprintf(log, "\t%-20lld %-20s %-20d\n", 
					i + ar->as->low, "ORIGIN", l->dst);
			} else
				fprintf(log, "\t%-20lld %-20s\n", 
					i + ar->as->low, "ORIGIN");
		} else if(d == -1)
			fprintf(log, "\t%-20d %-20s\n", i, "no route");
		else
			fprintf(log, "\t%-20lld %-20d %-25lld\n", 
				i + ar->as->low, state->m->ft[q],
				state->m->link[state->m->ft[q]].addr);
	}

	fprintf(log, "\n\n\t%-20s %-20s %-20s\n", "AS id", 
		"next_hop_interface", "next_hop_node_id");

	for(i = 0; q < ar->nmachines + ar->as->nareas + g_rn_nas; q++, i++)
	{
		if(i == state->ar->as->id)
		{
			fprintf(log, "\t%-20d %-20s\n", i, "self");
			continue;
		}

		// Check LSA for route
		dbe = &state->db[q];
		if(dbe->b.free)
		{
			fprintf(log, "\t%-20d %-20s\n", i, "no route");
			continue;
		}
		
		lsa = getlsa(state->m, dbe->lsa, dbe->b.entry);
		d = rn_route(state->m, lsa->adv_r);

		if(lsa->adv_r == lp->id)
		{
			if(lsa->links.size)
			{
				ospf_lsa_link * l = tw_memory_data(lsa->links.head);
				fprintf(log, "\t%-20d %-20s %-20d\n", 
						i, "ORIGIN", l->dst);
			} else
				fprintf(log, "\t%-20d %-20s\n", i, "ORIGIN");

		} else if(d == -1)
			fprintf(log, "\t%-20d %-20s\n", i, "no route");
		else
			fprintf(log, "\t%-20d %-20d %-25lld\n", i, state->m->ft[q],
				state->m->link[state->m->ft[q]].addr);
	}
}

void
ospf_routing_init(ospf_state * state, tw_lp * lp)
{
	ospf_nbr	*nbr;

	int		 i;

	// Init globals for Dijkstra's algorithm
	if(!front)
	{
		//int n = state->ar->g_ospf_nlsa;
		int n = g_rn_nrouters;

		front = calloc(n * sizeof(int), 1);
		p = calloc(n * sizeof(int), 1); 
		f = calloc(n * sizeof(int), 1); 
		s = calloc(n * sizeof(int), 1); 
		d = calloc(n * sizeof(int), 1); 
	}

	if(!state->m->nhop)
		state->m->nhop = tw_calloc(TW_LOC, "", sizeof(int), state->ar->g_ospf_nlsa);

	if(0 == g_ospf_rt_write)
	{
		rn_machine	*m = rn_getmachine(lp->id);
		int		 i;

		for(i = 0; i < state->ar->g_ospf_nlsa; i++)
			fscanf(g_ospf_routing_file, "%d", &m->ft[i]);

		//read(g_ospf_routing_file, rn_getmachine(lp->id)->ft, 
		//	state->ar->g_ospf_nlsa * sizeof(int));
	} else
	{
		ospf_rt_timer(state, lp->id);
#if 0

		write(g_ospf_routing_file, rn_getmachine(lp->id)->ft, 
			(state->ar->g_ospf_nlsa) * sizeof(int));
#endif
	}

	return;

	/*
	 * Take down adjacencies where there is no forwarding table entry
	 */
	for(i = 0, nbr = state->nbr; i < state->n_interfaces; nbr = &state->nbr[++i])
	{
		if(rn_route(state->m, nbr->id) == -1)
		{
			printf("%lld: taking down neighbor %d \n", lp->id, nbr->id);

			ospf_nbr_event_handler(state, nbr, ospf_nbr_kill_nbr_ev, lp);
#if 0
			// These are just OSPF AS X to OSPF AS Y nbrs
			// we are taking down!

			// setup just what I need!
			nbr->state = ospf_nbr_full_st;
			nbr->requests = (char *) calloc(state->ar->g_ospf_nlsa, 1);

			if(!nbr->requests)
				tw_error(TW_LOC, "Out of memory!");
#endif
		}
	}
}
