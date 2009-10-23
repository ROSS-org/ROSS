#include <bgp.h>

/**
 * This function returns the PEER associated with id
 */
bgp_nbr	*
bgp_getnbr(bgp_state * state, int id)
{
	bgp_nbr		*n;
	int		 i;

	for(i = 0; i < state->n_interfaces; i++)
	{
		n = &state->nbr[i];

		if (n->id == id)
			return n;
	}

	//tw_error(TW_LOC, "Unable to find nbr: %d \n", id);
	return NULL;
}
