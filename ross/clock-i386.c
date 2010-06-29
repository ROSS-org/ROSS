#include <ross.h>

#ifndef __GNUC__
#  error gcc asm extensions required
#endif
#ifndef __i386__
#  error only i386 platform supported
#endif

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

tw_clock tw_clock_read(void)
{
	tw_clock result;
	do {
		__asm__ __volatile__("rdtsc" : "=A" (result)); 
	} while (__builtin_expect ((int) result == -1, 0));
	return result;
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
