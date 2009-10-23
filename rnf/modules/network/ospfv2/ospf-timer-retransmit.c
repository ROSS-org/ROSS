#include <ospf.h>

/*
 * This should be optimized out now .. 
 * If this timer went off, it is because our neighbor sent it for a message
 * it could not ACK.
 */
void
ospf_retrans_timed_out(ospf_state * state, tw_bf * bf, ospf_retrans_timeout *msg, 
			tw_lp * lp)
{
#if 0
	tw_event       *event;
	tw_stime        ts;

	ospf_message	*m;
	ospf_nbr	*nbr;
	ospf_retrans_timeout *t;

	nbr = msg->nbr;

	ts = tw_rand_exponential(lp->id, 32);
	event = tw_event_new(tw_getlp(nbr->id), ts, lp);
	m = tw_event_data(event);
	m->src = lp->id;
	m->area = state->area;

	if (msg->type == ospf_retrans_dd)
	{
		m->type = OSPF_DD_MSG;
		ospf_util_setdata(m, nbr->retrans_dd);
	} else
	{
		if (msg->type == ospf_retrans_ls)
		{
			m->type = OSPF_LS_REQUEST;
			ospf_util_setdata(m, nbr->retrans_ls);
		} else
		{
			printf("Invalid retrans-timeout !!! CHECK !!\n");
			return;
		}
	}

	tw_event_send(event);

	nbr->retrans_timer = ospf_timer_start(nbr,
						nbr->retrans_timer,
						OSPF_RETRANS_INTERVAL, 
						OSPF_RETRANS_TIMEOUT,
						lp);

	if (nbr->retrans_timer)
	{
		m = tw_event_data(nbr->retrans_timer);
		t = ospf_util_data(m);

		t->type = msg->type;
	}
#endif
}
