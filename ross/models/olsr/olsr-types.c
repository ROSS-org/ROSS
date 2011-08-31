#include <math.h>

#include "olsr-constants.h"
#include "olsr-types.h"

inline double uint8_time_to_double(uint8_t htime) {
	uint8_t a = 0xF0 & htime;
	uint8_t b = 0x0F & htime;
	
	return C*(1+a/16)*pow(2,b);
}

uint8_t double_time_to_uint8(double time) {
	uint8_t b = (uint8_t)log2(time / C);
	uint8_t a = 16*(time/(C*(2^b))-1);

	return a*16+b;
}