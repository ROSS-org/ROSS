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

#ifndef INC_clock_armv7l
#define INC_clock_armv7l

typedef unsigned int tw_clock;

static inline tw_clock  tw_clock_read(void)
{
	unsigned int result;
#ifdef ROSS_timing
	do {
		__asm__ __volatile__ ("MRC p15, 0, %0, c9, c13, 0" : "=r"(result));
	} while (__builtin_expect ((int) result == -1, 0));
#endif

	return result;
}

#endif
