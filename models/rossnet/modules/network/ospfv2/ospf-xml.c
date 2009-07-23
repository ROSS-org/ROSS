#include <ospf.h>

void
ospf_xml(ospf_state * state, const xmlNodePtr node, tw_lp * lp)
{
	xmlNodePtr		interface;

	ospf_nbr		*nbr;

	int			 addr;
	int			 i;

	state->m = rn_getmachine(lp->id);
	state->ar = rn_getarea(state->m);

	/*
	 * Now we need to set up the LSAs/interfaces for the OSPF neighbors
	 */
	state->n_interfaces = 0;
	for(interface = node->children; interface; interface = interface->next)
	{
		if(0 != strcmp((char *) interface->name, "interface"))
			continue;

		addr = atoi(xml_getprop(interface, "addr"));

		if(state->ar->as == rn_getas(rn_getmachine(addr)))
			state->n_interfaces++;
	}

	if(state->n_interfaces == 0)
	{
		printf("%lld: no neighbors! \n", lp->id);
		return;
	}

	state->nbr = (ospf_nbr *) calloc(sizeof(ospf_nbr), state->n_interfaces);

	if(!state->nbr)
		tw_error(TW_LOC, "Out of memory!");

	for(i = 0, interface = node->children; interface; interface = interface->next)
	{
		if(0 != strcmp((char *) interface->name, "interface"))
			continue;

		nbr = &state->nbr[i++];
		addr = atoi(xml_getprop(interface, "addr"));

		if(state->ar->as != rn_getas(rn_getmachine(addr)))
			continue;

		nbr->id = addr;
		nbr->m = rn_getmachine(addr);
		nbr->ar = rn_getarea(nbr->m);
	}
}
