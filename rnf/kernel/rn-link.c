#include <rossnet.h>

void
rn_link_setwire(rn_link * l)
{
	if(NULL == l->wire)
		l->wire = rn_getmachine(l->addr);
}

rn_machine *
rn_link_getwire(rn_link * l)
{
	return l->wire;
}

rn_link	*
rn_getlink(rn_machine * m, tw_lpid id)
{
#if 0
	int	i;

	for(i = 0; i < m->nlinks; i++)
		if(m->link[i].addr == id)
			return &m->link[i];

	return NULL;
#else
	return rn_hash_fetch(m->hash_link, id);
#endif
}

rn_link_status
rn_link_getstatus(rn_link * l, tw_stime ts)
{
	if(!l)
		tw_error(TW_LOC, "No link!");

#if DYNAMIC_LINKS
	if((l->status == rn_link_down && ts < l->next_status) ||
	   (l->status == rn_link_up && ts >= l->next_status))
		return rn_link_down;
#endif

	return rn_link_up;
}

/*
 * Iterate through my links: probability a link will go up/down == 1%
 */
void
rn_link_random_changes(tw_lp * lp)
{
#if DYNAMIC_LINKS
	rn_machine	*m;
	rn_link		*l;
	rn_link		*ol;

	int	 	 percent;
	int		 i;

	m = rn_getmachine(lp->gid);

	for(i = 0; i < m->nlinks; i++)
	{
		l = &m->link[i];
	
		if(l->next_status != INT_MAX)
			continue;
	
		percent = tw_rand_integer(lp->rng, 0, 100);

		if(percent > g_rn_link_prob + 1)
			return;

		ol = rn_getlink(l->wire, m->id);

		//l->next_status = ol->next_status = tw_rand_integer(lp->gid, 25, g_tw_ts_end);
		//l->last_status = ol->last_status = 0;
		l->next_status = tw_rand_integer(lp->rng, 25, g_tw_ts_end);
		l->last_status = 0;
	}
#endif
}

/*
 * Never used.. implemented in OSPF directly.
 */
void
rn_link_random_weight_changes(tw_lp * lp)
{
	tw_stime	 next_status;
	tw_event	*e;

	rn_message	*msg;
	rn_machine	*m;

	int	 	 percent;
	int		 i;

	m = rn_getmachine(lp->gid);

	for(i = 0; i < m->nlinks; i++)
	{
		percent = tw_rand_integer(lp->rng, 0, 100);

		if(percent > 1 + 1)
			return;

		next_status = tw_rand_integer(lp->rng, 1, g_tw_ts_end);

		e = tw_event_new(lp->gid, next_status, lp);
		msg = tw_event_data(e);

		msg->port = i;
		msg->src = lp->gid;
		msg->dst = lp->gid;
		msg->type = LINK_STATUS_WEIGHT;

		tw_event_send(e);
	}
}
