#include <ross.h>

#ifndef __GNUC__
#  error gcc asm extensions required
#endif
#ifndef __amd64__
#  error only amd64 platform supported
#endif

tw_clock rdtsc(void)
{
	tw_clock result;

	unsigned a, d; 

	do {
		__asm__ __volatile__("rdtsc" : "=a" (a), "=d" (d)); 
		result = ((uint64_t)a) | (((uint64_t)d) << 32);
	} while (__builtin_expect ((int) result == -1, 0));

	return result;
}

void
tw_clock_init(tw_pe * me)
{
	me->clock_time = 0;
	me->clock_offset = rdtsc();
}

tw_clock
tw_clock_now(tw_pe * me)
{
	me->clock_time = rdtsc() - me->clock_offset;
	return me->clock_time;
}
