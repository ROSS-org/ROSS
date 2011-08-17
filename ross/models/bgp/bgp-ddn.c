/*
 *  Blue Gene/P model
 *  DDN
 *  by Ning Liu 
 */
#include "bgp.h"

void bgp_ddn_init( DDN_state* s,  tw_lp* lp )
{
  int i;
  int N_PE = tw_nnodes();                                                           
  nlp_DDN = NumDDN / N_PE;                                                          
  nlp_Controller = nlp_DDN * NumControllerPerDDN;                                   
  nlp_FS = nlp_Controller * NumFSPerController;                                     
  nlp_ION = nlp_FS * N_ION_per_FS;
  nlp_CN = nlp_ION * N_CN_per_ION;

  // calculate the IDs of CON which are hooked to this DDN
  s->previous_CON_id = (int *)calloc( NumControllerPerDDN, sizeof(int) );
  int PEid = lp->gid / nlp_per_pe;
  int localID = lp->gid % nlp_per_pe;
  int base = nlp_CN + nlp_ION + nlp_FS;
  localID = localID - base - nlp_Controller;
  
  for (i=0; i<NumControllerPerDDN; i++)
    s->previous_CON_id[i] = PEid * nlp_per_pe + base + 
      localID * NumControllerPerDDN + i;

#ifdef PRINTid
  for (i=0; i<NumControllerPerDDN; i++)
    printf("DDN LP %d speaking, my CON is %d\n", lp->gid,s->previous_CON_id[i]);
#endif

}

void bgp_ddn_eventHandler( DDN_state* s, tw_bf* bf, MsgData* msg, tw_lp* lp )
{
  tw_event * e;
  tw_stime ts;
  MsgData * m;
  int i;


}

void bgp_ddn_eventHandler_rc( DDN_state* s, tw_bf* bf, MsgData* m, tw_lp* lp )
{}

void bgp_ddn_finish( DDN_state* s, tw_lp* lp )
{
  free(s->previous_CON_id);
}
