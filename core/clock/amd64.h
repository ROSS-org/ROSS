#ifndef INC_clock_amd64
#define INC_clock_amd64

typedef uint64_t tw_clock;

static inline tw_clock  tw_clock_read(void)
{
	tw_clock result=0;
#ifdef ROSS_timing
	unsigned a, d; 

	do {
		__asm__ __volatile__("rdtsc" : "=a" (a), "=d" (d)); 
		result = ((uint64_t)a) | (((uint64_t)d) << 32);
	} while (__builtin_expect ((int) result == -1, 0));
#endif
	return result;
}

#endif
