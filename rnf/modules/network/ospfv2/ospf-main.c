#include <ospf.h>

#define VERIFY_OSPF_MAIN 0

void
ospf_md_opts()
{
	//tw_opt_add(ospf_options);
}

	/*
	 * Parse OSPF model specific command line arguments
	 */
void
ospf_setup_options(int argc, char ** argv)
{
	FILE	*init_file;

	int	 c;

	optind = 0;
	while(-1 != (c = getopt(argc, argv, "-o:y:")))
	{
		switch(c)
		{
			case 'y':
				g_ospf_link_weight = atoi(optarg);
				break;
			case 'o':
				g_ospf_init_file = (char *) calloc(255, 1);
				sprintf(g_ospf_init_file, "%s", optarg);
			break;
		}
	}

	if(!strcmp(g_rn_rt_table, ""))
		g_ospf_routing_file = fopen(g_rn_rt_table, "r");

	if(!g_ospf_routing_file)
	{
		char  file[255];

		g_ospf_rt_write = 1;

		sprintf(file, "%s/routing.file", g_rn_logs_dir);
		g_ospf_routing_file = fopen(file, "w");

		if(!g_ospf_routing_file)
			tw_error(TW_LOC, "Unable to open %s/routing.file!", 
				g_rn_logs_dir);

		printf("Creating routing table file: %s/routing.file \n",
				g_rn_logs_dir);
	} else
		printf("Reading routing table file: %s \n", g_rn_rt_table);

	g_ospf_state = tw_calloc(TW_LOC, "", sizeof(ospf_global_state), 1);

	if(NULL != g_ospf_init_file)
	{
		init_file = fopen(g_ospf_init_file, "r");

		if(!init_file)
			tw_error(TW_LOC, "Unable to open init file: %s",
				g_ospf_init_file);

		fscanf(init_file, "%lg %lg %lg %lg %d %lg", 
					&g_ospf_state->hello_sendnext,
					&g_ospf_state->hello_timer,
					&g_ospf_state->flood_timer,
					&g_ospf_state->ack_interval,
					&g_ospf_state->mtu,
					&g_ospf_state->rt_interval);

		fclose(init_file);

		g_ospf_state->poll_interval = 
			g_ospf_state->hello_sendnext * g_ospf_state->hello_timer;
	} else
	{
		g_ospf_init_file = calloc(255,1);
		sprintf(g_ospf_init_file, "default parameters");

		g_ospf_state->hello_sendnext = 4;
		g_ospf_state->hello_timer = 5;
		g_ospf_state->flood_timer = 1;
		g_ospf_state->ack_interval = 1;
		g_ospf_state->mtu = 1500;
		g_ospf_state->rt_interval = 0;

		g_ospf_state->poll_interval = 
					g_ospf_state->hello_sendnext *
					g_ospf_state->hello_timer;
	}

#if VERIFY_OSPF_MAIN || 1
	printf("\nOSPFv2 Initial State Variables: %s\n\n", g_ospf_init_file);
	printf("\t%-50s %11d\n", 
			"LINK WEIGHT CHANGE PROB", g_ospf_link_weight);
	printf("\t%-50s %11.2lf\n", 
			"OSPF_HELLO_SENDNEXT", g_ospf_state->hello_sendnext);
	printf("\t%-50s %11.2lf\n", 
			"OSPF_HELLO_TIMER", g_ospf_state->hello_timer);
	printf("\t%-50s %11.2lf\n", 
			"OSPF_POLL_INTERVAL", g_ospf_state->poll_interval);
	printf("\t%-50s %11.2lf\n", 
			"OSPF_FLOOD_TIMER", g_ospf_state->flood_timer);
	printf("\t%-50s %11.2lf\n", 
			"OSPF_ACK_INTERVAL", g_ospf_state->ack_interval);
	printf("\t%-50s %11.2lf\n", 
			"OSPF_RT_INTERVAL", g_ospf_state->rt_interval);
	printf("\t%-50s %11.2d\n", "MTU", g_ospf_state->mtu);
	printf("\n");
#endif
}

