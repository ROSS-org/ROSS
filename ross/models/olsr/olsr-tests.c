#include <ross.h>
#include "prop-loss.h"

int main() {
	double txPowerDbm = 0; //1mW transmit
	double distance = 10; //10 m
	double lambda = 0.125; //2.4 Ghz Exactly = 0.125 m
	
	double rxPowerDbm = calcRxPower (txPowerDbm, distance, lambda);
	printf("RX Power: %f dBm\n", rxPowerDbm);
	
	
	return 0;
}