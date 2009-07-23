#include <bgp.h>

#define IBGP 1

void
bgp_xml(bgp_state * state, const xmlNodePtr node, tw_lp * lp)
{
#if IBGP 
	xmlXPathObjectPtr	 obj;
	xmlNodePtr		 node2;

	int			 id;
	int			 j;
#endif

	xmlNodePtr		 interface;

	rn_machine		*m;
	rn_area			*ar;

	bgp_nbr			*n;

	//char			*local_pref;

	int			 i;
	int			 lvl;

#if IBGP_TREE
	int			 diff;
	int			 has_ibgp;
#endif

	state->m = m = rn_getmachine(lp->id);
	state->as = rn_getas(m);
	ar = rn_getarea(m);

	// Check for local parameter settings
	// then check for global parameter settings
	// then set to defaults
	if(0 != strcmp("", xml_getprop(node, "keep_alive")))
	{
		//state->keepalive_interval = atof(xml_getprop(node, "keep_alive"));
		//state->hold_interval = atof(xml_getprop(node, "hold"));
		//state->mrai = atof(xml_getprop(node, "mrai"));
	} else if(NULL != g_bgp_state)
	{
		state->keepalive_interval = g_bgp_state->keepalive_interval;
		state->hold_interval = g_bgp_state->hold_interval;
		state->mrai_interval = g_bgp_state->mrai_interval;
	} else
	{
		//state->keepalive_interval = 30.0;
		//state->hold_interval = 45.0;
		//state->mrai = 90.0;
	}

	/*
	 * Now we need to set up the interfaces for the BGP neighbors
	 */
	for(interface = node->children; interface; interface = interface->next)
	{
		if(0 != strcmp((char *) interface->name, "neighbor"))
			continue;

		state->n_interfaces++;
	}

	// Add in the iBGP neighbors
#if IBGP
	state->n_interfaces += g_bgp_ases[state->as->id]->nodesetval->nodeNr - 1;
	state->n_interfaces += state->as->nareas;
#endif

	if(state->n_interfaces <= 0)
		return; //tw_error(TW_LOC, "BGP router has no neighbors!");

	state->nbr = (bgp_nbr *) calloc(sizeof(bgp_nbr), state->n_interfaces);

	if(!state->nbr)
		tw_error(TW_LOC, "Out of memory!");

	for(i = 0, interface = node->children; interface; interface = interface->next)
	{
		if(0 != strcmp((char *) interface->name, "neighbor"))
			continue;

		if(rn_getmachine(atoi(xml_getprop(interface, "addr")))->subnet->area->as == state->as)
		{
			if(lp->id % 100000 == 0)
				printf("%lld: removing iBGP node from nbrs: %d \n",
					lp->id, atoi(xml_getprop(interface, "addr")));
			state->n_interfaces--;
			continue;
		}

		n = &state->nbr[i++];

		n->up = TW_FALSE;
		n->id = atoi(xml_getprop(interface, "addr"));

		g_bgp_as[state->as->id].degree[rn_getas(rn_getmachine(n->id))->id]++;
		g_bgp_as[state->as->id].degree[state->as->id]++;

#if 0
		local_pref = xml_getprop(interface, "local_pref");

		if(0 != strcmp("", local_pref))
		{
			n->local_pref = atoi(local_pref);

#if VERIFY_BGP
			printf("%ld BGP: nbr %d local_pref: %d \n",
				lp->id, n->id, n->local_pref);
#endif
		} else
		{
			n->local_pref = 100;
		}
#endif
	}

#if IBGP
	// AS level
#if 0
	// Determine Area root node
	if(ar->root_lvl > m->level)
	{
		ar->root = m->id;
		ar->root_lvl = m->level;
	}
#endif

	state->n_interfaces -= state->as->nareas;

	obj = g_bgp_ases[state->as->id];
	node2 = obj->nodesetval->nodeTab[0];
	for(j = 0; j < obj->nodesetval->nodeNr; node2 = obj->nodesetval->nodeTab[++j])
	{
		id = atoi(xml_getprop(node2->parent, "id"));
		lvl = atoi(xml_getprop(node2->parent, "lvl"));

		if(id == lp->id)
			continue;

#if IBGP_TREE
		diff = state->m->level - lvl;
		if(state->m->subnet->area == rn_getarea(rn_getmachine(id)) &&
		    (diff == -1 || diff == 1))
		{
			has_ibgp = 1;
			n = &state->nbr[i++];
			n->id = id;
			n->up = TW_TRUE;
		} else
			state->n_interfaces--;
#else
		n = &state->nbr[i++];
		n->id = id;
		n->up = TW_TRUE;
#endif
	}

#if 0
	// Make sure we're at least connected once to our BGP network
	if(state->m->root == 0 && has_ibgp == 0)
	{
		state->n_interfaces++;
		n = &state->nbr[i++];
		n->id = state->m->subnet->area->root;
		n->up = TW_FALSE;
	}
#endif
#endif
}
