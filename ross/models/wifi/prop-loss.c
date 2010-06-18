/**
 * @file prop-loss.c This contains the Friis Propagation loss model.
 * 
 */

#include "prop-loss.h"

double CalcRxPower (double txPowerDbm, double distance, double minDistance, double lambda, double systemLoss)
{
  double numerator, denominator, pr, pi;
  
  pi = 3.1415926535;
  numerator = lambda * lambda;
  denominator = 16 * pi * pi * distance * distance * systemLoss;
  pr = 10 * log10 (numerator / denominator);
  
  if (distance <= minDistance)
      return txPowerDbm;
  
  printf("distance= %f m, attenuation coefficient= %f dB", distance, pr);

  return txPowerDbm + pr;
}


double DbmFromW (double w) 
{
  double dbm = log10 (w * 1000.0) * 10.0;
  return dbm;
}
