#ifndef INC_clock_aarch64
#define INC_clock_aarch64

#include <stdint.h>

typedef uint64_t tw_clock;

static inline tw_clock  tw_clock_read(void)
{
        tw_clock result=0;
#ifdef ROSS_timing
       asm volatile ("mrs %0, cntvct_el0" : "=r" (result));
#endif
        return result;
}

#endif
