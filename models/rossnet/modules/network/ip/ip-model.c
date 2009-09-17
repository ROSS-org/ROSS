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
	int	 i;

	char	 log[1024];

	g_ip_stats = tw_calloc(TW_LOC, "", sizeof(ip_stats), 1);

	nbufs = 10000 / g_tw_nkp;
	nbufs = 1;

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

	for(i = 0; i < g_tw_nkp; i++)
		g_ip_fd = tw_kp_memory_init(tw_getkp(i), nbufs, sizeof(ip_message), 1);

	ip_routing_init();

	if(tw_ismaster())
	{
		printf("\nInitializing Model: IPv4\n");
		printf("\t%-50s %11d (%ld)\n", 
			"IP Membufs", nbufs, g_ip_fd);
	}
}

void
ip_md_final()
{
	if(!tw_ismaster())
		return;

	printf("\nIP Model Statistics:\n\n");
	printf("\t%-50s %11ld \n", "Total Packets Completed", g_ip_stats->s_ncomplete);
	printf("\t%-50s %11ld \n", "Total Packets Dropped", g_ip_stats->s_ndropped);
	printf("\t%-50s %11ld \n", "Packets Dropped at Source", 
		g_ip_stats->s_ndropped_source);
	printf("\t%-50s %11ld \n", "Packets Dropped (TTL)", g_ip_stats->s_ndropped_ttl);
	printf("\t%-50s %11ld \n", "Packets Dropped by Network Failure", 
		g_ip_stats->s_nnet_failures);
	printf("\t%-50s %11ld \n", "Packets Forwarded", g_ip_stats->s_nforward);
	printf("\n");
	printf("\t%-50s %11d \n", "Packet Avg TTL", 
		g_ip_stats->s_avg_ttl ? 
			(int) (g_ip_stats->s_avg_ttl / g_ip_stats->s_ncomplete) :
			(int) g_ip_stats->s_avg_ttl);
	printf("\t%-50s %11ld \n", "Packet Max TTL", g_ip_stats->s_max_ttl);
}
