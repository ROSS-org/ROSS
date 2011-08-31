#include <ross.h>
#include "prop-loss.h"
#include "olsr-types.h"

int main() {
	double txPowerDbm = 0; //1mW transmit
	double distance = 10; //10 m
	double lambda = 0.125; //2.4 Ghz Exactly = 0.125 m
	
	double rxPowerDbm = calcRxPower (txPowerDbm, distance, lambda);
	printf("RX Power: %f dBm\n", rxPowerDbm);
	int* a = calloc(sizeof(int), 5);
	
	uint8_t i;
	for(i = 0; i < 120; i++) {
		printf("%d: ", i);
		uint8_t z = double_time_to_uint8(i);
		printf("%d ", z);
		printf("%f\n", uint8_time_to_double(z));
	}
	//printf("%f\n", uint8_time_to_double(255));
	return 0;
}