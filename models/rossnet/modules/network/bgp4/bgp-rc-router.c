#include <bgp.h>

void
r_msg_open(bgp_state * state, tw_bf * bf, bgp_message * msg, tw_lp * lp)
{
	int    i;

	if (bf->c1)
	{
		for (i = 0; i < state->routes.size; i++)
		{
			tw_rand_reverse_unif(lp->id);
		}
	}

	tw_rand_reverse_unif(lp->id);
	state->stats->s_nopens--;
}

void
r_msg_keepalive(bgp_state * state, tw_bf * bf, bgp_message * msg, tw_lp * lp)
{
	state->stats->s_nkeepalives--;
}

void
r_msg_update(bgp_state * state, tw_bf * bf, bgp_message * msg, tw_lp * lp)
{
	int             i;

	if (bf->c6)
	{
		for (i = 0; i < msg->rev_num_neighbors; i++)
		{
			tw_rand_reverse_unif(lp->id);
		}
	}

	if (bf->c1)
	{							// reverse add
		tw_memory      *as_path;

		if (bf->c2)
		{
			if (bf->c3)
			{
				tw_memoryq_delete_any(&state->routes, msg->this_route);
				// state->routes.remove(msg->this_bgp_route);
				// state->routes.pop_back(); // makes it non-deterministic
				if (bf->c4)
				{
					tw_memory      *jim = tw_memory_free_rc(lp, g_bgp_fd_rtes);
					bgp_route      *rjim = tw_memory_data(jim);;
					tw_memoryq_push(&state->routes, jim);
					// state->bgp_routes.push_back(msg->erased_route);
					bzero(&rjim->as_path, sizeof(rjim->as_path));

					while (msg->rc_asp)
					{
						tw_memoryq_push(&rjim->as_path,
										tw_memory_free_rc(lp, g_bgp_fd_asp));
						msg->rc_asp--;
					}
				}
			}
		}
		while ((as_path =
				tw_memoryq_pop(&((bgp_route *) msg->this_route->data)->
							   as_path)))
			tw_event_memory_get_rc(lp, as_path, g_bgp_fd_asp);

		state->stats->s_nupdateadds--;
	} else
	{							// reverse remove
		if (bf->c5)
		{
			int             i = msg->rc_asp;
			tw_memory      *it4;
			bgp_route      *r;

			msg->this_route = tw_memory_free_rc(lp, g_bgp_fd_rtes);

			while (i)
			{
				tw_event_memory_get_rc(lp, tw_memory_free_rc(lp, g_bgp_fd_asp),
									   g_bgp_fd_asp);
				i--;
			}

			it4 = tw_memory_free_rc(lp, g_bgp_fd_rtes);
			r = tw_memory_data(it4);

			bzero(&r->as_path, sizeof(r->as_path));
			while (msg->rc_asp)
			{
				tw_memoryq_push(&r->as_path,
								tw_memory_free_rc(lp, g_bgp_fd_asp));
				msg->rc_asp--;
			}

			tw_memoryq_push(&state->routes, it4);

			// state->bgp_routes.push_back(msg->erased_bgp_route);
		}

		state->stats->s_nupdateremoves--;
	}

	tw_event_memory_get_rc(lp, msg->this_route, g_bgp_fd_rtes);

	msg->this_route = NULL;
}

void
r_msg_notification(bgp_state * state, tw_bf * bf, bgp_message * msg, tw_lp * lp)
{
	bgp_nbr		*n;

	if (bf->c1)
		if((n = bgp_getnbr(state, msg->src)))
			n->last_update = msg->old_last_update;

	state->stats->s_nnotifications--;
}

void
r_msg_keepalivetimer(bgp_state * state, tw_bf * bf, bgp_message * msg, tw_lp * lp)
{
	int		 i;

	for (i = 0; i <= state->n_interfaces; i++)
		tw_rand_reverse_unif(lp->id);

	state->stats->s_nkeepalivetimers--;
}

void
r_deal_with_timed_out_bgp_routers(bgp_state * state, tw_bf * bf,
						  bgp_message * msg, tw_lp * lp)
{
	tw_stime        now;
	tw_memory      *b;

	bgp_route      *r;
	bgp_nbr        *n;

	int             i;

	while (msg->rc_asp)
	{
		b = tw_memory_free_rc(lp, g_bgp_fd_rtes);
		r = tw_memory_data(b);
		i = r->as_path.size;

		bzero(&r->as_path, sizeof(r->as_path));
		while(i)
		{
			tw_memoryq_push(&r->as_path,
						tw_memory_free_rc(lp, g_bgp_fd_asp));
			i--;
		}

		msg->rc_asp--;
		tw_memoryq_push(&state->routes, b);
	}

	now = tw_now(lp);
	for(i = 0; i < state->n_interfaces; i++)
	{
		n = &state->nbr[i];

		if ((now - n->last_update) > state->hold_interval &&
			(now - n->last_update) < (2 * state->hold_interval))
			n->up = TW_TRUE;
	}
}

void
bgp_r_event_handler(bgp_state * state, tw_bf * bf, rn_message * rn_msg, tw_lp * lp)
{

	bgp_message    *msg;
	bgp_nbr		*n;

	msg = rn_message_data(rn_msg);

	switch (msg->type)
	{
	case OPEN:
		{
			// open bgp_message
			r_msg_open(state, bf, msg, lp);
			break;
		}
	case KEEPALIVE:
		{
			// keepalive bgp_message
			r_msg_keepalive(state, bf, msg, lp);
			break;
		}
	case UPDATE:
		{
			// update bgp_message
			r_msg_update(state, bf, msg, lp);
			break;
		}
	case NOTIFICATION:
		{
			// notification (error) bgp_message
			// r_msg_notification(state, bf, msg, lp);
			break;
		}
	case KEEPALIVETIMER:
		{
			// keepalive timer event
			// r_deal_with_timed_out_routers(state, bf, msg, lp);
			r_msg_keepalivetimer(state, bf, msg, lp);
			break;
		}
	default:
		printf("Unhandled message type: %d \n", msg->type);
	}

	n = bgp_getnbr(state, msg->src);
	n->last_update = msg->old_last_update;
	n->up = msg->was_up;
}
