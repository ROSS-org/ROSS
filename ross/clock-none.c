#include<ross.h>

tw_clock
tw_clock_read(void)
{
	return 0;
}

void
tw_clock_init(tw_pe *me)
{
}

tw_clock
tw_clock_now(tw_pe *me)
{
	tw_error(TW_LOC, "Compiled without clock support.");
}
