#include <rossnet.h>
#include <rn_config.h>

/*
 * Each RN lp is an instance of a machine which employs the OSI model.
 * The modules implement the functionality of the OSI model layers which
 * have been defined in the ./configure step.
 *
 * The functional link between the models and the RN core is the rn_lptype.
 * Each rn_lptype will register functions for the init, event_handler, RC
 * event handler and final routines.  The RN lp will call these functions
 * but impose on them the OSI model structure.  In this way the layers will
 * be reproduced in the proper order.  This will enforce the upstream/downstream
 * motion of packets within a machine, including de/muxing.
 * 
 * The RN lp will also be responsible for transmitting lateral rn_messages, which
 * are defined to be rn_messages between RN machines.  When the rn_message is at the
 * bottom-most layer of the OSI model, the RN lp will then handle the sending of
 * remote rn_message to another RN lp.  This will model the sending of packets between
 * two machines on a network, thus completing the physical layer of the 
 * OSI model.  This should also ensure
 * that the module development integration is smooth as individual modules should
 * not be sending packets directly, which is hard for them to do anyway because they
 * will never know the bottom-most layer type on the remote  machine.  The RN lp
 * will not know this either.  In fact, it will simply send an RN rn_message to the
 * other RN lp, and the other RN lp will be responsible for handling that rn_message's
 * upstream motion on the remote machine.
 *
 * This will facilitate the different layering on different types of 
 * machines on the network: hosts, routers, gateways, and future, unforeseen machines.
 * This will also give the ability for any type of machine to fill in the entire
 * OSI model as each chooses.  For example, a gateway running BGP/TCP would fill
 * in the appropriate layers, but could fill in additional layers, such as an 
 * experimental application running above the BGP/TCP stack working on the BGP
 * data.
 */

