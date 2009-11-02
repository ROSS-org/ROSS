#include <ip.h>

void
ip_setup_options(int argc, char ** argv)
{
	char	 c;

	/*
	 * Default protocol settings
	 */
	g_ip_write = 0;

	optind = 0;
	while(-1 != (c = getopt(argc, argv, "-i")))
	{
		switch(c)
		{
			case 'i':
				g_ip_routing_simple = 1;
				break;
		}
	}

	if(g_ip_routing_simple)
		ip_routing_init();
}

void
ip_main(int argc, char ** argv, char ** env)
{
	int	 i;

	g_ip_stats = tw_vector_create(sizeof(ip_stats), 1);

	for(i = 0; i < g_tw_nkp; i++)
		g_ip_fd = tw_kp_memory_init(tw_getkp(i), 655350, sizeof(ip_message), 1);

	ip_setup_options(argc, argv);
}

void
ip_md_final()
{
	printf("\nIP Model Statistics:\n\n");
	printf("\t%-50s %11d \n", "Total Packets Completed", g_ip_stats->s_ncomplete);
	printf("\t%-50s %11d \n", "Total Packets Dropped", g_ip_stats->s_ndropped);
	printf("\t%-50s %11d \n", "Packets Dropped at Source", 
		g_ip_stats->s_ndropped_source);
	printf("\t%-50s %11d \n", "Packets Dropped (TTL)", g_ip_stats->s_ndropped_ttl);
	printf("\t%-50s %11d \n", "Packets Dropped by Network Failure", 
		g_ip_stats->s_nnet_failures);
	printf("\t%-50s %11d \n", "Packets Forwarded", g_ip_stats->s_nforward);
	printf("\n");
	printf("\t%-50s %11d \n", "Packet Avg TTL", 
		g_ip_stats->s_avg_ttl ? 
			(int) (g_ip_stats->s_avg_ttl / g_ip_stats->s_ncomplete) :
			(int) g_ip_stats->s_avg_ttl);
	printf("\t%-50s %11d \n", "Packet Max TTL", g_ip_stats->s_max_ttl);
}
