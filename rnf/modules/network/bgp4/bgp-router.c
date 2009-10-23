#include <bgp.h>

void
bgp_event_handler(bgp_state * state, tw_bf * bf, rn_message * rn_msg, tw_lp * lp)
{
	tw_memory      *hdr;

	bgp_message    *msg;

	*(int *) bf = 0;

	// reset that neighbors timer
	hdr = tw_event_memory_get(lp);
	msg = tw_memory_data(hdr);

	if(g_rn_converge_ospf)
		tw_error(TW_LOC, "Should not be in BGP!");

	if(rn_msg->port != 169)
		tw_error(TW_LOC, "recv non-bgp message!");

	// 'cause it would be a timer..
	if (rn_msg->src != lp->id)
		bgp_keepalive(state, rn_msg->src, (int) rn_msg->ttl, lp);

	switch (msg->b.type)
	{
		case CONNECT:
#if VERIFY_BGP
			fprintf(state->log, "\n%ld BGP: CONNECT from %ld at %lf\n",
				lp->id, rn_msg->src, tw_now(lp));
#endif
			break;
		case OPEN:
#if VERIFY_BGP
			fprintf(state->log, "\n%ld BGP: OPEN from %ld at %lf\n",
				lp->id, rn_msg->src, tw_now(lp));
#endif
			bgp_open(state, bf, msg, rn_msg->src, lp);

			state->stats->s_cause_ospf += msg->b.cause_ospf;
			state->stats->s_cause_bgp += msg->b.cause_bgp;
			break;
		case UPDATE:
#if VERIFY_BGP
			fprintf(state->log, "\n%ld BGP: UPDATE from %ld at %lf\n",
				lp->id, rn_msg->src, tw_now(lp));
#endif
			bgp_update(state, bf, msg, rn_msg->src, lp);
			state->stats->s_nupdates_recv++;
			break;
		case NOTIFICATION:
#if VERIFY_BGP
			fprintf(state->log, "\n%ld BGP: NOTIFY from %ld at %lf\n",
				lp->id, rn_msg->src, tw_now(lp));
#endif
			bgp_notify(state, bf, rn_msg->src, lp);
			state->stats->s_nnotify_recv++;
			break;
		case KEEPALIVE:
#if VERIFY_BGP_KEEPALIVE
			fprintf(state->log, "\n%ld BGP: KEEPALIVE from %ld at %lf\n",
				lp->id, rn_msg->src, tw_now(lp));
#endif
			state->stats->s_nkeepalives++;
			break;
		case KEEPALIVETIMER:
#if VERIFY_BGP_KEEPALIVE
			fprintf(state->log, "\n%ld BGP: KEEPALIVE_TIMER at %lf\n",
				lp->id, tw_now(lp));
#endif
			bgp_keepalive_timer(state, bf, msg, lp);
			deal_with_timed_out_routers(state, bf, msg, lp);
			break;
		case MRAITIMER:
#if VERIFY_BGP
			fprintf(state->log, "\n%ld: BGP: MRAI_TIMER at %lf \n",
				lp->id, tw_now(lp));
#endif
			state->stats->s_nmrai_timers++;
			bgp_mrai_timer(state, bf, msg, lp);
			break;
		default:
			tw_error(TW_LOC, "Unhandled BGP event type!");
	}

	tw_memory_free(lp, hdr, g_bgp_fd);

	if(NULL != lp->pe->cur_event->memory)
		tw_error(TW_LOC, "memory buffers remain on event! \n");
}
