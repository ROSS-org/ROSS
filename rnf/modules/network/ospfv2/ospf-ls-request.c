#include <ospf.h>

#define LINK_TIME 0.005

#if 0 // OLD VERSION -- NEW/CORRECT IN ospf-ls.c
void
ospf_ls_request_recv(ospf_state * state, tw_bf * bf, ospf_nbr * nbr, tw_lp * lp)
{
	tw_stime	 ts;

	ospf_db_entry	*dbe;
	ospf_db_entry	*in_dbe;
	ospf_db_entry	*out_dbe;
	ospf_lsa	*lsa;

	tw_memory		*buf;
	tw_memory		*recv;
	tw_memory		*send;

	int		 accum;

	if(nbr->state < ospf_nbr_exchange_st)
		return;

	send = buf = NULL;
	accum = OSPF_HEADER;
	recv = tw_event_memory_get(lp);
	dbe = NULL;
	ts = 0;

	while(recv)
	{
		in_dbe = tw_memory_data(recv);
		lsa = getlsa(state->m, in_dbe->lsa, nbr->id);

		dbe = &state->db[lsa->adv_r];
		//dbe = &state->db[request->lsa[i].lsa->adv_r];

		accum +=  lsa->length;

		if(accum > state->gstate->mtu || recv->next == NULL)
		{
			ts += LINK_TIME;
			ospf_event_send(state,
					nbr->id,
					OSPF_LS_UPDATE,
					lp,
					ts,
					send,
					nbr->router->ar->id);

			accum = OSPF_HEADER;
			send = NULL;
		}

		if(NULL == send)
			send = buf = tw_memory_alloc(lp, g_ospf_fd);
		else
			buf->next = tw_memory_alloc(lp, g_ospf_fd);

		out_dbe = tw_memory_data(buf);
		out_dbe->lsa = dbe->lsa;
		out_dbe->b.age = ospf_lsa_age(state, dbe, lp);

		printf("Sending LSA ages: old %d, new %d \n", dbe->b.age,
					out_dbe->b.age);

		buf = buf->next;
		tw_memory_free(lp, recv, g_ospf_fd);
		recv = tw_event_memory_get(lp);
	}
}
#endif
