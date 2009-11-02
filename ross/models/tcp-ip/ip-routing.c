#include <ip.h>

#define IP_RT_DEBUG 0

int
routing_read()
{
	g_ip_routing_file = fopen(g_rn_rt_table, "r");

	if(!g_ip_routing_file)
		return 0;

	printf("Reading routing table file: %s\n", g_rn_rt_table);

	return 1;
}

void
routing_write()
{
	char	 dir[255];

	sprintf(dir, "%s/ip.rt", g_rn_logs_dir);
	g_ip_routing_file = fopen(dir, "w");

	if(!g_ip_routing_file)
		tw_error(TW_LOC, "Unable to create routing table file: %s", dir);

	g_ip_write = 1;
	printf("Creating routing table file: %s\n", dir);
}

/*
 * Create global structures needed to read/write forwarding tables
 * Note: still sequential at this point, called from ip_main
 */
void
ip_routing_init()
{
	if(!routing_read())
		routing_write();
}

void
routing_init_ft_read(ip_state * state, tw_lp * lp)
{
}

void
ip_rt_print(ip_state * state, tw_lp *lp, FILE * log)
{
	rn_machine	*m;
	rn_area		*ar;

	int	i;
	int	q;

	m = rn_getmachine(lp->id);
	ar = rn_getarea(m);

	fprintf(log, "\nRouting table for %ld: \n\n"
		"\t%-20s %-20s %-20s\n", lp->id, "Host id", 
		"next_hop_interface", "next_hop_node_id");

	for(i = 0; i < g_tw_nlp; i++)
	{
		if(i == lp->id)
		{
			fprintf(log, "\t%-20d %-20s %-20s\n", i, "self", "-");
			continue;
		}

		if(rn_route(m, i) == -1)
		{
			rn_area *dst = rn_getarea(rn_getmachine(i));

			// if in same area, then only use FT
			if(ar == dst)
			{
				fprintf(log, "\t%-20d %-20s\n", i, "no route");
			} else if(ar->as == dst->as)
			{
				fprintf(log, "\t%-20d %-20s\n", 
					i, "diff ar, my as, no route");
			} else
			{
				fprintf(log, "\t%-20d %-20s\n", 
					i, "not my ar or as, no route");
			}
		} else
		{
			fprintf(log, "\t%-20d %-20d %-25d\n", 
					i, rn_route(m, i), 
					m->link[rn_route(m, i)].addr);
		}
	}

	q = ar->nmachines;

	fprintf(log, "\n\n\t%-20s %-20s %-20s\n", "Area id", 
		"next_hop_interface", "next_hop_node_id");

	for(i = 0; q < ar->nmachines + ar->as->nareas; q++, i++)
	{
		if(i + ar->as->low == ar->id)
		{
			fprintf(log, "\t%-20ld %-20s\n", i + ar->as->low, "self");
		} else if(m->ft[q] == -1)
		{
			fprintf(log, "\t%-20ld %-20s\n", i + ar->as->low, "no route");
		} else
		{
			fprintf(log, "\t%-20ld %-20d %-25d\n", 
				i + ar->as->low, m->ft[q],
				m->link[m->ft[q]].addr);
		}
	}

	fprintf(log, "\n\n\t%-20s %-20s %-20s\n", "AS id", 
		"next_hop_interface", "next_hop_node_id");

	for(i = 0; q < ar->nmachines + ar->as->nareas + g_rn_nas; q++, i++)
	{
		if(i == ar->as->id)
		{
			fprintf(log, "\t%-20d %-20s\n", i, "self");
			continue;
		} else if(m->ft[q] == -1)
		{
			fprintf(log, "\t%-20d %-20s\n", i, "no route");
		} else
		{
			fprintf(log, "\t%-20d %-20d %-25d\n", i, m->ft[q],
				m->link[m->ft[q]].addr);
		}
	}
}

