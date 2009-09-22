#include <rossnet.h>

/*
 * This creates a new event to send directly to another layer on this rn_lp,
 * as soon as it is sent.. could also be used for out of band messages
 */
tw_event	*
rn_event_direct(tw_lpid dst, tw_stime ts, tw_lp * src)
{
	tw_event	*e;

	rn_lp_state	*state;
	rn_stream	*s;
	rn_message	*m;

	state = src->cur_state;

	e = tw_event_new(dst, ts, src);
	m = tw_event_data(e);

	m->type = DIRECT;
	m->timer = state->cur_lp;

	if(NULL != (s = rn_getstream(src)))
	{
		m->size = s->cur_layer;
		m->port = s->port;
	}

	m->src = src->gid;
	m->dst = dst;

	return e;
}

/**
 * inter-lp: events which will be sent to a remote lp
 *
 * 	inter-lp events are handled as usual
 *
 * intra-lp: events which are sent to another layer on the same lp
 *
 *	intra-lp events are immediately scheduled and processed, because their
 * 	destination is the local machine, and have a zero timestamp offset, which
 *	means the following:
 *
 *	the next layer is responsible for handling packet data, ie, from TCP
 *	to IP.  The IP layer must re-create the TCP packect on the dest
 * 	LP.
 *
 * abort events: should not let them traverse layers.. first layer to get abort
 *	is then the only one to get rolled back
 *
 * NOTE: We should probably base the timestamp off of the network characteristics,
 * 	 rather than make the model do it.  When the model calls this function, and
 *	 is the last layer, we should know the proper link to use, and can inform
 *	 the model if this event will fail if sent, over the network.
 */
tw_event	*
rn_event_new(tw_lpid dst, tw_stime ts, tw_lp * src, 
		rn_message_type dir, unsigned long int size)
{
	tw_event 	*e;
	tw_stime	 abs_ts = DBL_MAX;

	rn_lp_state	*state;
	rn_message	*r;
	rn_stream	*s;

	rn_machine	*m;

	state = src->cur_state;
	s = rn_getstream(src);
	m = rn_getmachine(src->gid);

	// if model attempts to send upstream event, and is 
	// top-most layer, abort processing
	if(dir == UPSTREAM && !s->cur_layer)
		return src->pe->abort_event;

	// Check for layer switching directions on us..
	if(state->cev)
	{
		r = tw_event_data(state->cev);

		if(r->type == UPSTREAM && dir == DOWNSTREAM)
		{
			state->cev = NULL;
		} else if(r->type != dir && dir == UPSTREAM)
		{
			state->cev = src->pe->cur_event;
			abs_ts = state->cev->recv_ts + ts;
		}
	}

	// if currently UPSTREAM processing, then state->cev ==
	// pe->cev, and so no new event is created
	//
	// if DOWNSTREAM, then state->cev is NULL for top-most
	// layer sending down
	if(!state->cev)
	{
		e = tw_event_new(dst, ts, src);

		state->cev = e;
		r = tw_event_data(e);

		r->type = dir;
		r->src = src->gid;
		r->dst = dst;
		r->size = size;
		r->port = s->port;
		r->timer = NULL;
		r->ttl = g_rn_ttl;

#if VERIFY_RN
		printf("%lld RN: Created new event %ld from %lld to %lld on port %d for %lf\n", 
			src->gid, (long int) e, r->src, r->dst, r->port, e->recv_ts);
#endif
	} else
	{
		e = state->cev;
		e->recv_ts += ts;

		r = tw_event_data(e);
		r->size = size;

		if(e->recv_ts > g_tw_ts_end)
			src->pe->cev_abort = 1;

		if(r->dst != dst)
		{
#if VERIFY_RN
			printf("%lld: Redirecting event %ld: from %lld to %lld \n",
				src->gid, (long int) e, (tw_lpid) e->dest_lp, dst);
#endif

			e->dest_lp = (tw_lp *) dst;
		}
	}

	return e;
}

