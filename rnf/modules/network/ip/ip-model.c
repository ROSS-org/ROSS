#include <ip.h>

const tw_optdef ip_opts[] =
{
	TWOPT_GROUP("IPv4 Model"),
	TWOPT_UINT("ip-log", g_ip_log_on, "turn on logging"),
	TWOPT_END()
};

void
ip_md_opts()
{
	tw_opt_add(ip_opts);
}

void
ip_md_init(int argc, char ** argv, char ** env)
{
	int	 nbufs;

	char	 log[1024];

	g_ip_stats = tw_calloc(TW_LOC, "", sizeof(ip_stats), 1);

	nbufs = 1000000 / g_tw_nkp;

	// in secs..
	g_ip_minor_interval = 60;
	g_ip_major_interval = 86400;

	if(g_ip_log_on)
	{
		sprintf(log, "%s/ip.log", g_rn_logs_dir);
		g_ip_log = fopen(log, "w");

		if(!g_ip_log)
			tw_error(TW_LOC, "Unable to open: %s \n", log);
	}

	g_ip_fd = tw_memory_init(nbufs, sizeof(ip_message), 1);

	ip_routing_init();

	if(tw_ismaster())
	{
		printf("\nInitializing Model: IPv4\n");
		printf("\t%-50s %11d (%ld)\n", 
			"IP Membufs per KP", nbufs, g_ip_fd);
	}
}

void
ip_md_final()
{
	ip_stats	 stats;

	if(MPI_Reduce(&(g_ip_stats->s_ncomplete),
			&stats,
			8,
			MPI_LONG_LONG,
			MPI_SUM,
			g_tw_masternode,
			MPI_COMM_WORLD) != MPI_SUCCESS)
		tw_error(TW_LOC, "TCP Final: unable to reduce statistics");

	if(!tw_ismaster())
		return;

	printf("\nIP Model Statistics:\n\n");
	printf("\t%-50s %11lld \n", "Total Packets Completed", 
			stats.s_ncomplete);
	printf("\t%-50s %11lld \n", "Total Packets Dropped", 
			stats.s_ndropped);
	printf("\t%-50s %11lld \n", "Packets Dropped at Source", 
			stats.s_ndropped_source);
	printf("\t%-50s %11lld \n", "Packets Dropped (TTL)", 
			stats.s_ndropped_ttl);
	printf("\t%-50s %11lld \n", "Packets Dropped by Network Failure", 
			stats.s_nnet_failures);
	printf("\t%-50s %11lld \n", "Packets Forwarded", 
			stats.s_nforward);
	printf("\n");
	printf("\t%-50s %11lld \n", "Packet Avg TTL", 
			stats.s_avg_ttl ? 
				(tw_stat) (stats.s_avg_ttl / stats.s_ncomplete) :
				(tw_stat) stats.s_avg_ttl);
	printf("\t%-50s %11lld \n", "Packet Max TTL", stats.s_max_ttl);
}
