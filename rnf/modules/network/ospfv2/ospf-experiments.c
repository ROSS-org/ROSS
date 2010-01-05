#include <ospf.h>

void
ospf_random_weights(ospf_state * state, tw_lp * lp)
{
	tw_memory	*b;
	tw_event	*e;
	tw_stime	 next_status;

	ospf_message	*m;

	int	 	 percent;
	long int	 i;

	for(i = 0; i < state->m->nlinks; i++)
	{
		percent = tw_rand_integer(lp->rng, 0, 100);

		if(percent > 10 + 1)
			return;

		next_status = tw_rand_integer(lp->rng, 1, g_tw_ts_end);

		if(next_status >= g_tw_ts_end)
			continue;

		e = NULL;
		e = rn_timer_simple(lp, next_status);

		b = tw_memory_alloc(lp, g_ospf_fd);
		m = tw_memory_data(b);

		m->type = OSPF_WEIGHT_CHANGE;
		m->data = (void *) i;

		tw_event_memory_set(e, b, g_ospf_fd);
	}
}

void
ospf_experiment_weights(ospf_state * state, long int i, tw_lp * lp)
{
	ospf_db_entry	*dbe;

	int		 metric;
	int		 low;
	int		 high;

	low = 50 - g_ospf_link_weight;
	high = 50 + g_ospf_link_weight;

	if(low <= 0)
		low = 1;

	metric = tw_rand_integer(lp->rng, low, high);

	if(metric == state->m->link[i].cost)
		return;

#if VERIFY_OSPF_EXPERIMENT
	printf("%ld: updating link %ld-%d metric: %d -> %d \n", 
		lp->gid, state->m->id, state->m->link[i].addr, 
		state->m->link[i].cost, metric);
#endif

	state->m->link[i].cost = metric;
	dbe = &state->db[lp->gid - state->ar->low];

	dbe->lsa = ospf_lsa_find(state, NULL, lp->gid - state->ar->low, lp);
	dbe->cause_ospf = 1;
	dbe->cause_bgp = 0;

	ospf_rt_build(state, lp->gid);
	ospf_lsa_refresh(state, dbe, lp);

	dbe->cause_ospf = 0;
}
