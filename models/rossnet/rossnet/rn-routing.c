#include <rossnet.h>

/*
 * Returns the port number to use for forwarding to dst
 */
int
rn_route(rn_machine * m, tw_lpid dst)
{
	rn_area	*ar = rn_getarea(m);
	rn_as	*as = ar->as;

	rn_area	*d_ar = rn_getarea(rn_getmachine(dst));
	rn_as	*d_as = rn_getas(rn_getmachine(dst));

	if(dst >= ar->low && dst <= ar->high)
	{
		if(dst - ar->low >= ar->g_ospf_nlsa)
			tw_error(TW_LOC, "Out of range!");

		return (int) m->ft[dst - ar->low];
	} else if(d_ar->id >= as->low && d_ar->id <= as->high)
	{
		if(ar->nmachines + d_ar->id - d_as->low  >= ar->g_ospf_nlsa)
			tw_error(TW_LOC, "Out of range!");

		return (int) m->ft[ar->nmachines + d_ar->id - d_as->low];
	} else if(d_as->id >= 0 && d_as->id < g_rn_nas)
	{
		if(ar->nmachines + as->nareas + d_as->id  >= ar->g_ospf_nlsa)
			tw_error(TW_LOC, "Out of range!");

		return (int) m->ft[ar->nmachines + as->nareas + d_as->id];
	} else
	{
		printf("%lld: Attempting to route to network? %d >= %lld \n",
			m->id, ar->nmachines + as->nareas + g_rn_nas, dst);
		printf("%lld: nMachines %d, Area %d, AS nAreas %d, AS %d \n",
			m->id, ar->nmachines, ar->id, as->nareas, as->id);

		return -1;
	}
}

/*
 * Change a next-hop port to one of: self, no_route, port #
 */
int
rn_route_change(rn_machine * m, int loc, int new_val)
{
	char old_val = m->ft[loc];

	m->ft[loc] = new_val;

	return (int) old_val;
}
