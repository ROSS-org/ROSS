#include <rossnet.h>

/** 
 * @file rn-netdmf.c  The functions defined in this file will mostly
 * just call corresponding functions in the rn-netdmf-wrapper.cpp file.
 * This is necessary because the C++ compiler actually chokes on our
 * ROSS code.
 */

/**
 * @fn rn_netdmf_init  This function handles initialization of the NetDMF
 * description language.  It simply calls the rnNetDMFInit function which
 * is declared extern "C" 
 */
void
rn_netdmf_init()
{
  rnNetDMFInit();
}

/**
 * @fn rn_netdmf_topology  This function creates and setups the
 * g_rn_machines global data structure of nodes.
 */
void
rn_netdmf_topology()
{
}
