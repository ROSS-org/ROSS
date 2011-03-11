#include <ross.h>

static const tw_optdef clock_opts [] = {
	TWOPT_GROUP("ROSS Timing"),
	TWOPT_STIME("clock-rate", g_tw_clock_rate, "CPU Clock Rate"),
	TWOPT_END()
};

const tw_optdef *tw_clock_setup(void) {
	return clock_opts;
}

/** tw_clock_read
*	@brief Reads chip clock using hardware registers
*
*	Chip Clock read using ASM instructions.  
*   Result is stored in tw_clock type
**/
tw_clock tw_clock_read(void) {
	tw_clock result;

#ifdef I386
	do {
		__asm__ __volatile__("rdtsc" : "=A" (result)); 
	} while (__builtin_expect ((int) result == -1, 0));
	return result;
	
#elif X86_64
	uint32_t a, d;
	do {
		__asm__ __volatile__("rdtsc" : "=a" (a), "=d" (d)); 
		result = ((uint64_t)a) | (((uint64_t)d) << 32);
	} while (__builtin_expect ((int) result == -1, 0));

#elif BLUE_GENE
	result = rts_get_timebase();
	
#elif PPC
	uint32_t tbu, tb1, tbu1;

	do {
		asm volatile(
			"mftbu %2\n\t"
			"mftb  %0\n\t"
			"mftbu %1\n\t"
		: "=r"(tb1), "=r"(tbu), "=r"(tbu1) );
	} while (tbu != tbu1);

	result = ( ((tw_clock)tbu) << 32 ) | tb1;

#elif IA64
	do {
		__asm__ __volatile__("mov %0=ar.itc" : "=r"(result) :: "memory");
	} while (__builtin_expect ((int) result == -1, 0));

#else
	result = 0;
#endif

	return result;	
}

void tw_clock_init(tw_pe * me) {
	me->clock_time = 0;
	me->clock_offset = tw_clock_read();
}

tw_clock tw_clock_now(tw_pe * me) {
	me->clock_time = tw_clock_read() - me->clock_offset;
	return me->clock_time;
}