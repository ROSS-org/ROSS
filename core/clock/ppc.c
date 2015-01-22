#include <ross.h>

#ifndef __GNUC__
#  error gcc asm extensions required
#endif
#ifndef __ppc__
#  error only ppc platform supported
#endif

static tw_clock tw_clock_read(void)
{
	unsigned long tbu;
	unsigned long tb1;
	unsigned long tbu1;

	do {
		asm volatile(
			"mftbu %2\n\t"
			"mftb  %0\n\t"
			"mftbu %1\n\t"
		: "=r"(tb1), "=r"(tbu), "=r"(tbu1) );
	} while (tbu != tbu1);

	return ( ((tw_clock)tbu) << 32 ) | tb1;
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
