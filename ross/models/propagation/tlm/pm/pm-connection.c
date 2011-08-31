#include <pm.h>

//static FILE	*F = NULL;

void
pm_connection_print(pm_state * state, pm_connection * c, tw_lp * lp)
{
#if 0
	if(!c)
		return;

	fprintf(F, "%4.4lf,%d,%ld,%2.4lf,%2.4lf,%ld\n",
		tw_now(lp), c->id, lp->id, state->x_pos, state->y_pos, c->to);
#endif
}