void
rn_event_send(tw_event * new_event)
{
	//tw_memory	*b;
	tw_lp		*lp = NULL;

	rn_lp_state	*state = NULL;
	rn_stream	*s = NULL;
	rn_message	*rn_msg;
	//rn_message	*m = NULL;

	rn_msg = tw_event_data(new_event);

	if(rn_msg->type == UPSTREAM)
	{
		state = (rn_lp_state *) new_event->dest_lp->cur_state;
		s = rn_getstream_byport(state, rn_msg->port);
	} else if(new_event->src_lp == NULL || new_event->src_lp->pe->cev_abort)
	{
		return;
	} else if(rn_msg->type == DOWNSTREAM)
	{
		state = (rn_lp_state *) new_event->src_lp->cur_state;
		s = rn_getstream(new_event->src_lp);
	} else if(rn_msg->type == DIRECT)
	{
		tw_event_send(new_event);
		return;
	} else
	{
		tw_error(TW_LOC, "Unknown event type sent!");
	}

	/*
	 * If direction is UP, and more layers, send up to next layer.
	 * If direction is down, and more layers, send down to next layer
	 *
	 * If direction is UP, and at top, nowhere else to go, 
	 *			so done, consider it processed.
	 *
	 * If direction is down, and no more layers, tw_event_send it.
	 */
	if(rn_msg->type == UPSTREAM && s->cur_layer > 0)
	{
		s->cur_layer--;
		lp = state->cur_lp = &s->layers[s->cur_layer].lp;
		new_event->dest_lp->rng = lp->rng;

		// if at top of stack want new events created
		if(s->cur_layer == 0)
			state->cev = NULL;

#if VERIFY_RN
		printf("%lld RN: layer %d sending msg UP, dst %d, %lf \n",
			lp->gid, 
			s->cur_layer + 1,
			(int) new_event->dest_lp->gid,
			new_event->recv_ts);
#endif

		(*lp->type.event)
			(lp->cur_state,
			&new_event->cv,
			(void *) rn_msg,
			new_event->dest_lp);

		s->cur_layer++;
		state->cur_lp = &s->layers[s->cur_layer].lp;
		new_event->dest_lp->rng = state->cur_lp->rng;
		state->l_stats.s_nevents_processed++;
	} else if(rn_msg->type == DOWNSTREAM && s->cur_layer < s->nlayers - 1)
	{
		s->cur_layer++;
		lp = state->cur_lp = &s->layers[s->cur_layer].lp;
		new_event->src_lp->rng = lp->rng;

#if VERIFY_RN
		printf("%lld RN: layer %d sending msg DOWN, dst %lld, ts %f \n",
			lp->gid, 
			(int) s->cur_layer - 1,
			(tw_lpid) new_event->dest_lp,
			new_event->recv_ts);
#endif

		(*lp->type.event)
			(lp->cur_state,
			&new_event->cv,
			(void *) rn_msg,
			new_event->src_lp);

		// Restore the cur_layer when this lvl of recursion is complete
		s->cur_layer--;
		state->cev = NULL;
		state->cur_lp = &s->layers[s->cur_layer].lp;
		new_event->src_lp->rng = state->cur_lp->rng;
		state->l_stats.s_nevents_processed++;
	} else if(rn_msg->type == DOWNSTREAM)
	{
		// Ok, so now we have reached the bottom of the stack and
		// can remove the event finally.
#if 0
// I think tw_event_memory_set handles this already.. and tw_sched handles
// the abort event properly.
		if(new_event->src_lp->pe->cev_abort)
		{
			while(new_event->memory)
			{
				b = new_event->memory;
				new_event->memory = b->next;

				tw_memory_free_single(new_event->src_lp->pe, b, b->fd);
			}

			return;
		}
#endif

#if VERIFY_RN
		printf("%d: RN sending remote event from layer %d: %lld -%lf-> %lld \n",
				g_tw_mynode,
				s->cur_layer,
				new_event->src_lp->gid,
				new_event->recv_ts,
				(tw_lpid) new_event->dest_lp);
#endif

#if 0
		// forward any membufs on the event
		m = tw_event_data(new_event->src_lp->pe->cur_event);
		if(new_event->src_lp->pe->cur_event->memory && m->type != TIMER)
		{
			if(new_event->memory)
				tw_error(TW_LOC, "membuf already on event?");

			b = new_event->memory = new_event->src_lp->pe->cur_event->memory;

			while(b)
			{
				b->ts = tw_now(new_event->src_lp);
				b = b->next;
			}
		}
#endif

		/*
		 * This is where we would want to do things like fixing up 
		 * the timestamp on the 
		 * message, and the dest_lp pointer in the event.  We need to 
		 * change the dest_lp appropriately in order to reflect 
		 * the final destination considering the connection and/or nbr 
		 */
		rn_msg->type = UPSTREAM;
		tw_event_send(new_event);
		state->cev = NULL;
	} else if(rn_msg->type == UPSTREAM)
	{
		tw_error(TW_LOC, "%lld: sending message up, top-most layer!? \n", 
				new_event->src_lp->gid);
	} else
	{
		tw_error(TW_LOC, "no send?");
	}
}
