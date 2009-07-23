#include <ospf.h>

ospf_nbr	*
ospf_getnbr(ospf_state * state, int id)
{
	int	 i;

	for(i = 0; i < state->n_interfaces; i++)
		if(state->nbr[i].id == id)
			return &state->nbr[i];

	//tw_error(TW_LOC, "Unable to find neighbor!");

	return NULL;
}

void
neighbor_init(ospf_state * state, ospf_nbr * nbr, tw_lp * lp)
{
	rn_link		*l;

	nbr->router = state;
	nbr->id = nbr->m->id;

	nbr->state = ospf_nbr_full_st;
	nbr->istate = ospf_int_point_to_point_st;
	nbr->ltype = ospf_link_point_to_point;
	nbr->dd_seqnum = 0;

	if(nbr->m->type != c_router)
		return;

	l = rn_getlink(state->m, nbr->id);

/*
	if(!l->cost)
		tw_error(TW_LOC, "%ld: link to %d has not cost! \n", lp->id, l->addr);
*/

	nbr->hello_interval = g_ospf_state->hello_sendnext;
	nbr->router_dead_interval = g_ospf_state->poll_interval;

	if(nbr->router_dead_interval < (l->delay * .15))
		tw_error(TW_LOC, "Link delay too large for this router "
			"dead interval (%f > %f)!",
			l->delay, nbr->router_dead_interval);

	nbr->hello = (ospf_hello *) calloc(sizeof(ospf_hello) + 
					(sizeof(unsigned int) * state->n_interfaces), 1);

	if(!nbr->hello)
		tw_error(TW_LOC, "Out of memory!");

	nbr->hello->neighbors = (unsigned int *) (((char *) nbr->hello) + sizeof(ospf_hello));
	nbr->requests = (char *) calloc(state->ar->g_ospf_nlsa, 1);

	if(!nbr->requests)
		tw_error(TW_LOC, "Out of memory!");

	if(nbr->ar->as != state->ar->as)
		return;

	if(rn_getmachine(nbr->id)->type == c_router)
	{
		nbr->hello_timer = NULL;

		nbr->hello_timer = ospf_timer_start(nbr, nbr->hello_timer,
						    nbr->hello_interval,
						    OSPF_HELLO_SEND, lp);

		nbr->inactivity_timer = ospf_timer_start(nbr, nbr->inactivity_timer,
						 nbr->router_dead_interval,
						 OSPF_HELLO_TIMEOUT,
						 lp);
	}
}

	/*
	 * Setup the neighbor data structures
	 */
void
ospf_neighbor_init(ospf_state * state, tw_lp * lp)
{
	int	 	 i;

	for(i = 0; i < state->n_interfaces; i++)
		neighbor_init(state, &state->nbr[i], lp);
}

/*
 * THIS FUNCTION SHOULD NOT BE USED IN SIMULATION, ONLY IN INIT!!
 */
ospf_nbr *
ospf_get_neighbor(ospf_state * state, int id)
{
	int i;

	for(i = 0; i < state->n_interfaces; i++)
		if(id == state->nbr[i].id)
			return &state->nbr[i];

	tw_error(TW_LOC, "Unable to find neighbor %d!!", id);

	return NULL;
}
