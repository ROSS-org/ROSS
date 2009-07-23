/*********************************************************************
		              tcp-main.c
*********************************************************************/

#include "tcp.h"


tw_lptype       mylps[] =
{
	{TW_TCP_HOST, sizeof(Host_State),
	 (init_f) tcp_host_StartUp,
	 (event_f) tcp_host_EventHandler,
	 (revent_f) tcp_host_rc_EventHandler,
	 (final_f) tcp_host_Statistics_CollectStats,
	 (statecp_f) NULL},
	{TW_TCP_ROUTER, sizeof(Router_State),
	 (init_f) tcp_router_StartUp,
	 (event_f) tcp_router_EventHandler,
	 (revent_f) tcp_router_rc_EventHandler,
	 (final_f) tcp_router_Statistics_CollectStats,
	 (statecp_f) NULL},
	{0},
};


int
main(int argc, char **argv)
{
  int             TWnlp;
  int             TWnkp;
  int             TWnpe;
  int             i,j,k,l,m,n;
  tw_lp          *lp;
  tw_kp          *kp;
  tw_pe          *pe;
  FILE           *config;

#ifdef LOGGING 
   char           file_name[30];
#endif

  if(argc < 4 ){ 
    printf("tcp: error: incorrect number of parameters\n");
    exit(0);
  }
  
  g_type = 0;
  if(strcmp(argv[1],"rocketfuel") == 0){
    if(argc != 6 ){ 
      printf("tcp: error: incorrect number of parameters\n");
      exit(0);
    }
    if(strcmp(argv[3],"load") == 0)
      tcp_init_rocketfuel_load(argv[4],argv[5]);
    else if (strcmp(argv[3],"build") == 0)
      tcp_init_rocketfuel_build(argv[4],argv[5]);
    else{
      printf("tcp: error: incorrect number of parameters\n");
      exit(0);
    }
  }
  else if(strcmp(argv[1],"dml") == 0){
    tcp_init(argv[3]);
  }
  else if(strcmp(argv[1],"simple") == 0){
    tcp_init_simple(argv[3]);
  }
  else if(strcmp(argv[1],"mapped") == 0){
      tcp_init_simple(argv[3]);
      tcp_init_mapped();
  }
  else {
    printf("tcp: error: incorrect number of parameters\n");
    exit(0);
  }
  
  //if(g_type > 0){
  // g_tw_events_per_pe = 4792 + 4092 ; //(pow(g_type,4) * 26 ) / g_npe;
  // g_tw_events_per_pe = 5376 + 4092 ; //(pow(g_type,4) * 26 ) / g_npe;
  // g_tw_events_per_pe = 45759 + 10092 ; //(pow(g_type,4) * 26 ) / g_npe;
  //  g_tw_events_per_pe = 85685 + 7092 ; //(pow(g_type,4) * 26 ) / g_npe;
  // g_tw_events_per_pe = 86016 + 7092 ; //(pow(g_type,4) * 26 ) / g_npe;
  // g_tw_events_per_pe = 522335 + 7092 ; //(pow(g_type,4) * 26 ) / g_npe;
  //  g_tw_events_per_pe = 1217929 + 7092 ; //(pow(g_type,4) * 26 ) / g_npe;
  // g_tw_events_per_pe = 1380021 + 7092 ; //(pow(g_type,4) * 26 ) / g_npe;
  //  g_tw_events_per_pe = 5273847 + 4092 ; //(pow(g_type,4) * 26 ) / g_npe;
  // g_tw_events_per_pe = 6876362 + 4092 ; //(pow(g_type,4) * 26 ) / g_npe;  
  //g_tw_events_per_pe = 13308192; //700316 + 7092 ; //(pow(g_type,4) * 26 ) / g_npe;  
  g_tw_events_per_pe = 1024 * 1024 +  pow(g_type,4) * 26; 
  /*
    if(g_type == 32){
    g_tw_events_per_pe = (pow(g_type,4) * 12 ) / g_npe + 890000;
    g_recv_wnd = 16;
    }
    }
    else if (g_type == -1 && g_num_links == 10)
    g_tw_events_per_pe = 1024 *1024 * 2.5 / g_npe;
    else if (g_type == -1 && g_num_links == 30)
    g_tw_events_per_pe = 1024 *1024 * 12 / g_npe;
    else
    g_tw_events_per_pe = 1024 * 1024 *2;
  */
  TWnkp = 1; //g_nkp; 
  TWnpe = g_npe;    
  TWnlp = g_hosts + g_routers;
  g_tw_ts_end = atof(argv[2]) * (double)g_frequency;
  g_tw_gvt_interval = 32;
  g_tw_mblock = 256;

  printf("Running simulation with following configuration: \n" );
  printf("    Processors Used = %d\n", TWnpe);
  printf("    KPs Used = %d\n", TWnkp);
  printf("    LPs Used = %d\n", TWnlp);
  printf("    End Time = %f \n", g_tw_ts_end);
  printf("    Buffers Allocated Per PE = %d\n", g_tw_events_per_pe);
  printf("    Gvt Interval = %d\n", g_tw_gvt_interval);
  printf("    Message Block Size (i.e., Batch) = %d\n", g_tw_mblock);
  printf("\n\n");
 

#ifdef LOGGING
  host_received_log = (FILE **) malloc(sizeof(FILE *));
  serv_tcpdump = (FILE **) malloc(sizeof(FILE *));
  serv_cwnd = (FILE **) malloc(sizeof(FILE *));
  host_sent_log = (FILE **) malloc(sizeof(FILE *));
  router_log = (FILE **) malloc(g_routers * sizeof(FILE *));
  for(j = 0; j<g_hosts; j++){
    sprintf(file_name,"logs/host_received.%d.log",j);
    host_received_log[j] = fopen(file_name,"w");  
    sprintf(file_name,"logs/serv_cwnd_%d.out",j);
    serv_cwnd[j] = fopen(file_name,"w");  
    sprintf(file_name,"logs/serv_tcpdump_%d.out",j);
    serv_tcpdump[j] = fopen(file_name,"w");  
    sprintf(file_name,"logs/host_sent.%d.log",j);
    host_sent_log[j] =  fopen(file_name,"w");
  }
  for(j = 0; j<g_routers; j++){
    sprintf(file_name,"logs/router.%d.log",j);
    router_log[j] =  fopen(file_name,"w");
  }
#endif

  tw_init(mylps, TWnpe, TWnkp, TWnlp, sizeof(Msg_Data));
  tcp_init_lps(argv[1]);
  

  // exit(0);
  /*
   * Initialize App Stats Structure 
   */
  TWAppStats.sent_packets = 0; 
  TWAppStats.received_packets = 0; 
  
  TWAppStats.dropped_packets = 0;
  TWAppStats.timedout_packets = 0;
  TWAppStats.throughput = 0;
 
  tw_run();
  
  tcpStatistics_Print(&TWAppStats);

  
#ifdef LOGGING
  for(j = 0; j<g_hosts; j++){
    fclose(serv_cwnd[j]);
    fclose(serv_tcpdump[j]);
    fclose(host_received_log[j]);
    fclose(host_sent_log[j]);
  }
  for(j = 0; j<g_routers; j++){
    fclose(router_log[j]);
  }
#endif
  
  return 0;
}


void 
tcpStatistics_Print(tcpStatistics *Stat)
{
  printf("Sent Packets.......................................%d\n",
	 Stat->sent_packets);
  printf("Received Packets...................................%d\n",
	 Stat->received_packets);
  printf("Timeout Packets....................................%d\n",
	 Stat->timedout_packets);
  printf("Dropped Packets....................................%d\n",
	 Stat->dropped_packets);
  	printf("Throughput.........................................%g\n",
	       (Stat->throughput / ( g_hosts / 2)) * 8);
}

















