#include <ospf.h>

/*
 * Optimized to send the packets immediately now, so we should never get this timer
 * anymore.
 */
void
ospf_flood_recv(ospf_state * state, ospf_nbr * nbr, tw_bf * bf, tw_lp * lp)
{
	nbr->flood = NULL;

#if 0
	tw_event       *event;
	ospf_message   *msg;
	tw_stime        ts;

	ospf_db_entry  *dbe;
	ospf_ls_update *update;

	int             i;
	int		accum;

	if (nbr->state <= ospf_nbr_exstart_st)
		return;

	if (nbr->nretrans < 1)
	{
		nbr->flood_timer = ospf_timer_cancel(nbr->flood_timer, lp);
		return;
	}

	ts = tw_rand_exponential(lp->id, OSPF_LS_INTERVAL);
	event = tw_event_new(tw_getlp(nbr->id), ts, lp);
	msg = tw_event_data(event);
	msg->type = OSPF_LS_UPDATE;
	msg->src = lp->id;
	msg->area = state->area;

	update = ospf_util_data(msg);
	update->nlsas = 0;

	for(accum = 0, i = 0; i < state->ndb; i++)
	{
		if(!nbr->retrans[i])
			continue;

		dbe = &state->db[i];

		if (m->src != nbr->id)
		{
			accum += dbe->lsa->length;

			if(accum > MTU)
				break;

			/*
			 * I can use this nbr's request list to send updates
			 * since we are adjacent and I should not be getting
			 * requests from them.
			 */
			nbr->retrans_ls[i].age = dbe->age;
			nbr->retrans_ls[i].lsa = dbe->lsa;

			update->nlsas++;
		}
	}

	update->lsa = nbr->retrans_ls;
	
	tw_event_send(event);
	state->stats->s_sent_ls_updates++;

	ospf_timer_start(nbr,
			 nbr->flood_timer,
			 OSPF_FLOOD_TIMER,
			 OSPF_FLOOD_TIMEOUT,
			 lp);
#endif
}
