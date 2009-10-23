#include <rossnet.h>

void
rn_reverse_event_send(tw_lpid dst, tw_lp * src, rn_message_type dir, unsigned long int size)
{
	tw_event	*cev;
	tw_lp		*lp;

	rn_lp_state	*state;
	rn_stream	*s;
	rn_message	*m;
	rn_message	 msg;

	cev = src->pe->cur_event;
	state = src->cur_state;
	s = rn_getstream(src);
	m = tw_event_data(cev);

	msg.type = dir;
	msg.src = src->gid;
	msg.dst = dst;
	msg.size = size;
	msg.port = m->port;
	msg.ttl = m->ttl;

	if(dir == DIRECT)
	{
		// do nothing, no layer to reverse
		return;
	} else if(dir == UPSTREAM && s->cur_layer > 0)
	{
		// RC layer above us
		s->cur_layer--;
		lp = state->cur_lp = &s->layers[s->cur_layer].lp;

		(*lp->type.revent) (lp->cur_state, &cev->cv, &msg, src);

		// restore my state
		s->cur_layer++;
		state->cur_lp = &s->layers[s->cur_layer].lp;
		state->l_stats.s_nevents_rollback++;
	} else if(dir == DOWNSTREAM && s->cur_layer < s->nlayers - 1)
	{
		// RC layer below us
		s->cur_layer++;
		lp = state->cur_lp = &s->layers[s->cur_layer].lp;

		(*lp->type.revent) (lp->cur_state, &cev->cv, &msg, src);

		// restore my state
		s->cur_layer--;
		state->cur_lp = &s->layers[s->cur_layer].lp;
		state->l_stats.s_nevents_rollback++;
	} else if(dir == DOWNSTREAM)
	{
		// bottom of stack, no more layers to RC
		return;
	} else
	{
		tw_printf(TW_LOC, "%lld: dir %d, m->type %d, cur_layer %d\n", 
			 src->gid, dir, m->type, s->cur_layer);
		tw_error(TW_LOC, "Unhandled case in rn_reverse_event_send!");
	}
}
