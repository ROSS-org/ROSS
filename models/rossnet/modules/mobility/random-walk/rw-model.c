#include <rw.h>

static const tw_optdef rw_options [] =
{
	TWOPT_UINT("rw-min", g_rw_mu, "random walk min change dir"),
	TWOPT_UINT("rw-max", g_rw_distr_sd, "random walk max change dir"),
	TWOPT_STIME("wave-percent", percent_wave, "percentage of waves per move"),
	TWOPT_END()
};

void
rw_md_opts()
{
	tw_opt_add(rw_options);
}
	
void
rw_md_init(int argc, char ** argv, char ** env)
{
	int	 nbufs;

	g_rw_stats = tw_calloc(TW_LOC, "", sizeof(*g_rw_stats), 1);

	nbufs = 1000000 / g_tw_nkp;
	nbufs = 10000 / g_tw_nkp;

	g_rw_fd = tw_memory_init(nbufs, sizeof(rw_message), 1);

	if(tw_ismaster())
	{
		printf("\nInitializing Model: Random Walk\n");
		printf("\t%-50s %11d (%ld)\n", 
			"Membufs Allocated", nbufs, g_rw_fd);
	}
}

void
rw_md_final()
{
	if(!tw_ismaster())
		return;

	printf("\nRandom Walk Model Statistics: \n\n");
	printf("\t%-50s %11.2lf\n", "Waves Percentage", percent_wave);
	printf("\t%-50s %11lld\n", "Number of Waves", g_rw_stats->s_nwaves);
	printf("\t%-50s %11lld\n", "Number of Moves", g_rw_stats->s_move_ev);

#if 0
	printf("\t%-50s %11ld\n", "Ttl PROX Events", g_rw_stats->s_prox_ev);
	printf("\t%-50s %11ld\n", "Ttl Recv Events", g_rw_stats->s_recv_ev);
	printf("\n");

	printf("PM Connections:\n\n");
	printf("\t%-42s %11ld\n", "Ttl Direct Conns",
						g_rw_stats->s_nconnect);
	printf("\t%-42s %11ld\n", "Ttl Direct Disconnects",
						g_rw_stats->s_ndisconnect);
	printf("\n");
#endif
}
