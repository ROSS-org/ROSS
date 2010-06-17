/**
 * @file friis-prop-loss.h This contains the Friis Propagation loss model.
 * 
 */

/**
 * @fn CalcRxPower 
 * \brief A Friis propagation loss model
 *
 * The Friis propagation loss model was first described in
 * "A Note on a Simple Transmission Formula", by 
 * "Harald T. Friis".
 * 
 * The original equation was described as:
 *  \f$ \frac{P_r}{P_t} = \frac{A_r A_t}{d^2\lambda^2} \f$
 *  with the following equation for the case of an
 *  isotropic antenna with no heat loss:
 *  \f$ A_{isotr.} = \frac{\lambda^2}{4\pi} \f$
 *
 * The final equation becomes:
 * \f$ \frac{P_r}{P_t} = \frac{\lambda^2}{(4 \pi d)^2} \f$
 *
 * Modern extensions to this original equation are:
 * \f$ P_r = \frac{P_t G_t G_r \lambda^2}{(4 \pi d)^2 L}\f$
 *
 * Here, we ignore tx and rx gain and the input and output values are in dB or dBm:
 * \f$ RX_{dB} = TX_{dB} + 10\mathrm{log_{10}} \frac{\lambda^2}{(4 \pi d)^2 L}\f$
 *
 * With:
 *  - \f$ P_r \f$ : reception power (W)
 *  - \f$ P_t \f$ : transmission power (W)
 *  - \f$ G_t \f$ : transmission gain (unit-less)
 *  - \f$ G_r \f$ : reception gain (unit-less)
 *  - \f$ \lambda \f$ : wavelength (m)
 *  - \f$ d \f$ : distance (m)
 *  - \f$ L \f$ : system loss (unit-less)
 *
 *
 * This model is invalid for small distance values.
 * The current implementation returns the txpower as the rxpower
 * for any distance smaller than MinDistance.
 */

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
