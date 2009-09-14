#include <rossnet.h>

/**
 * This file provides functionality for the AS data type in rossnet.
 */
rn_as	*
rn_nextas(rn_as	* as)
{
	if(as == NULL)
		return g_rn_as;

	if(as->id == g_rn_nas - 1)
		return NULL;

	return &g_rn_as[as->id+1];
}
