#include <ospf.h>

/** \file ospf-router.c
 * This file contains the OSPFv2 Model LP event handler and init functions.
 *
 * These functions are required by ROSS.Net in order to pass control to the
 * OSPF model when OSPF events are received, and for LP initialization.
 */

/*
 * I am leaving this function here so that it bothers me to the point that I 
 * take the time to optimize it out.  It is done on every event processed, and
 * is very inefficient.
 */
ospf_nbr	*
ospf_int_getinterface(ospf_state * state, int id)
{
	int	i;

	for (i=0; i < state->n_interfaces; i++)
	{
		if (state->nbr[i].id == id)
			return &state->nbr[i];
	}

	return NULL;
}

/**
 * OSPF model event handler function.
 *
 * This is the OSPF model event handler function.  This function is a
 * simple switch statement which determine what kind of event was recieved
 * and passes control to the appropriate function.
 */
void
ospf_event_handler(ospf_state * state, tw_bf * bf, rn_message * rn_msg, tw_lp * lp)
{
	tw_memory	*b;
	ospf_nbr	*nbr;
	ospf_message	*msg;

	// get the OSPF message header
	b = tw_event_memory_get(lp);

	if(!b)
		tw_error(TW_LOC, "No membuf on event!");

	msg = tw_memory_data(b);

	nbr = ospf_int_getinterface(state, rn_msg->src);

	if(!nbr)
		nbr = msg->data;

	if(rn_msg->type != TIMER && rn_getas(state->m) != rn_getas(nbr->m))
		printf("Got event from nbr outside my AS! \n");


/*
	//printf("%d: got message  %d at %g\n", lp->gid, rn_msg->src, tw_now(lp));

	if(g_route[gr1] == lp->gid)
	{
		gr1++;
		g_route[gr1] = state->from;
	} 

	if(g_route1[gr2] == lp->gid)
	{
		gr2++;
		g_route1[gr2] = state->from1;
	}
*/

	if(rn_msg->port != 23)
		tw_error(TW_LOC, "%ld: recv non-ospf message from %d!", lp->gid, nbr->id);

	switch (msg->type)
	{
	case OSPF_HELLO_MSG:
#if VERIFY_HELLO || 1
	if(1 == lp->gid)
		printf("%lld OSPF: recv HELLO_MSG (%ld) from %lld, ts %f\n", 
			lp->gid, (long int) lp->pe->cur_event, rn_msg->src, tw_now(lp));
#endif

		ospf_hello_packet(state, nbr, msg->data, bf, lp);
		state->stats->s_e_hello_in++;
		break;

	case OSPF_HELLO_SEND:
#if VERIFY_HELLO || 1
	if(1 == lp->gid)
		printf("%lld OSPF: recv HELLO_SEND timer from %lld, ts %lf \n", 
				lp->gid, rn_msg->src, tw_now(lp));
#endif

		ospf_hello_send(state, nbr, bf, lp);
		break;

	case OSPF_DD_MSG:
#if VERIFY_DD || 1
	if(1 == lp->gid)
		printf("\n%lld OSPF: got DD packet from %lld\n", lp->gid, rn_msg->src);
#endif

		ospf_dd_event_handler(state, nbr, bf, lp);
		state->stats->s_e_dd_msgs++;
		break;

	case OSPF_LS_REQUEST:
#if VERIFY_LS || 1
	if(1 == lp->gid)
		printf("\n%lld: got LS_Request from %lld at %f\n", 
			lp->gid, rn_msg->src, tw_now(lp));
#endif

		ospf_ls_request_recv(state, bf, nbr, lp);
		state->stats->s_e_ls_requests++;
		break;

	case OSPF_LS_UPDATE:
#if VERIFY_LS || 1
	if(1 == lp->gid)
		printf("\n%lld: got LS Update from %lld at %f\n",
			lp->gid, rn_msg->src, tw_now(lp));
#endif
		ospf_ls_update_recv(state, bf, nbr, lp);
		state->stats->s_e_ls_updates++;
		break;

	case OSPF_FLOOD_TIMEOUT:
		tw_error(TW_LOC, "Should not be here!!");

		//ospf_flood_recv(state, nbr, bf, lp);
		break;

	case OSPF_LS_ACK:
#if VERIFY_LS || 1
	if(1 == lp->gid)
		printf("\n%lld: got LS_ACK from %lld %f\n",
				lp->gid, rn_msg->src, tw_now(lp));
#endif

		ospf_ack_process(nbr, bf, lp);
		state->stats->s_e_ls_acks++;

		break;

	case OSPF_AGING_TIMER:
#if VERIFY_AGING || 1
	if(1 == lp->gid)
		printf("%lld: got AGING_TIMER at %f\n", lp->gid, tw_now(lp));
#endif
		ospf_aging_timer(state, bf, lp);
		state->stats->s_e_aging_timeouts++;
		break;

	case OSPF_RT_TIMER:
#if 1
		printf("\n%lld: rt timer fired! \n", lp->gid);
#endif
		tw_event_memory_get(lp);
		ospf_rt_timer(state, lp->gid);

		break;
	case OSPF_RETRANS_TIMEOUT:
#if VERIFY_DD || 1
	if(1 == lp->gid)
		printf("%lld: got RETRANS_TIMEOUT\n", lp->gid);
#endif

		tw_error(TW_LOC, "should not be here!");

		ospf_dd_retransmit(state, nbr, bf, lp);
		break;

	case OSPF_ACK_TIMEOUT:
#if VERIFY_LS || 1
	if(1 == lp->gid)
		printf("%lld: got Ack Timer \n", lp->gid);
#endif
		ospf_ack_timed_out(nbr, bf, lp);
		state->stats->s_e_ack_timeouts++;
		break;

	case OSPF_HELLO_TIMEOUT:
#if VERIFY_HELLO || 1
	if(1 == lp->gid)
		printf("%lld: recv HELLO_TIMEOUT for nbr %d, ts %lf \n", 
						lp->gid, nbr->id, tw_now(lp));
#endif

		state->stats->s_e_hello_timeouts++;

		ospf_nbr_event_handler(state, nbr,
					ospf_nbr_inactivity_timer_ev, lp);
		break;
	case OSPF_WEIGHT_CHANGE:
#if VERIFY_OSPF_EXPERIMENT
		printf("%lld: recv WEIGHT CHANGE for link %ld, ts %lf \n", 
			lp->gid, (long int) msg->data, tw_now(lp));
#endif
		ospf_experiment_weights(state, (long int) msg->data, lp);
		break;
	default:
		tw_error(TW_LOC, "%lld: Invalid packet type: %d at %f",
				 lp->gid, msg->type, tw_now(lp));
	}

	// Free the OSPF message header membuf
	tw_memory_free(lp, b, g_ospf_fd);

	if(lp->pe->cur_event->memory)
		tw_error(TW_LOC, "Left memory buffer on event!");
}

