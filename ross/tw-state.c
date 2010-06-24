#include <ross.h>

void
tw_state_save(tw_lp *lp, tw_event *cevent)
{
#if 0
  tw_lp_state	*n;
  tw_lp_state	*o;

  o = (tw_lp_state *) lp->cur_state;
  n = lp->state_qh;
  if (!n)
    {
      tw_state_alloc(lp, g_tw_sv_growcnt);
      n = lp->state_qh;
    }
  lp->state_qh = n->next;

  cevent->lp_state = o;
  lp->cur_state = n;

  if (lp->type.statecp)
    (*lp->type.statecp)(n, o);
  else
    memcpy(n, o, lp->type.state_sz);
#endif
}

void
tw_state_rollback(tw_lp *lp, tw_event *revent)
{
#if 0
  tw_lp_state	*o;

  if (lp->type.revent)
    {
#endif
	lp->pe->cur_event = revent;
	lp->kp->last_time = revent->recv_ts;

	(*lp->type.revent)(
		lp->cur_state,
		&revent->cv,
		tw_event_data(revent),
		lp);
#if 0
    } else
      {
	o = (tw_lp_state *) lp->cur_state;
	o->next = lp->state_qh;
	lp->state_qh = o;

	lp->cur_state = revent->lp_state;
      }
#endif
}

void
tw_state_alloc(tw_lp *lp, int nvect)
{
#if 0
  tw_lp_state	*h;
  tw_lp_state	*c;

  h = tw_calloc(TW_LOC, "state vectors", lp->type.state_sz, nvect);
  c = h;
  while(nvect--)
    {
      if (nvect == 0)
	{
	  c->next = NULL;
	} else
	  {
	    c->next = (tw_lp_state*)( ((char*)c) + lp->type.state_sz );
	    c = c->next;
	  }
    }

  lp->state_qh = h;
#endif
	if(!lp->cur_state)
		lp->cur_state = tw_calloc(TW_LOC, "state vectors", lp->type.state_sz, 1);
}