rn_lptype       layer_types[] = {

#ifdef HAVE_RANDOM_WALK_H
	{
	 (init_f) rw_init,
	 (event_f) rw_event_handler,
	 (revent_f) rw_rc_event_handler,
	 (final_f) rw_final,
	 (map_f) NULL,
	 sizeof(rw_state),
	 "random-walk",
	 (rn_xml_init_f) rw_xml,
	 (md_opt_f) rw_md_opts,
	 (md_init_f) rw_md_init,
	 (md_final_f) rw_md_final}
	,
#endif
#ifdef HAVE_TLM_H
	{
	 (init_f) _tlm_init,
	 (event_f) _tlm_event_handler,
	 (revent_f) _tlm_rc_event_handler,
	 (final_f) _tlm_final,
	 (map_f) NULL,
	 sizeof(tlm_state),
	 "tlm",
	 (rn_xml_init_f) tlm_xml,
	 (md_opt_f) tlm_md_opts,
	 (md_init_f) tlm_md_init,
	 (md_final_f) tlm_md_final}
	,
#endif
#ifdef HAVE_OSPF_H
	{
	 (init_f) ospf_startup,
	 (event_f) ospf_event_handler,
	 (revent_f) ospf_rc_event_handler,
	 (final_f) ospf_statistics_collection,
	 (map_f) NULL,
	 sizeof(ospf_state),
	 "ospf",
	 (rn_xml_init_f) ospf_xml,
	 (md_opt_f) ospf_md_opts,
	 (md_init_f) ospf_main,
	 (md_final_f) ospf_md_final}
	,
#endif
#ifdef HAVE_EPI_H
	{
	 (init_f) epi_init,
	 (event_f) epi_event_handler,
	 (revent_f) epi_rc_event_handler,
	 (final_f) epi_final,
	 (map_f) NULL,
	 sizeof(epi_state),
	 "epi",
	 (rn_xml_init_f) epi_xml,
	 (md_opt_f) epi_md_opts,
	 (md_init_f) epi_main,
	 (md_final_f) epi_md_final}
	,
#endif
#ifdef HAVE_NUM_H
	{
	 (init_f) num_init,
	 (event_f) num_event_handler,
	 (revent_f) num_rc_event_handler,
	 (final_f) num_final,
	 (map_f) NULL,
	 sizeof(num_state),
	 "num",
	 (md_opt_f) num_md_opts,
	 (rn_xml_init_f) num_xml,
	 (md_init_f) num_main,
	 (md_final_f) num_md_final}
	,
#endif
#ifdef HAVE_BGP_H
	{
	 (init_f) bgp_init,
	 (event_f) bgp_event_handler,
	 (revent_f) NULL,
	 (final_f) bgp_final,
	 (map_f) NULL, 
	 sizeof(bgp_state),
	 "bgp",
	 (rn_xml_init_f) bgp_xml,
	 (md_opt_f) bgp_md_opts,
	 (md_init_f) bgp_main,
	 (md_final_f) bgp_md_final}
	,
#endif
#ifdef HAVE_TCP_H
	{
	 (init_f) tcp_init,
	 (event_f) tcp_event_handler,
	 (revent_f) tcp_rc_event_handler,
	 (final_f) tcp_final,
	 (map_f) NULL,
	 sizeof(tcp_state),
	 "tcp",
	 (rn_xml_init_f) tcp_xml,
	 (md_opt_f) tcp_md_opts,
	 (md_init_f) tcp_md_init,
	 (md_final_f) tcp_md_final}
	,
#endif
#ifdef HAVE_MULTICAST_H
	{
	 (init_f) McastStartUp,
	 (event_f) McastEventHandler,
	 (revent_f) McastRcEventHandler,
	 (final_f) McastCollectStats,
	 (map_f) NULL,
	 sizeof(NodeState),
	 "mcast",
	 (rn_xml_init_f) McastXml,
	 (md_opt_f) NULL,
	 (md_init_f) NULL,
	 (md_final_f) NULL}
	,
#endif
#ifdef HAVE_IP_H
	{
	 (init_f) ip_init,
	 (event_f) ip_event_handler,
	 (revent_f) ip_rc_event_handler,
	 (final_f) ip_final,
	 (map_f) NULL,
	 sizeof(ip_state),
	 "ip",
	 (rn_xml_init_f) NULL,
	 (md_opt_f) ip_md_opts,
	 (md_init_f) ip_md_init,
	 (md_final_f) ip_md_final}
	,
#endif
#ifdef HAVE_PHOLD_H
	{
	 (init_f) phold_init,
	 (event_f) phold_event_handler,
	 (revent_f) phold_event_handler_rc,
	 (final_f) phold_finish,
	 (map_f) NULL,
	 sizeof(phold_state),
	 "phold",
	 (rn_xml_init_f) phold_xml,
	 (md_opt_f) phold_md_opts,
	 (md_init_f) NULL,
	 (md_final_f) NULL}
	,
#endif
	{0}
	,
};

	/*
	 * Init the rn lp, and then each of the protocol layer lps for this node.
	 * Beyond setting up the ISO/OSI model, we also need to setup the star model.
	 */
unsigned int    g_rn_memory = 0;

tw_peid
rn_map(tw_lpid gid)
{
	if(gid / g_tw_nlp >= tw_nnodes())
		tw_error(TW_LOC, "Bad dest_pe: %lld / %lld = %lld >= %d\n",
			 gid, g_tw_nlp, gid / g_tw_nlp, tw_nnodes());

	return (tw_peid) gid / g_tw_nlp;
}

