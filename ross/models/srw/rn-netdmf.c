#include <ross.h>

/**
 * @file
 * @brief NetDMF Support
 *
 * The functions defined in this file will mostly
 * just call corresponding functions in the rn-netdmf-wrapper.cpp file.
 * This is necessary because the C++ compiler actually chokes on our
 * ROSS code.
 */

void rnNetDMFInit();

/**
 * This function handles initialization of the NetDMF
 * description language.  It simply calls the rnNetDMFInit function which
 * is declared extern "C".  This function should use our global variable
 * containing the filename of the configuration.
 */
void
rn_netdmf_init()
{
  rnNetDMFInit();
}

/**
 * This function creates and setups the
 * g_rn_machines global data structure of nodes.
 */
void
rn_netdmf_topology()
{
}
