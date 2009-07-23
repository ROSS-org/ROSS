#include <pm.h>

/*
 * pm_pleff - calculates the pathloss through emtpy space
 *
 * d		- distance between two nodes
 */
double
pm_pathloss_eff(pm_state * state, double d)
{
	double	 pl = 0.0;

	if (d <= 1)
		pl = state->pathloss1;
	else if (d <= state->breakpoint)
		pl = state->pathloss1 + (10 * state->slope1 * log10(d));
	else
		pl = (state->pathloss1 + (10 * state->slope1 * state->log_bp)) +
				(10 * state->slope2 * (log10(d) - state->log_bp));

	return pl;
}