/**
 * OSPF model LP init function.
 *
 * This is the OSPF model LP initialization function.  This function is
 * responsible for initializing each OSPF LP.  The main steps are:
 * <ol>
 * <li>Create and init the neighbor array
 * <li>Create and init the LP LSA database
 * <li>Create and init the LP forwarding table
 * <li>Start the aging timer for the oldest LSA
 * </ol>
 *
 * The order of steps 2 and 3 are dependant upon if the model is started in
 * converged state or not.  The determination is based on the fact that 
 * sometimes we build the forwarding table from the LSA database, and sometimes
 * we read it in from file.
 */
void
ospf_startup(ospf_state * state, tw_lp * lp)
{
	unsigned int	rv;
	unsigned int	next_timer;

#if OSPF_LOG
	char		 name[255];

	sprintf(name, "%s/ospf-router-%ld.log", g_rn_logs_dir, lp->gid);
	//state->log = fopen(name, "w");
	state->log = stdout;
#endif

	state->from = -1;
	state->from1 = -1;

	state->gstate = g_ospf_state;
	state->m = rn_getmachine(lp->gid);
	state->ar = rn_getarea(state->m);
	state->stats = tw_calloc(TW_LOC, "", sizeof(ospf_statistics), 1);

	if(lp->gid == 0)
	{
		for(rv = 0; rv < 400; rv++)
			g_route[rv] = g_route1[rv] = -1;

		gr1 = gr2 = 0;

		state->sn = .003;
	}

	//printf("Init OSPF router: %ld \n", lp->gid);

	// Create the neighbor data structures
	ospf_neighbor_init(state, lp);

	// Create my router LSA links
	state->lsa_seqnum = OSPF_MIN_LSA_SEQNUM + 1;

	if(!g_rn_converge_ospf)
	{
		ospf_db_read(state, lp);
		ospf_routing_init(state, lp);
	} else if(!g_rn_converge_ospf)
	{
		ospf_routing_init(state, lp);
		ospf_db_read(state, lp);
	} else if(0 == g_ospf_rt_write)
	{
		// Setup the routing table for the first time
		ospf_routing_init(state, lp);

		// Setup the LSA database for the first time
		next_timer = ospf_db_init(state, lp);
	} else
	{
		// Setup the LSA database for the first time
		next_timer = ospf_db_init(state, lp);

		// Setup the routing table for the first time
		ospf_routing_init(state, lp);
	}

	state->lsa = tw_memory_data(state->ar->g_ospf_lsa[lp->gid - state->ar->low]);

	if(!g_rn_converge_ospf && !g_rn_converge_bgp)
		ospf_random_weights(state, lp);

	/*
	 * This timer should only go off when the oldest LSA in my DB
	 * gets to be the REFRESH age, so that I can make sure to request an
	 * update for it
	 */
	state->aging_timer = NULL;

#if 0
	state->aging_timer =
		ospf_timer_start(NULL, state->aging_timer,
				 next_timer, OSPF_AGING_TIMER, lp);
#endif

#if VERIFY_AGING
	printf("%ld: setting aging timer finally: %lf \n", 
		lp->gid, next_timer);
#endif
}
