double 80211b_DsssDqpskCck11_SuccessRate (double sinr,uint32_t nbits)
{
  printf("Running a 802.11b CCK Matlab model less accurate than GSL model\n"); 
  // The matlab model
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