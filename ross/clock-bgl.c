#include <ross.h>

tw_clock rdtsc(void)
{
 unsigned long long int result=0;
 unsigned long int upper, lower,tmp;
 __asm__ volatile(
               "0:                  \n"
               "\tmftbu   %0           \n"
               "\tmftb    %1           \n"
               "\tmftbu   %2           \n"
               "\tcmpw    %2,%0        \n"
               "\tbne     0b         \n"
               : "=r"(upper),"=r"(lower),"=r"(tmp)
               );
 result = upper;
 result = result<<32;
 result = result|lower;

 return(result);
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
