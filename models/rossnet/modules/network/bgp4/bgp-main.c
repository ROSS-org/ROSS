#include <bgp.h>

void
bgp_md_opts()
{
	//tw_opt_add(bgp_opts);
}

	/*
	 * Parse BGP model specific command line arguments
	 */
void
bgp_setup_options(int argc, char ** argv)
{
	int	 i;
	int	 c;

	optind = 0;
	while(-1 != (c = getopt(argc, argv, "-b:")))
	{
		switch(c)
		{
			case 'b':
				g_bgp_init_file = (char *) calloc(255, 1);
				sprintf(g_bgp_init_file, "%s", optarg);
			break;
		}
	}

	g_bgp_state = (bgp_state *) calloc(sizeof(bgp_state), 1);

	g_bgp_as = (bgp_as *) calloc(sizeof(bgp_as) * g_rn_nas, 1);

	if(!g_bgp_as)
		tw_error(TW_LOC, "Out of memory!");

	for(i = 0; i < g_rn_nas; i++)
		g_bgp_as[i].degree = (int *) calloc(sizeof(int), g_rn_nas);

	g_bgp_state->b.local_pref = 1;
	g_bgp_state->b.as_path = 1;
	g_bgp_state->b.origin = 1;
	g_bgp_state->b.med = 1;
	g_bgp_state->b.hot_potato = 1;
	g_bgp_state->b.next_hop = 1;
	g_bgp_state->b.existing = 1;

	if(NULL != g_bgp_init_file)
	{
		FILE	*init_file;
		int	 lp;
		int	 ap;
		int	 or;
		int	 med;
		int	 hp;
		int	 nh;
		int	 ex;

		init_file = fopen(g_bgp_init_file, "r");

		if(!init_file)
			tw_error(TW_LOC, "Unable to open BGP4 init file: %s",
				g_bgp_init_file);

		fscanf(init_file, "%lf", &g_bgp_state->keepalive_interval);
		fscanf(init_file, "%lf", &g_bgp_state->hold_interval);
		fscanf(init_file, "%lf", &g_bgp_state->mrai_interval);

		fscanf(init_file, "%d", &lp);
		fscanf(init_file, "%d", &ap);
		fscanf(init_file, "%d", &or);
		fscanf(init_file, "%d", &med);
		fscanf(init_file, "%d", &hp);
		fscanf(init_file, "%d", &nh);
		fscanf(init_file, "%d", &ex);

		g_bgp_state->b.local_pref = lp;
		g_bgp_state->b.as_path = ap;
		g_bgp_state->b.origin = or;
		g_bgp_state->b.med = med;
		g_bgp_state->b.hot_potato = hp;
		g_bgp_state->b.next_hop = nh;
		g_bgp_state->b.existing = ex;

		for(i = 0; i < g_rn_nas; i++)
			fscanf(init_file, "%d", &g_bgp_as[i].med);

		for(i = 0; i < g_rn_nas; i++)
			fscanf(init_file, "%d", &g_bgp_as[i].local_pref);

		for(i = 0; i < g_rn_nas; i++)
			fscanf(init_file, "%d", &g_bgp_as[i].path_padding);

		printf("\nBGP Model Variables: %s \n\n", g_bgp_init_file);
		printf("\t%-50s %-11lf\n", "KeepAlive Interval", 
						g_bgp_state->keepalive_interval);
		printf("\t%-50s %-11lf\n", "Hold Interval", 
						g_bgp_state->hold_interval);
		printf("\t%-50s %-11lf\n", "MRAI", g_bgp_state->mrai_interval);
		printf("\nBGP Decision Algorithm (on=1, off=0): \n");
		printf("\t%-50s %-11d\n", "Local Pref", g_bgp_state->b.local_pref);
		printf("\t%-50s %-11d\n", "AS Path", g_bgp_state->b.as_path);
		printf("\t%-50s %-11d\n", "Origin", g_bgp_state->b.origin);
		printf("\t%-50s %-11d\n", "MED", g_bgp_state->b.med);
		printf("\t%-50s %-11d\n", "Hot Potato", g_bgp_state->b.hot_potato);
		printf("\t%-50s %-11d\n", "Next Hop", g_bgp_state->b.next_hop);
		printf("\t%-50s %-11d\n", "Default", g_bgp_state->b.existing);
		printf("\n");

		fclose(init_file);
	}
}

