#include <ospf.h>

#define LOG_ROUTER 0

tw_stime c_time = 0.0;

void
ospf_statistics_collection(ospf_state * state, tw_lp * lp)
{
#if LOG_ROUTER || OSPF_LOG
	//char		 name[255];
#endif
	tw_memory	*b;

	ospf_lsa	*lsa;

	int		 i;

	g_ospf_stats->s_e_hello_in += state->stats->s_e_hello_in;
	g_ospf_stats->s_e_hello_out += state->stats->s_e_hello_out;

	g_ospf_stats->dropped_packets += state->stats->dropped_packets;
	g_ospf_stats->s_drop_dd += state->stats->s_drop_dd;
	g_ospf_stats->s_sent_hellos += state->stats->s_sent_hellos;
	g_ospf_stats->s_sent_dds += state->stats->s_sent_dds;
	g_ospf_stats->s_sent_ls_requests += state->stats->s_sent_ls_requests;
	g_ospf_stats->s_sent_ls_updates += state->stats->s_sent_ls_updates;
	g_ospf_stats->s_sent_ls_acks += state->stats->s_sent_ls_acks;
	g_ospf_stats->s_sent_unknown += state->stats->s_sent_unknown;
	g_ospf_stats->s_sent_lost += state->stats->s_sent_lost;

	g_ospf_stats->s_cause_bgp += state->stats->s_cause_bgp;
	g_ospf_stats->s_cause_ospf += state->stats->s_cause_ospf;

	if(lp->gid < 1000 && 0)
		printf("Finalizing OSPF lp %lld \n", lp->gid);

	c_time = max(c_time, state->c_time);

#if VERIFY_OSPF_CONVERGENCE
	if(lp->gid == 0)
	{
		printf("OSPFv2 Model Statistics: \n\n");
		printf("\tRoute 0: ");
		for(i = 0; i < 40000; i++)
		{
			if(g_route[i] == -1)
				break;

			printf(" %d,", g_route[i]);
		}
		printf(" (i = %d) \n", i);

		printf("\tRoute 1: ");
		for(i = 0; i < 40000; i++)
		{
			if(g_route1[i] == -1)
				break;

			printf(" %d", g_route1[i]);
		}
		printf(" (i = %d) \n", i);
		
		printf("\n\tTotal Network Convergence Time: %f \n", c_time);
	}
#endif

#if OSPF_LOG && 0
	if(state->log && state->log != stdout)
		fclose(state->log);

	sprintf(name, "%s/ospf-router-%ld.log", g_rn_logs_dir, lp->gid);
	state->log = fopen(name, "w");

	if(!state->log)
		return;

	fprintf(state->log, "OSPFv2 Router %ld: Final Time: %f\n\n", lp->gid, tw_now(lp));

	ospf_db_print(state, state->log, lp);
	ospf_rt_print(state, lp, state->log);

#endif
#if LOG_ROUTER
	fprintf(state->log, "Events Received:\n");
	fprintf(state->log, "\t%-50s %11ld\n", "HELLO msgs recvd",
						state->stats->s_e_hello_in);
	fprintf(state->log, "\t%-50s %11ld\n", "HELLO send timers",
						state->stats->s_e_hello_out);
	fprintf(state->log, "\t%-50s %11ld\n", "HELLO inactive timeouts", 
						state->stats->s_e_hello_timeouts);
	fprintf(state->log, "\n");
	fprintf(state->log, "\t%-50s %11ld\n", "DD msgs recvd", state->stats->s_e_dd_msgs);
	fprintf(state->log, "\t%-50s %11ld\n", "DD pkts dropped", state->stats->s_drop_dd);
	fprintf(state->log, "\n");
	fprintf(state->log, "\t%-50s %11ld\n", "LS Requests recvd",
						state->stats->s_e_ls_requests);
	fprintf(state->log, "\t%-50s %11ld\n", "ACK timeouts",
						state->stats->s_e_ack_timeouts);
	fprintf(state->log, "\n");
	fprintf(state->log, "\t%-50s %11ld\n", "Aging timer events",
						state->stats->s_e_aging_timeouts);
	fprintf(state->log, "\t%-50s %11ld\n", "Unknown events!", state->stats->s_e_unknown);
	fprintf(state->log, "\n");
	fprintf(state->log, "Events Sent:\n");
	fprintf(state->log, "\t%-50s %11ld\n", "HELLO msgs", state->stats->s_sent_hellos);
	fprintf(state->log, "\t%-50s %11ld\n", "DD packets", state->stats->s_sent_dds);
	fprintf(state->log, "\t%-50s %11ld\n", "LS Requests", state->stats->s_sent_ls_requests);
	fprintf(state->log, "\t%-50s %11ld\n", "Dropped Packets", state->stats->s_sent_lost);
	fprintf(state->log, "\t%-50s %11ld\n", "Unknown!", state->stats->s_sent_unknown);



	fprintf(state->log, "\n");
	fprintf(state->log, "\t%-50s %11ld\n", "LS Updates sent ", 
		state->stats->s_sent_ls_updates);
	fprintf(state->log, "\t%-50s %11ld\n", "LS ACKS recvd", state->stats->s_e_ls_acks);

	fprintf(state->log, "\n");
	fprintf(state->log, "\t%-50s %11ld\n", "LS Updates recvd",
		state->stats->s_e_ls_updates);
	fprintf(state->log, "\t%-50s %11ld\n", "LS ACKs sent ", 
		state->stats->s_sent_ls_acks);
	fprintf(state->log, "\n");
	fprintf(state->log, "%s %ld\n", "BGP Caused Updates", 
		state->stats->s_cause_bgp);
	fprintf(state->log, "%s %ld\n", "OSPF Caused Updates", 
		state->stats->s_cause_ospf);

	if(state->log && state->log != stdout)
		fclose(state->log);
#endif

	// Create the ospf-lsa.file

	if(!g_rn_converge_ospf && !g_rn_converge_bgp)
		return;

	// Print out the LSAs for the area

	if(state->ar->low == lp->gid)
	{
		for(i = 0; i < state->ar->g_ospf_nlsa; i++)
		{
			b = state->ar->g_ospf_lsa[i];

			while(b && NULL != (lsa = tw_memory_data(b)))
			{
				ospf_lsa_print(g_ospf_lsa_file, lsa);

				b = lsa->next;
			}
		}

		fprintf(g_ospf_lsa_file, "-1\n");
	}

	ospf_db_print_raw(state, g_ospf_lsa_file, lp);
}