/*
 * Setup an area's g_ospf_lsa array with the proper LSAs for the start
 * state of the model.
 * 
 * To start non-converged, do nothing.
 * To start converged, must alloc all LSA membufs and fill them in according
 * to the start state of the topology.
 */
void
main_setup_areas(xmlXPathObjectPtr l_obj)
{
	xmlNodePtr	 layer;
	xmlNodePtr	 interface;

	tw_memoryq	*q;
	tw_memory	*b;
	ospf_lsa_link	*l;
	tw_lp		*lp;

	rn_area		*ar;
	rn_area		*d_ar;
	ospf_lsa	*lsa;

	int	i;
	int	j;
	int	x;
	int	id;
	int	src;
	int	dst;

	q = tw_memoryq_init();
	//q->type = q_memory_t;
	q->d_size = sizeof(ospf_lsa);
	q->grow = 1;

	// Simply allocate the g_ospf_lsa array for each area
	for(i = 0; i < g_rn_nareas; i++)
	{
		ar = &g_rn_areas[i];

		if(!ar->g_ospf_nlsa)
		{
			//printf("No machines for this area: %d \n", ar->id);
			continue;
		}
#if 0
		 else
			printf("Area %d has ft: %d \n", ar->id, ar->g_ospf_nlsa);
#endif

		ar->g_ospf_lsa = (tw_memory **) calloc(sizeof(tw_memory *) * 
						(ar->g_ospf_nlsa), 1);
	
		if(!ar->g_ospf_lsa)
			tw_error(TW_LOC, "Could not allocate global LSA table!");
#if VERIFY_OSPF_XML
		else
			printf("Allocated %d LSAs in global LSA array for area %d\n", 
					(ar->g_ospf_nlsa), ar->id);
#endif

		q->head = q->tail = NULL;
		q->start_size = ar->g_ospf_nlsa;
		tw_memory_allocate(q);

		for(j = 0; j < ar->nmachines; j++)
		{
			ar->g_ospf_lsa[j] = tw_memoryq_pop(q);

			lsa = tw_memory_data(ar->g_ospf_lsa[j]);
			lsa->type = ospf_lsa_router;
			lsa->adv_r = ar->low + j;
			lsa->id = j;
		}

		for(; j < ar->nmachines + ar->as->nareas; j++)
		{
			ar->g_ospf_lsa[j] = tw_memoryq_pop(q);

			lsa = tw_memory_data(ar->g_ospf_lsa[j]);
			lsa->type = ospf_lsa_summary3;
			lsa->adv_r = -1;
			lsa->id = j;
		}

		for(; j < ar->g_ospf_nlsa; j++)
		{
			ar->g_ospf_lsa[j] = tw_memoryq_pop(q);

			lsa = tw_memory_data(ar->g_ospf_lsa[j]);
			lsa->type = ospf_lsa_as_ext;
			lsa->adv_r = -1;
			lsa->id = j;
		}
	}

	/*
	 * LSA links are determined by the "interface" XML element, and not
	 * the topology.
	 */
	layer = *l_obj->nodesetval->nodeTab;
	for(i = 0; i < l_obj->nodesetval->nodeNr; layer = l_obj->nodesetval->nodeTab[++i])
	{
		for(interface = layer->children; interface; interface = interface->next)
		{
			if(0 != strcmp((char *) interface->name, "interface"))
				continue;

			src = atoi(xml_getprop(interface, "src"));
			dst = atoi(xml_getprop(interface, "addr"));

			lp = tw_getlp(src);
			ar = rn_getarea(rn_getmachine(src));
			d_ar = rn_getarea(rn_getmachine(dst));


#if VERIFY_OSPF_MAIN || 0
			printf("\nConfiguring LSA: src %d ar %d as %d, dst %d ar %d as %d\n", 
					src, ar->id, ar->as->id, 
					dst, d_ar->id, d_ar->as->id);
#endif

			// Add link to router, area, or AS LSA appropriately
			id = src - ar->low;

			if(ar->as != d_ar->as)
				id = ar->nmachines + ar->as->nareas + d_ar->as->id;
			else if(ar != d_ar)
				id = ar->nmachines + d_ar->id - d_ar->as->low;

			lsa = tw_memory_data(ar->g_ospf_lsa[id]);

			// Lowest router in area is default router
			if(lsa->adv_r == 0xffffffff)
				lsa->adv_r = src;
			else if(lsa->adv_r != src)
				continue;

			lsa->length += OSPF_LSA_LINK_LENGTH;
			b = tw_memory_alloc(lp, g_ospf_ll_fd);
			l = tw_memory_data(b);
			l->dst = dst;

			x = atoi(xml_getprop(interface, "cost"));
			l->metric = x;

			if(l->metric <= 0)
				l->metric = 1;

			l->metric = 50;
			tw_memoryq_push(&lsa->links, b);

#if 0
			printf("Area %d: ", ar->id);
			ospf_lsa_print(stdout, lsa);
#endif
		}
	}
}

