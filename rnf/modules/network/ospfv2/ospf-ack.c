#include <ospf.h>

void
ospf_ack_delayed(ospf_nbr * nbr, tw_memory * buf, int size, tw_lp * lp)
{
	if (NULL != nbr->delay_ack_timer)
	{
		// This means that this is the first delayed ack to be added to the
		// queue..
		nbr->delay_ack_timer = ospf_timer_start(nbr, nbr->delay_ack_timer,
							1, OSPF_ACK_TIMEOUT, lp);
	}

	buf->next = nbr->delay;
	nbr->delay = buf;
	nbr->delay_ack_size += size;
}

/*
 * When this timer goes off we send our neighbor the delayed ACK
 *
 * Unhandled case: too many acks added (beyond MTU)
 */
void
ospf_ack_timed_out(ospf_nbr * nbr, tw_bf * bf, tw_lp * lp)
{
	tw_event	*e;

	e = ospf_event_new(nbr->router, tw_getlp(nbr->id), 0.0, lp);
	ospf_event_send(nbr->router, e, OSPF_LS_ACK, lp,
					nbr->delay_ack_size,
					nbr->delay, nbr->router->ar->id);

/*
	ospf_event_send(nbr->router, tw_getlp(nbr->id), OSPF_LS_ACK, lp, 
					nbr->delay_ack_size,
					nbr->delay, nbr->router->ar->id);
*/

	nbr->delay = NULL;
	nbr->delay_ack_size = OSPF_ACK_HEADER;
	nbr->delay_ack_timer = ospf_timer_cancel(nbr->delay_ack_timer, lp);
}

/*
 * Just send the recv'd buffer back to the source
 */
void
ospf_ack_direct(ospf_nbr * nbr, tw_memory * buf, int size, tw_lp * lp)
{
	tw_event       *e;
	
	ospf_db_entry  *dbe;

	dbe = tw_memory_data(buf);
	e = ospf_event_new(nbr->router, tw_getlp(nbr->id), 0.0, lp);
	ospf_event_send(nbr->router, e, OSPF_LS_ACK, lp, size + OSPF_ACK_HEADER, 
						buf, nbr->router->ar->id);
/*
	e = ospf_event_send(nbr->router, tw_getlp(nbr->id), OSPF_LS_ACK,
						lp, size + OSPF_ACK_HEADER, 
						buf, nbr->router->area);
*/

#if VERIFY_OSPF_ACK
	printf("%ld: sending direct ACK for LSA %d to %d at %f, now %f\n",
		   lp->id, dbe->b.entry, nbr->id, e->recv_ts, tw_now(lp));
#endif
}

/*
 * There is nothing to do with ACKs unless I get a negative ACK
 */
void
ospf_ack_process(ospf_nbr * nbr, tw_bf * bf, tw_lp * lp)
{
	tw_memory      *buf;
	ospf_db_entry  *dbe;
	int             cnt;

	cnt = 0;
	for (buf = tw_event_memory_get(lp); buf; buf = tw_event_memory_get(lp))
	{
		dbe = tw_memory_data(buf);
		tw_memory_free(lp, buf, g_ospf_fd);
	}


#if VERIFY_OSPF_ACK
	if (nbr->state < ospf_nbr_exchange_st)
	{
		printf("%ld: nbr state is %d: LS ACK is ignored\n", 
				lp->id, nbr->state);
		return;
	}
#endif
}
