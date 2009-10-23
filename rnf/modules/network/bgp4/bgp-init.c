#include <bgp.h>

void
init_rib(bgp_state *state, tw_lp * lp)
{
	tw_memoryq	*q;
	tw_memory	*b;

	bgp_route	*r;

	int		*h;
	int		 i;
	int		 j;
	int		 as_path_size;

	unsigned short	 dst;

	if(!g_rn_converge_bgp && g_rn_converge_ospf)
		tw_error(TW_LOC, "should not be here!");

#if VERIFY_BGP || 1
	if(!state->log)
		state->log = stdout;
#endif

	//printf("Loading RIB for router: %ld \n", lp->id);

	for(i = 0; i < g_rn_nas; i++)
	{
		dst = -1;
		fscanf(g_bgp_rt_fd, "%hd ", &dst);

		state->rib[i] = tw_memory_alloc(lp, g_bgp_fd_rtes);
		r = tw_memory_data(state->rib[i]);
		r->bit = TW_FALSE;

		q = &r->as_path;
		q->size = as_path_size = 0;
		fscanf(g_bgp_rt_fd, "%hd %d %hd %hd %d", 
			&r->next_hop, &r->src, 
			&r->med, &r->origin, &as_path_size);

		r->dst = dst;

		for(j = 0; j < as_path_size; j++)
		{
			b = tw_memory_alloc(lp, g_bgp_fd_asp);
			h = tw_memory_data(b);

			fscanf(g_bgp_rt_fd, "%d ", h);

			q->size++;
			b->next = b->prev = NULL;

			if(q->tail)
			{
				q->tail->next = b;
				b->prev = q->tail;
				q->tail = b;
			} else
				q->head = q->tail = b;

			if(b == b->next)
				tw_error(TW_LOC, "here!");
		}
	}
}

void
bgp_init(bgp_state * state, tw_lp * lp)
{
	tw_stime	 keepalive_interval;

	bgp_nbr		*n;

	int		 i;

#if BGP_CONVERGED
	rn_area		*ar;
	rn_area		*d_ar;

	int		 x_id;
#endif


#if VERIFY_BGP
	char		 name[255];

	sprintf(name, "%s/router-%ld.log", g_rn_logs_dir, lp->id);

	if(lp->log)
		state->log = lp->log;
	else
		state->log = stdout;
		//lp->log = state->log = fopen(name, "w");
#else
	state->log = stdout;
#endif

	state->m = rn_getmachine(lp->id);
	state->as = rn_getas(state->m);

	state->stats = (bgp_stats *) calloc(sizeof(bgp_stats), 1);

	if(!state->stats)
		tw_error(TW_LOC, "Out of memory!");

	state->rib = (tw_memory	**) calloc(sizeof(tw_memory *), g_rn_nas);

	if(!state->rib)
		tw_error(TW_LOC, "Out of memory!");

#if 0
	// connect all area tree roots
	if(0 && state->m->subnet->area->root == lp->id)
	{
#if VERIFY_BGP && 0
		printf("%ld: I am root for area: %d, lvl %d \n", 
			lp->id, state->m->subnet->area->id, state->m->level);
#endif

		for(i = 0; i < state->as->nareas; i++)
		{
			if(g_rn_areas[i+state->as->low].root == INT_MAX ||
			   g_rn_areas[i+state->as->low].root == lp->id)
				continue;

			n = &state->nbr[state->n_interfaces++];
			n->id = g_rn_areas[i+state->as->low].root;
			n->up = TW_FALSE;
		}
	}
#endif

	if(g_rn_converge_ospf)
		return;

	// start the keepalive timer
	if(0 && state->as->hold_interval != 0.0)
	{
		keepalive_interval = tw_rand_exponential(lp->rng, 
					state->as->keepalive_interval);
		state->tmr_keep_alive = bgp_timer_start(lp,
						state->tmr_keep_alive,
						keepalive_interval,
						KEEPALIVETIMER);
	}

#if BGP_CONVERGED
	for(i = 0; i < state->n_interfaces; i++)
	{
		n = &state->nbr[i];
		n->last_update = 0;

		if(-1 != rn_route(state->m, n->id) || rn_getlink(state->m, n->id))
			n->up = TW_TRUE;
		else
			n->up = TW_FALSE;

		ar = rn_getarea(state->m);
		d_ar = rn_getarea(rn_getmachine(n->id));

		// x_id is 0 for nbr ASes!
		if(ar == d_ar)
			x_id = n->id - ar->low;
		else if(state->as == d_ar->as)
			x_id = ar->nmachines + d_ar->id - state->as->low;
		else
			x_id = -1;

		if(-1 == x_id)
			n->hop_count = 0;
		else
			n->hop_count = state->m->nhop[x_id];
	}

	init_rib(state, lp);
	bgp_keepalive_timer(state, NULL, NULL, lp);
#else
	// Create and install the route in our table
	bgp_route_add(state, bgp_route_new(state, lp, lp, lp));

	for(i = 0; i < state->n_interfaces; i++)
	{
		n = &state->nbr[i];
		n->last_update = 0;

		if(n->up == TW_FALSE)
			bgp_connect(state, n, lp);
	}
#endif
}