void
rn_startup(rn_lp_state * state, tw_lp * lp)
{
	/* Our machine gives us all the configurables from the XML topology */
	rn_machine     *m = rn_getmachine(lp->gid);

#if RN_TIMING
	tw_wtime        start;
	tw_wtime        end;
	tw_wtime        run;

	tw_wtime        xml_start;
	tw_wtime        xml_end;
	tw_wtime        xml_run;

	tw_wtime        init_start;
	tw_wtime        init_end;
	tw_wtime        init_run;

	int             memory;

	tw_wall_now(&start);
	memory = sizeof(rn_lp_state);
#endif

	if(lp->gid > g_rn_nmachines-1)
		return;

#if 0
	if(0 == lp->gid % 100000)
		printf("Initializing RN lp %d \n", (int) lp->gid);
#endif

	rn_setup_streams(lp->cur_state, layer_types, lp);
	rn_setup_mobility(lp->cur_state, layer_types, lp);
	m->xml = NULL;

	//rn_link_random_changes(lp);
	rn_setup_links(state, lp);
	rn_init_streams(state, lp);

#if 0
	if(lp->id == g_tw_nlp - 1)
		printf("Link status changes: %d \n", g_rn_nlink_status);
#endif

#if RN_TIMING
	//memory += sizeof(tw_lp) * m->nlayers;
	tw_wall_now(&end);
	tw_wall_sub(&run, &end, &start);
	tw_wall_sub(&xml_run, &xml_end, &xml_start);
	tw_wall_sub(&init_run, &init_end, &init_start);
	g_rn_memory += memory;

	if (lp->gid % 50 == 0)
	{
		printf
			("rn lp %d: Done with startup, xml %f secs, init %f secs, overall %f secs\n",
			 lp->id, tw_wall_to_double(&xml_run), tw_wall_to_double(&init_run),
			 tw_wall_to_double(&run));
		printf
			("rn lp %d: memory used for this lp %d, overall to now %d \n\n",
			 lp->id, memory, g_rn_memory);
	}
#endif

	return;
}

	/*
	 * The forward computation event-handler routine
	 */
