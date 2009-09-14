#include <rossnet.h>

void	*
rn_getstate(tw_lp * lp)
{
#if 0
	rn_lp_state *me;

	me = (rn_lp_state *) lp->cur_state;

	return me->lp_list[me->cur_layer].cur_state;
#endif

	return NULL;
}

tw_lp	*
rn_getlp(rn_lp_state * state, int id)
{
	//return &state->layers[id];
	return NULL;
}

void
rn_lp_settype(tw_lp * lp, char * name)
{
	rn_lptype	*t;

	for(t = g_rn_lptypes; t; t++)
		if(0 == strcmp(t->sname, name))
			break;

	memcpy(&lp->type, t, sizeof(tw_lptype));
}

rn_lptype	*
rn_lp_gettype(tw_lp * lp)
{
	rn_lptype	*t;

	for(t = g_rn_lptypes; t && t->init; t++)
	{
		if(lp->type.init == t->init)
			return t;
	}

	return NULL;
}

tw_lp *
rn_next_lp(rn_lp_state * state, tw_lp * last, int type)
{
#if 0
	if(type == DOWNSTREAM)
	{
		if(last == NULL)
			return state->lp_list;

		if(last->id == state->nlp - 1)
			return NULL;

		return &state->lp_list[last->id + 1];
	}

	if(last == NULL)
		return &state->lp_list[state->nlp - 1];

	if(last->id == 0)
		return NULL;

	return &state->lp_list[last->id - 1];
#endif

	return NULL;
}