void
routing_init_ft_write(ip_state * state, tw_lp * lp)
{
	rn_machine	*m = rn_getmachine(lp->id);
	rn_area		*ar = rn_getarea(m);
	rn_machine	*node;
	rn_machine	*dst;
	rn_area		*dst_ar;
	rn_link		*l;
	rn_link		*link;

	int		start_vertex;
	int             i, j, q, v, w, low;
	int		nhop = 0;

	int frontier[ar->g_ospf_nlsa];
	int p[ar->g_ospf_nlsa];
	int f[ar->g_ospf_nlsa];
	int solution[ar->g_ospf_nlsa];
	int d[ar->g_ospf_nlsa];

	/*
	 * d[] - distance to vertices. * frontier[] - stores the index to vertices
	 * that are in the frontier. * p[] - position of vertices in frontier[]
	 * array. * f[] - boolean frontier set array. * solution[] - boolean solution
	 * set array. * * timing - timing results structure for returning the
	 * time taken. * dist - used in distance computations. * vertices, n -
	 * Graph details: vertices array, size. * out_n - Current vertex's OUT
	 * set size. 
	 */

	int		 dist;
	short int	*parent;

	if(m->type != c_router)
		return;

	w = 0;
	start_vertex = (int) lp->id - ar->low;
	low = ar->low;
	parent = m->ft;

	if(ar->g_ospf_nlsa == 0)
		tw_error(TW_LOC, "No forwarding table for %ld!", start_vertex);

	m->nhop = tw_vector_create(sizeof(short), ar->g_ospf_nlsa);

#if VERIFY_ROUTING
	printf("\n%hd: Rebuilding routing table \n", start_vertex);
#endif

	for (i = 0; i < ar->g_ospf_nlsa; i++)
	{
		parent[i] = -1;
		f[i] = 0;
		solution[i] = 0;
	}

	// The start vertex is part of the solutions set. 
	solution[start_vertex] = 1;
	d[start_vertex] = 0;

#if IP_RT_DEBUG
	printf("%ld: updating frontier \n", lp->id);
#endif

	// Put out set of the starting vertex into the frontier and update the
	// distances to vertices in the out set.  k is the index for the out set.
	// j is used as the size of the frontier set. 
	for(i = 0, j = 0; i < m->nlinks; i++)
	{
		link = &m->link[i];

		if(link->status == rn_link_down)
			continue;

		if(link->wire->type != c_router)
			continue;

		dst_ar = rn_getarea(link->wire);

		if(link->addr >= ar->low && link->addr <= ar->high)
		{
			w = link->addr - low;
		} else if(dst_ar->id >= ar->as->low && 
			  dst_ar->id <= ar->as->high)
		{
			w = ar->nmachines + rn_getarea(link->wire)->id - rn_getas(link->wire)->low;
		} else
			w = ar->nmachines + ar->as->nareas + rn_getas(link->wire)->id;

		if(f[w])
			continue;

		d[w] = link->cost;
		m->nhop[w] = 0;

#if IP_RT_DEBUG
		printf("\tadding %d (w=%d), dist %d \n", link->addr, w, link->cost);
#endif

		frontier[j] = w;
		p[w] = j;
		f[w] = 1;
		parent[w] = i;
		j++;
	}

	if(j >= ar->g_ospf_nlsa)
		tw_error(TW_LOC, "Mem leak on frontier array?");

#if IP_RT_DEBUG
	printf("%ld: step 2 (j=%d)\n", lp->id, j);
#endif

	// At this point we are assuming that all vertices are reachable from
	// the starting vertex and N > 1 so that j > 0.
	while (j > 0)
	{
		// Find the vertex in frontier that has minimum distance. 
		v = frontier[0];
		for (q = 1; q < j; q++)
			if (d[frontier[q]] < d[v])
				v = frontier[q];

		// Move this vertex from the frontier to the solution set.  It's old
		// position in the frontier array becomes occupied by the element
		// previously at the end of the frontier.

		j--;

		frontier[p[v]] = frontier[j];
		p[frontier[j]] = p[v];
		solution[v] = 1;
		f[v] = 0;

		// Update distances to vertices, w, in the out set of v.
		node = rn_getmachine(v + ar->low);
		m->nhop[v] = nhop;

		if(node->type != c_router)
			continue;

		//dbe = &state->db[v];
		//lsa = getlsa(state->m, dbe->lsa, dbe->b.entry);
		//node = rn_getmachine(lsa->adv_r);
		//m->nhop[lsa->adv_r - ar->low] = nhop;

#if IP_RT_DEBUG
		printf("\tconsidering node: %ld \n", node->id);
#endif

		//b = lsa->links.head;
		//for (q = 0; q < lsa->links.size; q++, b = b->next)
		for(q = 0, l = node->link; q < node->nlinks; l = &node->link[++q])
		{
			//l = tw_memory_data(b);
			//dst = rn_getmachine(l->dst);
			//dst_ar = rn_getarea(dst);

			dst = l->wire;
			dst_ar = rn_getarea(dst);

			// all w should be q
			if(dst->id >= ar->low && dst->id <= ar->high)
				w = dst->id - low;
			else if(dst_ar->id >= ar->as->low && dst_ar->id <= ar->as->high)
				w = ar->nmachines + dst_ar->id - dst_ar->as->low;
			else
				w = ar->nmachines + ar->as->nareas + dst_ar->as->id;

			// Only update if w is not already in the solution set.
			if (solution[w])
				continue;

			// If w is in the frontier the new distance to w 
			// is the minimum of its current distance and the 
			// distance to w via v.
			dist = d[v] + l->cost;
			if(f[w] == 1)
			{
				if (dist < d[w])
				{
#if IP_RT_DEBUG
					printf("\t\tup: dist to %ld (w=%d): %d, nhop %d \n", 
						node->id, w, dist, nhop);
#endif
					d[w] = dist;
					parent[w] = parent[v];
					m->nhop[w] = nhop+1;
				}
			} else
			{
#if IP_RT_DEBUG
				printf("\t\tadd: dist to %d %d, nhop %d \n", w, dist, nhop);
#endif

				parent[w] = parent[v];
				m->nhop[w] = nhop++;
				d[w] = dist;
				frontier[j] = w;
				p[w] = j;
				f[w] = 1;
				j++;
			}
		}
	}

#if 0
	// Correct the AS external LSA entries in the routing table
	for(w = ar->nmachines; w < ar->g_ospf_nlsa; w++)
	{
		//printf("Attempting to update route to LSA %d \n", w);
		//ospf_db_route(state, &state->db[w], tw_getlp(m->id));
	}
#endif

	if(0 && lp->id == 930)
		ip_rt_print(state, lp, stdout);
}

/*
 * Construct the forwarding table for this simple ip router
 * Note: parallel at this point, called from ip_init
 *	 (not going to be able to read/write in this context)
 */
void
ip_routing_init_ft(ip_state * state, tw_lp * lp)
{
	if(g_ip_write)
		routing_init_ft_write(state, lp);
	else
		routing_init_ft_read(state, lp);
}
