#include <ross.h>

#ifndef __GNUC__
#  error gcc asm extensions required
#endif
#if ! (defined(__amd64__) || defined(__x86_64__))
#  error only amd64 platform supported
#endif

/*
 * Our function below calls the "rdtsc" x86 assembly language function
 * to obtain the current clock cycle value.
 */
static const tw_optdef clock_opts [] =
{
	TWOPT_GROUP("ROSS Timing"),
	TWOPT_STIME("clock-rate", g_tw_clock_rate, "CPU Clock Rate"),
	TWOPT_END()
};

const tw_optdef *tw_clock_setup(void)
{
	return clock_opts;
}



void
tw_clock_init(tw_pe * me)
{
	me->clock_time = 0;
	me->clock_offset = tw_clock_read();
}

tw_clock
tw_clock_now(tw_pe * me)
{
	me->clock_time = tw_clock_read() - me->clock_offset;
	return me->clock_time;
}
