/**
 * @file prop-loss.c This contains the Friis Propagation loss model.
 * 
 */

#include "prop-loss.h"

double calcRxPower(double txPowerDbm, double distance, double minDistance, double lambda, double systemLoss) {
  double numerator, denominator, pr;
  
  numerator = lambda * lambda;
  denominator = 16 * PI * PI * distance * distance * systemLoss;
  pr = 10 * log10 (numerator / denominator);
  
  if (distance <= minDistance)
      return txPowerDbm;
  
  printf("distance= %f m, attenuation coefficient= %f dB", distance, pr);

  return txPowerDbm + pr;
}


double dbmFromW(double w) {
  double dbm = log10 (w * 1000.0) * 10.0;
  return dbm;
}

double wFromDbm(double dbm) {
  double w = pow(10,(dbm * 0.1)) * 0.001;
  return w;
}

double ratioToDb(double ratio)
{
  return 10.0 * log10(ratio);
}

double calculateSnr(rf_signal rf)
{
  // Nt is the power of thermal noise in W
  double Nt = BOLTZMANN * 290.0 * rf.bandwidth;
  // receiver noise Floor (W) which accounts for thermal noise and non-idealities of the receiver
  double noiseFloor = rf.noiseFigure * Nt;
  double noise = noiseFloor + rf.noiseInterference;
  double snr = rf.signal / noise;
  return snr;
}
