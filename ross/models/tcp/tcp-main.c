/*********************************************************************
		              tcp-main.c
*********************************************************************/

#include "tcp.h"


tw_lptype       mylps[] =
{
	{TW_TCP_HOST, sizeof(Host_State),
	 (init_f) tcp_host_startup,
	 (event_f) tcp_host_eventhandler,
	 (revent_f) tcp_host_rc_eventhandler,
	 (final_f) tcp_host_Statistics_CollectStats,
	 (statecp_f) NULL},
	{TW_TCP_ROUTER, sizeof(Router_State),
	 (init_f) tcp_router_startup,
	 (event_f) tcp_router_eventhandler,
	 (revent_f) tcp_router_rc_eventhandler,
	 (final_f) tcp_router_statistics_collectStats,
	 (statecp_f) NULL},
	{0},
};

const tw_optdef app_opt[] =
{
	TWOPT_GROUP("TCP Model"),
	TWOPT_STIME("remote-link", percent_remote, "desired remote TCP connectionsevent rate"),
	TWOPT_UINT("network-bits", nlp_per_model, "number of 2^bits is number of LPs in whole system"),
	TWOPT_STIME("mean", mean, "exponential distribution mean for timestamps"),
	TWOPT_STIME("mult", mult, "multiplier for event memory allocation"),
	TWOPT_STIME("lookahead", lookahead, "lookahead for links"),
	TWOPT_UINT("memory", optimistic_memory, "additional memory buffers"),
	TWOPT_CHAR("run", run_id, "user supplied run name"),
	TWOPT_END()
};

int
main(int argc, char **argv)
{
  tw_lp          *lp;
  tw_kp          *kp;
  tw_pe          *pe;

  tw_opt_add(app_opt);
  tw_init(&argc, &argv);

  if( lookahead > 1.0 )
    tw_error(TW_LOC, "Lookahead > 1.0 .. needs to be less\n");
  
  //reset mean based on lookahead
  mean = mean - lookahead;

  g_tw_memory_nqueues = 16; // give at least 16 memory queue event
  
  offset_lpid = g_tw_mynode * nlp_per_pe;
  ttl_lps = tw_nnodes() * g_tw_npe * nlp_per_pe;
  g_tw_events_per_pe = (mult * nlp_per_pe * g_phold_start_events) + 
    optimistic_memory;
  //g_tw_rng_default = TW_FALSE;
  g_tw_lookahead = lookahead;
  
  tw_define_lps(nlp_per_pe, sizeof(phold_message), 0);
  
  for(i = g_tw_mynode * ; i < g_tw_nlp; i++)
    tw_lp_settype(i, &mylps[0]);
  
  if( g_tw_mynode == 0 )
    {
      printf("Running simulation with following configuration: \n" );
      printf("    Processors Used = %d\n", g_tw_pe);
      printf("    KPs Used = %d\n", g_tw_nkp);
      printf("    LPs Used = %d\n", nlp_per_model);
      printf("    End Time = %f \n", g_tw_ts_end);
      printf("    Buffers Allocated Per PE = %d\n", g_tw_events_per_pe);
      printf("    Gvt Interval = %d\n", g_tw_gvt_interval);
      printf("    Message Block Size (i.e., Batch) = %d\n", g_tw_mblock);
      printf("\n\n");
    }

  TWAppStats.sent_packets = 0; 
  TWAppStats.received_packets = 0; 
  TWAppStats.dropped_packets = 0;
  TWAppStats.timedout_packets = 0;
  TWAppStats.throughput = 0;
 
  tw_run();
  tw_end();
  tcp_finalize( &TWAppStats );
  return 0;
}


void 
tcp_finalize();
{
  Tcp_Statistics  stats;
  // START HERE -- fix!!
  if(MPI_Reduce(&(g_tcp_stats->bad_msgs),
		&stats,
		8,
		MPI_LONG_LONG,
		MPI_SUM,
		g_tw_masternode,
		MPI_COMM_WORLD) != MPI_SUCCESS)
    tw_error(TW_LOC, "TCP Final: unable to reduce statistics");

  if(MPI_Reduce(&(g_tcp_stats->throughput),
		&(stats.throughput),
		3,
		MPI_DOUBLE,
		MPI_SUM,
		g_tw_masternode,
		MPI_COMM_WORLD) != MPI_SUCCESS)
    tw_error(TW_LOC, "TCP Final: unable to reduce statistics");

  if(!tw_ismaster())
    return;

  printf("\nTCP Model Statistics: \n\n");
  printf("\t%-50s %11lld\n", "Sent Packets", stats.sent);
  printf("\t%-50s %11lld\n", "Recv Packets", stats.recv);
  printf("\t%-50s %11lld\n", "Sent ACKs", stats.ack_sent);
  printf("\t%-50s %11lld\n", "Recv ACKs", stats.ack_recv);
  printf("\t%-50s %11lld\n", "Invalid ACKs", stats.ack_invalid);
  printf("\t%-50s %11lld\n", "TimeOut Packets", stats.tout);
  printf("\t%-50s %11lld\n", "Drop Packets", stats.dropped_packets);
  printf("\t%-50s %11.4lf Kbps\n", "Total Throughput", stats.throughput);
}
~                                                                                                                                                                       
~                                                                                                                                                                       
~               
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

















