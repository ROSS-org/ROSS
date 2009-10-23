#include <rossnet.h>

/**
 * This file provides functionality for the AS data type in rossnet.
 */
rn_area	*
rn_nextarea(rn_area * as)
{
	if(as == NULL)
		return g_rn_areas;

	if(as->id == g_rn_nareas - 1)
		return NULL;

	return &g_rn_areas[as->id+1];
}

/*
 * This function returns the next area in the AS
 */
rn_area	*
rn_nextarea_onas(rn_area * ar, rn_as * as)
{
	if(ar == NULL)
		return as->areas;
	else
		return ar->next;
}
