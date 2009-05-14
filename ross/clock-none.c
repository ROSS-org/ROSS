#include<ross.h>

void
tw_clock_init(tw_pe *me)
{
}

tw_clock
tw_clock_now(tw_pe *me)
{
	tw_error(TW_LOC, "Compiled without clock support.");
}
