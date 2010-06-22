#include <ross.h>

#define WLAN_SIR_PERFECT 10
#define WLAN_SIR_IMPOSSIBLE 0.1

#ifdef ENABLE_GSL
double SymbolErrorProb256Cck (double e1);
double SymbolErrorProb16Cck (double e2);
double IntegralFunction (double x, void *params);
#endif

double WiFi_80211b_DsssDqpskCck11_SuccessRate (double sinr,uint32_t nbits);