void
rn_event_handler(rn_lp_state * state, tw_bf * bf, rn_message * msg, tw_lp * lp)
{
#if DYNAMIC_LINKS
	tw_event	*e;
#endif
	tw_lp		*l;

#if DYNAMIC_LINKS
	rn_message	*m;
#endif
	rn_stream	*cur_stream;

	state->l_stats.s_nevents_processed++;

	/*
	 * Timers need to be sent directly to the layer which set them
	 */
	if (msg->type == TIMER)
	{
		state->cev = NULL;
		state->cur_lp = msg->timer;
		cur_stream = rn_getstream_byport(state, msg->port);
		cur_stream->cur_layer = msg->size;

#if VERIFY_RN
		printf("\n%lld RN: TIMER src %lld, port %d, ts %lf \n\n",
				lp->gid, msg->src, msg->port, tw_now(lp));
#endif

		(*msg->timer->type.event) (msg->timer->cur_state, bf, msg, lp);

		return;
	} else if(msg->type == DIRECT)
	{
		// handled in the same way as TIMER events
		state->cev = NULL;
		state->cur_lp = msg->timer;

		if(NULL != (cur_stream = rn_getstream_byport(state, msg->port)))
			cur_stream->cur_layer = msg->size;

#if VERIFY_RN
		printf("\n%lld RN: DIRECT src %lld, port %d, ts %lf \n\n",
				lp->gid, msg->src, msg->port, tw_now(lp));
#endif

		//(*msg->timer->type.event) (msg->timer->cur_state, bf, msg, lp);
		(*state->cur_lp->type.event) (state->cur_lp->cur_state, bf, msg, lp);

		return;

	} else if(msg->type == LINK_STATUS ||
		  msg->type == LINK_STATUS_DOWN ||
		  msg->type == LINK_STATUS_UP ||
		  msg->type == LINK_STATUS_WEIGHT)
	{
		rn_link	*l;

#if VERIFY_RN && DYNAMIC_LINKS
		printf("%lld: link to %lld status change at %lf: %d -> %d \n",
			lp->gid, 
			g_rn_machines[lp->gid].link[msg->port].wire->id,
			tw_now(lp),
			g_rn_machines[lp->gid].link[msg->port].status,
			msg->type);
#endif

		l = &g_rn_machines[lp->gid].link[msg->port];

		if(msg->type == LINK_STATUS_WEIGHT)
		{
			tw_error(TW_LOC, "Link cost disabled!");
			//l->cost = tw_rand_integer(lp->id, 1, 100);
		} 
#if DYNAMIC_LINKS
		else if(msg->type == LINK_STATUS_DOWN)
		{
			l->status = rn_link_down;
		} else if(msg->type == LINK_STATUS_UP)
		{
			l->status = rn_link_up;
		} else
		{
			if(l->status == rn_link_up)
				l->status = rn_link_down;
			else
				l->status = rn_link_up;
		}

		l->last_status = tw_now(lp);
		l->next_status = INT_MAX;

		/*
		 * If mode != once, then need to send the next 
		 * status change event here!
		 */

		// If we took the link down, then bring it back up again
		// a short time later
		if(0 && msg->type == LINK_STATUS_DOWN)
		{
			rn_link *ol = rn_getlink(l->wire, lp->gid);

			if(ol->next_status > tw_now(lp))
				l->next_status = ol->next_status;
			else
				l->next_status = tw_rand_integer(lp->rng, floor(tw_now(lp)),
							 floor(tw_now(lp)) + 100);

			e = tw_event_new(lp, l->next_status - tw_now(lp), lp);
			m = tw_event_data(e);

			m->timer = NULL;
			m->port = msg->port;
			m->src = lp->gid;
			m->dst = lp->gid;
	
			m->type = LINK_STATUS_UP;

			tw_event_send(e);

			printf("%ld: bringing link to %d up at time %lf (ns: %lf), now %lf\n", 
				lp->gid, l->addr, e->recv_ts, l->next_status, tw_now(lp));
		}
#endif

		return;
	}

	if(msg->type != UPSTREAM)
		tw_error(TW_LOC, "Did not get upstream event!");

	if(!lp->pe->cur_event->memory)
		tw_error(TW_LOC, "no membuf on event!");

	/*
	 * This is where we traverse the star model:
	 *
	 * We need to start by finding the LP(s) which are on the port,
	 * and the send up the list based on the port number.. I suppose
	 * multiple layers could be using the same port at the same level,
	 * so we need to handle that also.
	 */
	cur_stream = rn_getstream_byport(state, msg->port);

	// set this to keep new events from being created on the
	// way up the stack.. want this event to be the sent event
	state->cev = lp->pe->cur_event;
	l = &cur_stream->layers[cur_stream->nlayers-1].lp;
	cur_stream->cur_layer = cur_stream->nlayers - 1;
	state->cur_lp = l;

	// if only one layer.. 
	if(cur_stream->cur_layer == 0)
		state->cev = NULL;

#if VERIFY_RN
	printf("\n%lld RN: UPSTREAM src %lld, ts %lf (%ld)\n",
			lp->gid, msg->src, tw_now(lp), (long int) lp->pe->cur_event);
#endif

	(*l->type.event) (l->cur_state, bf, msg, lp);
}

	/*
	 * The reverse computation event-handler routine
	 */
void
rn_rc_event_handler(rn_lp_state * state, tw_bf * bf, rn_message * msg, tw_lp * lp)
{
	tw_event	*cev;
	tw_lp		*l;

	rn_stream	*cur_stream;

	state->l_stats.s_nevents_rollback++;

	if (msg->type == TIMER)
	{
		state->cev = NULL;
		state->cur_lp = msg->timer;
	
		cur_stream = rn_getstream_byport(state, msg->port);
		cur_stream->cur_layer = msg->size;

		(*msg->timer->type.revent) (msg->timer->cur_state, bf, msg, lp);

		return;
	} else if(msg->type == DIRECT)
	{
		state->cev = NULL;
		state->cur_lp = msg->timer;
	
		if(NULL != (cur_stream = rn_getstream_byport(state, msg->port)))
			cur_stream->cur_layer = msg->size;

		(*state->cur_lp->type.revent) (state->cur_lp->cur_state, bf, msg, lp);

		return;
	} else if(msg->type == LINK_STATUS)
	{
		tw_error(TW_LOC, "Unhandled: RC of LINK status changes");
	}

	if(msg->type != UPSTREAM)
		tw_error(TW_LOC, "Did not get upstream event!");

	cur_stream = rn_getstream_byport(state, msg->port);
	l = state->cur_lp = &cur_stream->layers[cur_stream->nlayers-1].lp;
	cur_stream->cur_layer = cur_stream->nlayers - 1;

#if VERIFY_RN
	printf("\n%lld RN: RC_UPSTREAM src %lld, ts %lf (%ld)\n",
		lp->gid, msg->src, tw_now(lp), (long int) lp->pe->cur_event);
#endif

	(*l->type.revent) (l->cur_state, bf, msg, lp);

	// reverse timestamp on membufs to now
	cev = lp->pe->cur_event;
	if(cev->memory)
	{
		tw_memory * b = cev->memory;

		while(b)
		{
			b->ts = tw_now(lp);
			b = b->next;
		}
	}
}

	/*
	 * Collect the statistics from each LP
	 */
