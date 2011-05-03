/**
 * @file prop-loss.h This contains the Friis Propagation loss model.
 * 
 */

#include <ross.h>

#define BOLTZMANN 	1.3803e-23 /**< thermal noise at 290K in J/s = W */
#define PI 			3.1415926535 /**< mmm, Pi. */
#define PI_SQ		9.8696043785
#define FREQ   		2400000000
#define LAMBDA 		0.125
#define MIN_DISTANCE 0.0001



typedef struct {
  double signal;
  double bandwidth;
  double noiseFigure;
  double noiseInterference;
} rf_signal;

/**
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
 *  
 *  MinDistance is defined as follows: (Paul, Whites, Nasir, Intro. to Electromagnetic Fields)
 *  \f$ d = \frac{kD^2}{8\lambda} \f$
 *  Picking a typical value of 16 for k, and assuming a 1/4 wavelength antenna MinDistance becomes:
 *  \f$ d = \frac{1}{8}\lambda \f$
 *
 *
 */
double calcRxPower (double txPowerDbm, double distance, double lambda);
double dbmFromW (double w);
double wFromDbm(double dbm);
double calculateSnr(rf_signal rf);


