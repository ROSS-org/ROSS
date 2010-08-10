#include <ross.h>
#include "srw.h"

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
 * containing the filename of the configuration.  Further, this function
 * WILL be called via function pointer after the LPs are properly
 * configured.  We need to be called at that point so we can adjust
 * data contained in the LP states/events.
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

/**
 * Attach an srw_node_info struct to an LP.
 */
void attach_node_to_lp(long lpnum, long nodeId)
{
  tw_lp *lp = g_tw_lp[lpnum];
  srw_state *srws = lp->cur_state;
  srw_node_info *nodes = srws->nodes;

  nodes[srws->num_radios].node_id = nodeId;
  srws->num_radios++;
}