void
rn_final(rn_lp_state * state, tw_lp * lp)
{
	tw_lp		*l;

	rn_stream	*s;

	unsigned int i;
	unsigned int j;

#if 0
	if(0 == lp->gid % 100000)
		printf("Finalizing RN lp %d \n", (int) lp->gid);
#endif

	for (i = 0; i < state->nstreams; i++)
	{
		s = &state->streams[i];

		for(j = 0; j < s->nlayers; j++)
		{
			l = &s->layers[j].lp;

			(*l->type.final) (l->cur_state, lp);
		}
	}

	g_rn_stats->s_nevents_processed += state->l_stats.s_nevents_processed;
	g_rn_stats->s_nevents_rollback += state->l_stats.s_nevents_rollback;
}

tw_lptype       rnlps[] = {
	{
	 (init_f) rn_startup,
	 (event_f) rn_event_handler,
	 (revent_f) rn_rc_event_handler,
	 (final_f) rn_final,
	 (map_f) rn_map,
	 sizeof(rn_lp_state),
	},
	{0}
	,
};

tw_lptype	env_lps[] = {
#ifdef HAVE_TLM_H
	{
	 (init_f) _tlm_init,
	 (event_f) _tlm_event_handler,
	 (revent_f) _tlm_rc_event_handler,
	 (final_f) _tlm_final,
	 (map_f) rn_map,
	 sizeof(tlm_state)
	},
#endif
	{0}
	,
};

void
rn_print_statistics()
{
	rn_statistics	 stats;
	tw_stat		 events;

	if(MPI_Reduce(&(g_rn_stats->s_nevents_processed),
			&stats,
			2,
			MPI_LONG_LONG,
			MPI_SUM,
			g_tw_masternode,
			MPI_COMM_WORLD) != MPI_SUCCESS)
		tw_error(TW_LOC, "TCP Final: unable to reduce statistics");

	if(!tw_ismaster())
		return;

	printf("\nROSS.Net Library Statistics: \n\n");
	//printf("\t%-50s %11d\n", "Event Size", (int) g_tw_msg_sz);
	printf("\t%-50s %11lld\n", "Total Packets Processed",
		   	stats.s_nevents_processed);
	printf("\t%-50s %11lld\n", "Total Packets Rolled Back",
			stats.s_nevents_rollback);
	printf("\n");

	events = stats.s_nevents_processed - stats.s_nevents_rollback;
	printf("\t%-50s %11lld *\n", "Net Packets Processed", events);
	printf("\t%-50s %11.2f (pkts/sec)\n", "Packet Rate", 
		(double) (events / g_tw_pe[0]->stats.s_max_run_time));
}

