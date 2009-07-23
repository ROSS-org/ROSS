#include <bgp.h>

void
bgp_final(bgp_state * state, tw_lp * lp)
{
#ifndef BGP_CONVERGED
	bgp_route	*r;

#endif
	int		 i;

        bgp_rib_print(state, lp);
	
	g_bgp_stats.s_nroute_adds += state->stats->s_nroute_adds;
	g_bgp_stats.s_nroute_removes += state->stats->s_nroute_removes;

	g_bgp_stats.s_nupdates_sent += state->stats->s_nupdates_sent;
	g_bgp_stats.s_nupdates_recv+= state->stats->s_nupdates_recv;

	g_bgp_stats.s_nnotify_sent += state->stats->s_nnotify_sent;
	g_bgp_stats.s_nnotify_recv += state->stats->s_nnotify_recv;

	g_bgp_stats.s_nopens += state->stats->s_nopens;
	g_bgp_stats.d_opens += state->stats->d_opens;

	g_bgp_stats.s_nunreachable += state->stats->s_nunreachable;
	g_bgp_stats.s_nkeepalives += state->stats->s_nkeepalives;
	g_bgp_stats.s_nkeepalivetimers += state->stats->s_nkeepalivetimers;
	g_bgp_stats.s_nmrai_timers += state->stats->s_nmrai_timers;

	g_bgp_stats.s_nconnects += state->stats->s_nconnects;
	g_bgp_stats.s_nconnects_dropped += state->stats->s_nconnects_dropped;
	g_bgp_stats.s_nconnects_dropped += state->stats->s_nconnects_dropped;

	g_bgp_stats.s_cause_bgp += state->stats->s_cause_bgp;
	g_bgp_stats.s_cause_ospf += state->stats->s_cause_ospf;

	for(i = 0; i < state->n_interfaces; i++)
	{
		if(state->nbr[i].up == TW_FALSE)
		{
			if(rn_getas(rn_getmachine(state->nbr[i].id)) == state->as)
				g_bgp_stats.s_nibgp_nbrs++;
			else
				g_bgp_stats.s_nebgp_nbrs++;
		} else
		{
			if(rn_getas(rn_getmachine(state->nbr[i].id)) == state->as)
				g_bgp_stats.s_nibgp_nbrs_up++;
			else
				g_bgp_stats.s_nebgp_nbrs_up++;
		}
	}

#if VERIFY_BGP
	if(stdout != state->log)
		fclose(state->log);
#endif

#ifndef BGP_CONVERGED

	if(g_rn_converge_ospf)
		return;

	if(!g_bgp_rt_fd)
	{
		char file[255];

		sprintf(file, "%s/bgp-route.file", g_rn_logs_dir);
		g_bgp_rt_fd = fopen(file, "w");

		if(!g_bgp_rt_fd)
			tw_error(TW_LOC, "Unable to open: %s", file);
		else
			printf("%lld: opening %s\n", lp->id, file);
	}

	state->log = g_bgp_rt_fd;
	for(i = 0; i < g_rn_nas; i++)
	{
		if(state->rib[i] == NULL)
		{
			fprintf(state->log, "-1 \n");
		} else
		{
			r = tw_memory_data(state->rib[i]);

			bgp_route_print(state, r);
		}
	}
#endif
}
