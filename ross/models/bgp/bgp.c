/*
 *  Blue Gene/P model
 *  Header file
 *  by Ning Liu 
 */

#include "bgp.h"

const tw_optdef app_opt [] =
{
  TWOPT_GROUP("BGP Model"),
  TWOPT_UINT("memory", opt_mem, "optimistic memory"),
  TWOPT_UINT("numfs", N_FS_active, "number of file server active"),
  TWOPT_UINT("numion", N_ION_active, "number of ION active"),
  TWOPT_UINT("burst", burst_buffer_on, "burst buffer button"),
  TWOPT_END()
};

tw_peid bgp_mapping( tw_lpid gid)
{
  return (tw_peid)gid/g_tw_nlp;
  //return (tw_peid)gid%tw_nnodes();
}

tw_lptype mylps[] =
  {
    {
      (init_f) bgp_cn_init,
      (event_f) bgp_cn_eventHandler,
      (revent_f) bgp_cn_eventHandler_rc,
      (final_f) bgp_cn_finish,
      (map_f) bgp_mapping,
      sizeof( CN_state )
    },
    {
      (init_f) bgp_ion_init,
      (event_f) bgp_ion_eventHandler,
      (revent_f) bgp_ion_eventHandler_rc,
      (final_f) bgp_ion_finish,
      (map_f) bgp_mapping,
      sizeof( ION_state )
    },    
    {
      (init_f) bgp_fs_init,
      (event_f) bgp_fs_eventHandler,
      (revent_f) bgp_fs_eventHandler_rc,
      (final_f) bgp_fs_finish,
      (map_f) bgp_mapping,
      sizeof( FS_state )
    },
    {
      (init_f) bgp_controller_init,
      (event_f) bgp_controller_eventHandler,
      (revent_f) bgp_controller_eventHandler_rc,
      (final_f) bgp_controller_finish,
      (map_f) bgp_mapping,
      sizeof( CON_state )
    },
    {
      (init_f) bgp_ddn_init,
      (event_f) bgp_ddn_eventHandler,
      (revent_f) bgp_ddn_eventHandler_rc,
      (final_f) bgp_ddn_finish,
      (map_f) bgp_mapping,
      sizeof( DDN_state )
    },
    {0},
 
  };

int main( int argc, char** argv )
{
  int i;

  tw_opt_add(app_opt);
  tw_init(&argc, &argv);
  printf("First version BGP model! \n");

  ////////////
  N_controller_per_DDN = NumControllerPerDDN;
  N_FS_per_DDN = N_controller_per_DDN * NumFSPerController;
  N_ION_per_DDN = N_FS_per_DDN * N_ION_per_FS;
  N_CN_per_DDN = N_ION_per_DDN * N_CN_per_ION;

  int N_lp_per_DDN = N_controller_per_DDN + N_FS_per_DDN + N_ION_per_DDN
  + N_CN_per_DDN + 1;

  int N_lp_t = N_lp_per_DDN * NumDDN;
  nlp_per_pe = N_lp_t/tw_nnodes()/g_tw_npe;

  N_DDN_per_PE = NumDDN/tw_nnodes()/g_tw_npe;

  g_tw_events_per_pe = nlp_per_pe * 32 + opt_mem;
  tw_define_lps(nlp_per_pe, sizeof(MsgData), 0);

  // mapping initialization

  int LPaccumulate = 0;
  for( i=0; i<N_CN_per_DDN*N_DDN_per_PE; i++ )
    tw_lp_settype(i, &mylps[0]);
  LPaccumulate += N_CN_per_DDN*N_DDN_per_PE;

  for( i=0; i<N_ION_per_DDN*N_DDN_per_PE; i++ )
    tw_lp_settype(i + LPaccumulate, &mylps[1]);
  LPaccumulate += N_ION_per_DDN*N_DDN_per_PE;

  for( i=0; i<N_FS_per_DDN*N_DDN_per_PE; i++ )
    tw_lp_settype(i + LPaccumulate, &mylps[2]);
  LPaccumulate += N_FS_per_DDN*N_DDN_per_PE;

  for( i=0; i<N_controller_per_DDN*N_DDN_per_PE; i++ )
    tw_lp_settype(i + LPaccumulate, &mylps[3]);
  LPaccumulate += N_controller_per_DDN*N_DDN_per_PE;

  for( i=0; i<N_DDN_per_PE; i++ )
    tw_lp_settype(i + LPaccumulate, &mylps[4]);

  tw_run();

  tw_end();
  return 0;

}
