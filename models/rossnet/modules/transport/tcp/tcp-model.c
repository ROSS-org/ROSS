#include <tcp.h>

void
tcp_md_init(int argc, char ** argv, char ** env)
{
	int	 i;

	g_tcp_stats = tw_calloc(TW_LOC, "", sizeof(*g_tcp_stats), 1);
	g_tcp_f = fopen("tcp.data", "w");

	if(!g_tcp_f)
		tw_error(TW_LOC, "Unable to open TCP validation file!");

	for(i = 0; i < g_tw_nkp; i++)
		g_tcp_fd = tw_kp_memory_init(tw_getkp(i), 1000000 / g_tw_nkp, 
					sizeof(tcp_message), 1);

	if(tw_ismaster())
	{
		printf("\nInitializing Model: TCP Tahoe\n");
		printf("\t%-50s %11d (%ld)\n", 
			"TCP Membufs", 1000000, g_tcp_fd);
	}
}

void
tcp_md_final()
{
	if(g_tcp_f)
		fclose(g_tcp_f);

	if(!tw_ismaster())
		return;

	printf("\nTCP Model Statistics: \n\n");
	printf("\t%-50s %11d\n", "Sent Packets", g_tcp_stats->sent);
	printf("\t%-50s %11d\n", "Recv Packets", g_tcp_stats->recv);
	printf("\t%-50s %11d\n", "Sent ACKs", g_tcp_stats->ack_sent);
	printf("\t%-50s %11d\n", "Recv ACKs", g_tcp_stats->ack_recv);
	printf("\t%-50s %11d\n", "Invalid ACKs", g_tcp_stats->ack_invalid);
	printf("\t%-50s %11d\n", "TimeOut Packets", g_tcp_stats->tout);
	printf("\t%-50s %11d\n", "Drop Packets", g_tcp_stats->dropped_packets);
	//printf("\t%-50s %11.4lf\n", "Throughput", g_tcp_stats->throughput);
}
