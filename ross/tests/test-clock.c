#include <ross.h>

int main() {
	tw_pe test_pe;
	tw_clock clock_offset_1, clock_offset_2, 
			 clock_time_pe, clock_time_ret, clock_time_init;
	
	tw_clock_init(&test_pe);
	clock_time_init = test_pe.clock_time;
	clock_offset_1 = test_pe.clock_offset;
	
	if(clock_time_init != 0) {
		return 1;
	}
	
	clock_time_ret = tw_clock_now(&test_pe);
	clock_time_pe = test_pe.clock_time;
	clock_offset_2 = test_pe.clock_offset;
	
	if(clock_time_ret != clock_time_pe) {
		return 2;
	}
	
	if(clock_offset_1 != clock_offset_2) {
		return 3;
	}
	
	return 0;
}
