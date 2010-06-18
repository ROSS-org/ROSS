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