void
bgp_setup_ases()
{
	rn_as		*as = NULL;

	char		expr[255];
	int		i;

	g_bgp_ases = (xmlXPathObjectPtr *) calloc(sizeof(xmlXPathObjectPtr), g_rn_nas);

	if(!g_bgp_ases)
		tw_error(TW_LOC, "Out of memory!");

	i = 0;
	while(NULL != (as = rn_nextas(as)))
	{
		as->keepalive_interval = g_bgp_state->keepalive_interval;
		as->hold_interval = g_bgp_state->hold_interval;
		as->mrai_interval = g_bgp_state->mrai_interval;

		sprintf(expr, 
			"/rossnet/as[@id=\'%d\']/area/subnet/node/stream[@port=\'169\']", 
			as->id);

		g_bgp_ases[as->id] = xpath(expr, ctxt);

		printf("\tAS %d: iBGP nbr load: %d^2 = %d, MED %d, LOCAL_PREF %d, AS PATH Padding %d\n",
			as->id, g_bgp_ases[as->id]->nodesetval->nodeNr,
			g_bgp_ases[as->id]->nodesetval->nodeNr *
			g_bgp_ases[as->id]->nodesetval->nodeNr, 
			g_bgp_as[as->id].med, 
			g_bgp_as[as->id].local_pref,
			g_bgp_as[as->id].path_padding);
	}

	printf("\n");
}

void
bgp_main(int argc, char **argv, char **env)
{
	/*
	 * Init the memory buffers for this model
	 */
#if BGP_CONVERGED
	char		file[1024];
#endif

	printf("\nInitializing BGP memory buffers... ");

/*
	// Commented out for NCS
	g_bgp_fd_rtes = tw_kp_memory_init(kp, 1000 * g_tw_nlp, sizeof(bgp_route), 1);
	g_bgp_fd_asp = tw_kp_memory_init(kp, 1000 * g_tw_nlp, sizeof(int), 1);
	g_bgp_fd = tw_kp_memory_init(kp, 2000 * g_tw_nlp, sizeof(bgp_message), 1);
*/
	g_bgp_fd_rtes = tw_memory_init(g_tw_nlp, sizeof(bgp_route), 1);
	g_bgp_fd_asp = tw_memory_init(g_tw_nlp, sizeof(int), 1);
	g_bgp_fd = tw_memory_init(g_tw_nlp, sizeof(bgp_message), 1);

	printf("done!\n");

/*
	printf("\tRoute   q: %10ld bufs %10ld fd\n", 1000 * g_tw_nlp, g_bgp_fd_rtes);
	printf("\tAS Path q: %10ld bufs %10ld fd\n", 1000 * g_tw_nlp, g_bgp_fd_asp);
	printf("\tMessage q: %10ld bufs %10ld fd\n", 1000 * g_tw_nlp, g_bgp_fd);
*/
	printf("\tRoute   q: %10lld bufs %10ld fd\n", g_tw_nlp, g_bgp_fd_rtes);
	printf("\tAS Path q: %10lld bufs %10ld fd\n", g_tw_nlp, g_bgp_fd_asp);
	printf("\tMessage q: %10lld bufs %10ld fd\n", g_tw_nlp, g_bgp_fd);
	printf("\n");

	bgp_setup_options(argc, argv);
	bgp_setup_ases();

#if BGP_CONVERGED
	sprintf(file, "tools/%s/bgp-route.file", g_rn_tools_dir);
	g_bgp_rt_fd = fopen(file, "r");

	if(!g_bgp_rt_fd)
		tw_error(TW_LOC, "Unable to open BGP routing file: %s", file);
#else
/*
	sprintf(file, "%s/bgp-route.file", g_rn_logs_dir);
	g_bgp_rt_fd = fopen(file, "w");

	if(!g_bgp_rt_fd)
		tw_error(TW_LOC, "Unable to open file: %s!", file);
*/
#endif
}

