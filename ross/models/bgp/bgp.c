/*
 *  Blue Gene/P model
 *  Header file
 *  by Ning Liu 
 */

#include "bgp.h"

const tw_optdef app_opt [] =
{
  TWOPT_GROUP("BGP Model"),
  //TWOPT_UINT("memory", opt_mem, "optimistic memory"),
  //TWOPT_STIME("arrive_rate", ARRIVAL_RATE, "packet arrive rate"),
  TWOPT_END()
};

tw_peid bgp_mapping( tw_lpid gid)
{
  return (tw_peid)gid/g_tw_nlp;
}

tw_lptype mylps[] =
  {
    // IO LP                                    
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

  ///////////////////////////////////////////////////////////////
  //nlp_per_pe = N_nodes/tw_nnodes()/g_tw_npe;
  g_tw_events_per_pe = nlp_per_pe * 16 + OPT_MEM;
  tw_define_lps(nlp_per_pe, sizeof(MsgData), 0);

  printf("g_tw_nlp is %d\n",g_tw_nlp);

  printf("N_nodes is %d, tw_nnodes() is %d, g_tw_npe is %d\n",
	 N_nodes,tw_nnodes(),g_tw_npe);

  printf("lp per pe is %d\n",nlp_per_pe);
  ///////////////////////////////////////////////////////////////
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


  /*for( i=0; i<N_FS_per_PE; i++ )
    {
      
      for( j=0; j<N_CN_per_FS; j++ )
	tw_lp_settype( i * N_lp_per_FS + j, &mylps[0] );
      for( j=0; j<N_ION_per_FS; j++ )
	tw_lp_settype( i * N_lp_per_FS + N_CN_per_FS +j, &mylps[1] );
      tw_lp_settype( i * N_lp_per_FS + N_CN_per_FS + N_ION_per_FS, &mylps[2] );
      for( j=0; j<N_Disk_per_FS; j++ )
	tw_lp_settype( i * N_lp_per_FS + N_CN_per_FS + N_ION_per_FS + 1 + j, &mylps[3] );
	}*/
  
  //for(i = 0; i < g_tw_nlp; i++)
  //tw_lp_settype(i, &mylps[0]);

  printf("Here for the runs!\n");

  tw_run();

  tw_end();
  return 0;

}