/*
 * TODO: 
 * 
 * 1)This is still a global LSA array, which needs to be moved into
 * the individual areas.  I just don't want to do that work now.
 *
 * 2) We are over-allocating mem buffers .. do not need to do all of these
 * allocations if no OSPF nodes on a given KP
 */
void
ospf_main(int argc, char **argv, char **env)
{
	char		 file[255];

	xmlXPathObjectPtr	l_obj;

	g_ospf_stats = tw_calloc(TW_LOC, "", sizeof(ospf_statistics), 1);

	/*
	 * Determine the number of LSAs to create in the global LSA array
	 */

	// Need a better xpath expr to find only OSPF routers in my area!
	l_obj = xpath("//layer[@name=\'ospf\']", ctxt);

	if(l_obj->nodesetval->nodeNr < 1)
		return; //tw_error(TW_LOC, "Could not find any OSPF nodes!");

	if(sizeof(ospf_dd_pkt) < sizeof(ospf_lsa_link))
		tw_error(TW_LOC, "Cannot use DD membufs for LSA links!");

	/*
	 * Init the nemory queues needed by this model
	 */
	printf("\nInitializing OSPF memory buffers... ");
	tw_memory_init(20000, sizeof(ospf_db_entry), 1);

/*
	// Commented out for NCS
	g_ospf_fd = tw_kp_memory_init(tw_getkp(i), 
				400 * l_obj->nodesetval->nodeNr, 
				sizeof(ospf_db_entry), 1);
	g_ospf_lsa_fd = tw_kp_memory_init(tw_getkp(i), 
				100 * l_obj->nodesetval->nodeNr, 
				sizeof(ospf_lsa), 1);
	g_ospf_dd_fd = tw_kp_memory_init(tw_getkp(i), 
				1 * l_obj->nodesetval->nodeNr, 
				sizeof(ospf_dd_pkt), 1);
	g_ospf_ll_fd = tw_kp_memory_init(tw_getkp(i), 
				100 * l_obj->nodesetval->nodeNr, 
				sizeof(ospf_lsa_link), 1);
*/
	g_ospf_fd = tw_memory_init(2000000, sizeof(ospf_db_entry), 1);
	g_ospf_lsa_fd = tw_memory_init(2000000, sizeof(ospf_lsa), 1);
	g_ospf_dd_fd = tw_memory_init(100000, sizeof(ospf_dd_pkt), 1);
	g_ospf_ll_fd = tw_memory_init(1000000, sizeof(ospf_lsa_link), 1);

		//g_tw_kp[0].queues[g_ospf_fd].debug = 1;
		//g_tw_kp[0].queues[g_ospf_lsa_fd].debug = 1;
		//g_tw_kp[0].queues[g_ospf_dd_fd].debug = 1;
		//g_tw_kp[0].queues[g_ospf_ll_fd].debug = 1;

/*
		printf("\tDB  q: %10.0f bufs %10ld fd\n",
			(float) (100 * l_obj->nodesetval->nodeNr), g_ospf_fd);
		printf("\tLSA q: %10.0f bufs %10ld fd\n",
			(float) (100 * l_obj->nodesetval->nodeNr), g_ospf_lsa_fd);
		printf("\tDD  q: %10.0f bufs %10ld fd\n",
			(float) (1 * l_obj->nodesetval->nodeNr), g_ospf_dd_fd);
		printf("\tLL  q: %10.0f bufs %10ld fd\n",
			(float) (100 * l_obj->nodesetval->nodeNr), g_ospf_ll_fd);
*/
	printf("\tDB  q: %10.0f bufs %10ld fd\n",
		(float) 2000000.0, g_ospf_fd);
	printf("\tLSA q: %10.0f bufs %10ld fd\n",
		(float) 2000000, g_ospf_lsa_fd);
	printf("\tDD  q: %10.0f bufs %10ld fd\n",
		(float) 100000, g_ospf_dd_fd);
	printf("\tLL  q: %10.0f bufs %10ld fd\n",
		(float) 1000000, g_ospf_ll_fd);

	ospf_setup_options(argc, argv);

	if(g_rn_converge_ospf)
	{
		main_setup_areas(l_obj);
	} else
	{
		sprintf(file, "tools/%s/ospf-lsa.file", g_rn_tools_dir);
		g_ospf_lsa_input = fopen(file, "r");
	}

	xmlXPathFreeObject(l_obj);

	sprintf(file, "%s/ospf-lsa.file", g_rn_logs_dir);
	g_ospf_lsa_file = fopen(file, "w");

	if(!g_ospf_lsa_file)
		tw_error(TW_LOC, "Unable to open: %s", file);
}

