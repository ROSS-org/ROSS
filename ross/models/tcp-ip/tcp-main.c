#include <tcp.h>

	/*
	 * Just a place to setup and handle command line options for this module
	 */
void
tcp_setup_options(int argc, char **argv)
{
}

void
tcp_main(int argc, char ** argv, char ** env)
{
	int	 i;

	g_tcp_stats = tw_vector_create(sizeof(tcp_statistics), 1);

	for(i = 0; i < g_tw_nkp; i++)
		g_tcp_fd = tw_kp_memory_init(tw_getkp(i), 655350, 
					sizeof(tcp_message), 1);

	tcp_setup_options(argc, argv);
}

void
tcp_md_final()
{
	printf("\nTCP Model Statistics: \n\n");
	printf("\t%-50s %11d\n", "Sent Packets", g_tcp_stats->sent);
	printf("\t%-50s %11d\n", "Recv Packets", g_tcp_stats->recv);
	printf("\t%-50s %11d\n", "Sent ACKs", g_tcp_stats->ack_sent);
	printf("\t%-50s %11d\n", "Recv ACKs", g_tcp_stats->ack_recv);
	printf("\t%-50s %11d\n", "Invalid ACKs", g_tcp_stats->ack_invalid);
	printf("\t%-50s %11d\n", "TimeOut Packets", g_tcp_stats->tout);
	printf("\t%-50s %11d\n", "Drop Packets", g_tcp_stats->dropped_packets);
	printf("\t%-50s %11.4lf\n", "Throughput", g_tcp_stats->throughput);
}
