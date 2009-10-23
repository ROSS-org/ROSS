#include <rossnet.h>

rn_subnet	*
rn_nextsn_onarea(rn_subnet * sn, rn_area * ar)
{
	if(sn == NULL)
		return ar->subnets;
	else
		return sn->next;
}

rn_machine	*
rn_nextmachine_onsn(rn_machine * m, rn_subnet * sn)
{
	if(m == NULL)
		return sn->machines;
	else
		tw_error(TW_LOC, "Enable next pointer!");//return m->next;

	return NULL;
}
