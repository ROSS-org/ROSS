#include <ross.h>

#ifndef __GNUC__
#  error gcc asm extensions required
#endif
#ifndef __ia64__
#  error only ia64 platform supported
#endif

static tw_clock tw_clock_read(void)
{
	tw_clock result;
	do {
		__asm__ __volatile__("mov %0=ar.itc" : "=r"(result) :: "memory");
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
