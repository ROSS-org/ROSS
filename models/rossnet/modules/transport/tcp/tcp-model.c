#include <tcp.h>

const tw_optdef tcp_opts[] =
{
	TWOPT_GROUP("TCP Model"),
	TWOPT_END()
};

void
tcp_md_opts()
{
	tw_opt_add(tcp_opts);
}

void
tcp_md_init(int argc, char ** argv, char ** env)
{
	int	 nbufs;

	nbufs = 1000000 / g_tw_nkp;
	nbufs = 1;

	g_tcp_stats = tw_calloc(TW_LOC, "", sizeof(*g_tcp_stats), 1);

	g_tcp_fd = tw_memory_init(nbufs, sizeof(tcp_message), 1);

	if(tw_ismaster())
	{
		printf("\nInitializing Model: TCP Tahoe\n");
		printf("\t%-50s %11d (%ld)\n", 
			"TCP Membufs", nbufs, g_tcp_fd);
	}
}

void
tcp_md_final()
{
	tcp_statistics	stats;

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