const tw_optdef rn_opts [] =
{
	TWOPT_GROUP("ROSS.Net Model"),
	TWOPT_UINT("link-prob", g_rn_link_prob, "link failure rate"),
	TWOPT_UINT("bgp", g_rn_converge_bgp, "pre-converge BGP model"),
	TWOPT_UINT("ospf", g_rn_converge_ospf, "pre-converge OSPFv2 model"),
	TWOPT_CHAR("logs", g_rn_logs_dir, "user specified log directory"),
	TWOPT_CHAR("topology", g_rn_xml_topology, "user specified network topology"),
	TWOPT_CHAR("route-table", g_rn_rt_table, "user specified routing table"),
	TWOPT_CHAR("model", g_rn_xml_model, "user specified traffic model (unimplemented"),
	TWOPT_CHAR("link-topology", g_rn_xml_link_topology, "user specified link topology"),
	TWOPT_CHAR("scenario", g_rn_tools_dir, "tools scenario"),
	TWOPT_CHAR("run", g_rn_run, "user supplied run identifier"),
	TWOPT_END()
};

	/*
	 * Map LPs->KPs->PEs, 
	 * initialize the ROSS engine, 
	 * run the model, 
	 * and print the model statistics.
	 */
int
main(int argc, char **argv, char **env)
{
	tw_lpid		 ttl_lps_per_pe;
	tw_lpid		 nnetlps_per_pe;
	tw_lpid		 nenvlps_per_pe;

	rn_lptype	*t;

	int		 i;

	g_rn_stats = tw_calloc(TW_LOC, "RN Statistics", sizeof(*g_rn_stats), 1);
	g_rn_lptypes = layer_types;

	/* add command line args */
	tw_opt_add(rn_opts);

	for(t = layer_types, i = 0; t && t->init; t = &layer_types[++i])
		if(*t->md_opts)
			(*t->md_opts) ();

	/* Initialize ROSS */
	tw_init(&argc, &argv);

	rn_setup();

	/* init the XML package */
	rn_xml_init();

	/* init the environment, if one exists */
	rn_init_environment();

	/* RN configurables */
	g_rn_msg_sz = sizeof(rn_message);

#ifdef HAVE_EPI_H
	g_tw_memory_nqueues += 1;
#endif

#ifdef HAVE_TCP_H
	g_tw_memory_nqueues += 1;
#endif

#ifdef HAVE_OSPF_H
	g_tw_memory_nqueues += 5;
#endif

#ifdef HAVE_IP_H
	g_tw_memory_nqueues += 1;
#endif

#ifdef HAVE_BGP_H
	g_tw_memory_nqueues += 3;
#endif

#ifdef HAVE_RANDOM_WALK_H
	g_tw_memory_nqueues += 1;
#endif

#ifdef HAVE_TLM_H
	//g_tw_memory_nqueues += 1;

	if(sizeof(tlm_message) > g_rn_msg_sz)
		g_rn_msg_sz = sizeof(tlm_message);
#endif

	/* ROSS configurables */
	g_tw_events_per_pe = g_tw_nlp * 40;
	g_tw_ts_end = 100.0;
	g_tw_rng_default = TW_FALSE;

	ttl_lps_per_pe = ceil( (double) (g_rn_nmachines + g_rn_env_nlps) / (double) (tw_nnodes() * g_tw_npe));
	tw_define_lps(ttl_lps_per_pe, g_rn_msg_sz, NULL);

	/* initialize the models */
	rn_models_init(layer_types, argc, argv, env);

	nnetlps_per_pe = g_rn_nmachines / (tw_nnodes() * g_tw_npe);
	for(i = 0; i < nnetlps_per_pe; i++)
		tw_lp_settype(i, &rnlps[0]);

	nenvlps_per_pe = g_rn_env_nlps / (tw_nnodes() * g_tw_npe);
	nenvlps_per_pe += i;
	for(; i < nenvlps_per_pe; i++)
		tw_lp_settype(i, &env_lps[0]);

	// extra LPs to pad out nlp per pe
	for(; i < g_tw_nlp; i++)
		tw_lp_settype(i, &rnlps[0]);

	tw_run();

	rn_xml_end();
	rn_models_final(layer_types);
	rn_print_statistics();

	tw_end();

	return 0;
}