void
ospf_md_final()
{
	rn_machine	*m;
	rn_area		*ar;

	int		 i;
	int		 j;

	printf("\nOSPFv2 Model Statistics: \n\n");

	printf("\t%-50s %11ld\n", "Hello Packets", g_ospf_stats->s_sent_hellos);
	printf("\t%-50s %11ld\n", "Hello: sent", g_ospf_stats->s_e_hello_out);
	printf("\t%-50s %11ld\n", "Hello: recv", g_ospf_stats->s_e_hello_in);
	printf("\n");
	printf("\t%-50s %11ld\n", "DD Packets", g_ospf_stats->s_sent_dds);
	printf("\t%-50s %11ld\n", "Dropped Packets", g_ospf_stats->dropped_packets);
	printf("\t%-50s %11ld\n", "Dropped DD Packets", g_ospf_stats->s_drop_dd);
	printf("\t%-50s %11ld\n", "LS Requests", g_ospf_stats->s_sent_ls_requests);
	printf("\t%-50s %11ld\n", "LS Updates", g_ospf_stats->s_sent_ls_updates);
	printf("\t%-50s %11ld\n", "LS Acks", g_ospf_stats->s_sent_ls_acks);
	printf("\t%-50s %11ld\n", "Unknown Packets", g_ospf_stats->s_sent_unknown);
	printf("\t%-50s %11ld\n", "Lost Packets", g_ospf_stats->s_sent_lost);
	printf("\n");
	printf("BGP Caused OSPF Updates: %ld\n", g_ospf_stats->s_cause_bgp);
	printf("OSPF Caused OSPF Updates: %ld\n", g_ospf_stats->s_cause_ospf);
	printf("\n");

	if(!g_ospf_routing_file)
	{
		printf("Unable to create routing file! \n");
		return;
	}

	for(i = 0; i < g_rn_nmachines; i++)
	{
		m = &g_rn_machines[i];
		ar = m->subnet->area;

		if(m->type != c_router)
			continue;

		//write(g_ospf_routing_file, m->ft, ar->g_ospf_nlsa * sizeof(unsigned int));
		for(j = 0; j < ar->g_ospf_nlsa; j++)
			fprintf(g_ospf_routing_file, "%d ", m->ft[j]);
		fprintf(g_ospf_routing_file, "\n");
	}

	//fclose(g_ospf_routing_file);

	if(g_ospf_lsa_file)
		; // fclose(g_ospf_lsa_file);
}