void
bgp_md_final()
{
	int	 i;
	int	 j;

	printf("\nBGP4 Model Statistics: \n\n");

	printf("\t%-50s %11d\n", "Connects", g_bgp_stats.s_nconnects);
	printf("\t%-50s %11d\n", "Connects Dropped", g_bgp_stats.s_nconnects_dropped);
	printf("\n");
	printf("\t%-50s %11d\n", "Opens", g_bgp_stats.s_nopens);
	printf("\t%-50s %11d\n", "D Opens", g_bgp_stats.d_opens);
	printf("\n");
	printf("\t%-50s %11d\n", "Routes Added", g_bgp_stats.s_nroute_adds);
	printf("\t%-50s %11d\n", "Routes Removed", g_bgp_stats.s_nroute_removes);
	printf("\n");
	printf("\t%-50s %11d\n", "Updates Sent", g_bgp_stats.s_nupdates_sent);
	printf("\t%-50s %11d\n", "Updates Recv", g_bgp_stats.s_nupdates_recv);
	printf("\t%-50s %11d\n", "Update Unreachable", g_bgp_stats.s_nunreachable);
	printf("\n");
	printf("\t%-50s %11d\n", "Notify Sent", g_bgp_stats.s_nnotify_sent);
	printf("\t%-50s %11d\n", "Notify Recv", g_bgp_stats.s_nnotify_recv);
	printf("\n");
	printf("\t%-50s %11d\n", "Keepalives", g_bgp_stats.s_nkeepalives);
	printf("\t%-50s %11d\n", "Keepalive Timers", g_bgp_stats.s_nkeepalivetimers);
	printf("\n");
	printf("\t%-50s %11d\n", "MRAI Timers", g_bgp_stats.s_nmrai_timers);
	printf("\n");
	printf("\t%-50s %11d\n", "iBGP Neighbors Down", g_bgp_stats.s_nibgp_nbrs);
	printf("\t%-50s %11d\n", "eBGP Neighbors Down", g_bgp_stats.s_nebgp_nbrs);
	printf("\n");
	printf("\t%-50s %11d\n", "iBGP Neighbors Up", g_bgp_stats.s_nibgp_nbrs_up);
	printf("\t%-50s %11d\n", "eBGP Neighbors Up", g_bgp_stats.s_nebgp_nbrs_up);
	printf("\n");
	printf("\t%-50s %11d\n", "Decision: Local Pref", g_bgp_stats.s_ndec_local_pref);
	printf("\t%-50s %11d\n", "Decision: AS Path", g_bgp_stats.s_ndec_aspath);
	printf("\t%-50s %11d\n", "Decision: Origin", g_bgp_stats.s_ndec_origin);
	printf("\t%-50s %11d\n", "Decision: MED", g_bgp_stats.s_ndec_med);
	printf("\t%-50s %11d\n", "Decision: Hot Potato", g_bgp_stats.s_ndec_hot_potato);
	printf("\t%-50s %11d\n", "Decision: Next Hop", g_bgp_stats.s_ndec_next_hop);
	printf("\t%-50s %11d\n", "Decision: Existing", g_bgp_stats.s_ndec_existing);
	printf("\t%-50s %11d\n", "Decision: Default", g_bgp_stats.s_ndec_default);
	printf("\n");
	printf("BGP Caused Updates: %d\n", g_bgp_stats.s_cause_bgp);
	printf("OSPF Caused BGP Updates: %d", g_bgp_stats.s_cause_ospf);
	printf("\n");

	printf("BGP4 Topology: \n\n");

	for(i = 0; i < g_rn_nas; i++)
	{
		for(j = 0; j < g_rn_nas; j++)
			printf("%10d ", g_bgp_as[i].degree[j]);

		printf("\n");
	}

	if(0 && g_bgp_rt_fd)
		fclose(g_bgp_rt_fd);
}
