#include "coding-error.h"

#ifdef ENABLE_GSL
double IntegralFunction (double x, void *params)
{
  double beta = ((FunctionParameters *) params)->beta;
  double n = ((FunctionParameters *) params)->n;
  double IntegralFunction = pow (2*gsl_cdf_ugaussian_P (x+ beta) - 1, n-1) 
                            * exp (-x*x/2.0) / sqrt (2.0 * M_PI);
  return IntegralFunction;
}

double SymbolErrorProb16Cck (double e2)
{
  double sep;
  double error;
 
  FunctionParameters params;
  params.beta = sqrt (2.0*e2);
  params.n = 8.0;

  gsl_integration_workspace * w = gsl_integration_workspace_alloc (1000); 
 
  gsl_function F;
  F.function = &IntegralFunction;
  F.params = &params;

  gsl_integration_qagiu (&F,-params.beta, 0, 1e-7, 1000, w, &sep, &error);
  gsl_integration_workspace_free (w);
  if (error == 0.0) 
    {
       sep = 1.0;
    }

  return 1.0 - sep;
}

double SymbolErrorProb256Cck (double e1)
{
  return 1.0 - pow (1.0 - SymbolErrorProb16Cck (e1/2.0), 2.0);
}

double Wifi_80211b_DsssDqpskCck11_SuccessRate (double sinr,uint32_t nbits)
{
 // symbol error probability
  double EbN0 = sinr * 22000000.0 / 1375000.0 / 8.0;
  double sep = SymbolErrorProb256Cck (8.0*EbN0/2.0);
  return pow (1.0-sep,nbits/8.0);
}

#elif defined(ENABLE_ESSL)
#error ESSL not supported just yet
#else
/**
 * \brief Model the error rate for different modulations.
 *
 * A packet of interest (e.g., a packet can potentially be received by the MAC) 
 * is divided into chunks. Each chunk is related to an start/end receiving event. 
 * For each chunk, it calculates the ratio (SINR) between received power of packet 
 * of interest and summation of noise and interfering power of all the other incoming 
 * packets. Then, it will calculate the success rate of the chunk based on 
 * BER of the modulation. The success reception rate of the packet is derived from 
 * the success rate of all chunks.
 *
 * The 802.11b modulations:
 *    - 1 Mbps mode is based on DBPSK. BER is from equation 5.2-69 from John G. Proakis
 *      Digitial Communications, 2001 edition
 *    - 2 Mbps model is based on DQPSK. Equation 8 from "Tight bounds and accurate 
 *      approximations for dqpsk transmission bit error rate", G. Ferrari and G.E. Corazza 
 *      ELECTRONICS LETTERS, 40(20):1284-1285, September 2004
 *    - 5.5 Mbps and 11 Mbps are based on equations (18) and (17) from "Properties and 
 *      performance of the ieee 802.11b complementarycode-key signal sets", 
 *      Michael B. Pursley and Thomas C. Royster. IEEE TRANSACTIONS ON COMMUNICATIONS, 
 *      57(2):440-449, February 2009.
 *    - More detailed description and validation can be found in 
 *      <a href="http://www.nsnam.org/~pei/80211b.pdf">http://www.nsnam.org/~pei/80211b.pdf</a>
 */
double WiFi_80211b_DsssDqpskCck11_SuccessRate(double sinr,uint32_t nbits)
{
  double ber; 
  if (sinr > WLAN_SIR_PERFECT)
    {
       ber = 0.0 ;
    }
  else if (sinr < WLAN_SIR_IMPOSSIBLE)
    {
       ber = 0.5;
    }
  else
    { // fitprops.coeff from matlab berfit
       double a1 =  7.9056742265333456e-003;
       double a2 = -1.8397449399176360e-001;
       double a3 =  1.0740689468707241e+000;
       double a4 =  1.0523316904502553e+000;
       double a5 =  3.0552298746496687e-001;
       double a6 =  2.2032715128698435e+000;
       ber =  (a1*sinr*sinr+a2*sinr+a3)/(sinr*sinr*sinr+a4*sinr*sinr+a5*sinr+a6);
     }
  return pow ((1.0 - ber), nbits);
}

#endif /* ENABLE_GSL */
