/*
	This implementation of an ARM v7 clock reader utilizes the
	Performance Monitoring Unit (PMU) on Cortex-A7 chips.
	Unfortunately, access to the cycle counter from userspace
	is disabled by default. A kernel module that enables access
	from userspace is required or the system will fault.

	An example kernel module that does just that can be found:
	https://github.com/nmcglohon/armv7l-userspace-counter.git

	More information can be found:
	http://neocontra.blogspot.com/2013/05/user-mode-performance-counters-for.html
 */

#include <ross.h>

#ifndef __GNUC__
#  error gcc asm extensions required
#endif
#if ! (defined(__arm__))
#  error only 32 bit arm platform supported
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


void tw_clock_init(tw_pe * me)
{
	me->clock_time = 0;
	me->clock_offset = tw_clock_read();
}


tw_clock tw_clock_now(tw_pe * me)
{
	me->clock_time = tw_clock_read() - me->clock_offset;
	return me->clock_time;
}
