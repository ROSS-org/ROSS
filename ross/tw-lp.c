#include <ross.h>

/**
 * @file tw-lp.c
 * @brief tw_lp_settype is defined here!
 */

/**
 * IMPORTANT: This function should be called after tw_define_lps.  It
 * copies the function pointers which define the LP to the appropriate
 * location for *each* LP, i.e. you probably want to call this more than
 * once.
 */
void
tw_lp_settype(tw_lpid id, const tw_lptype * type)
{
	tw_lp *lp = g_tw_lp[id];

	if(id >= g_tw_nlp)
		tw_error(TW_LOC, "ID %ld exceeded MAX LPs (%ld)", id, g_tw_nlp);

	if(!lp || !lp->pe)
		tw_error(TW_LOC, "LP %u has no PE assigned.", lp->gid);

	memcpy(&lp->type, type, sizeof(*type));
}

void
tw_lp_onpe(tw_lpid id, tw_pe * pe, tw_lpid gid)
{
	if(id >= g_tw_nlp)
		tw_error(TW_LOC, "ID %d exceeded MAX LPs", id);

	if(g_tw_lp[id])
		tw_error(TW_LOC, "LP already allocated: %lld\n", id);

	g_tw_lp[id] = tw_calloc(TW_LOC, "Local LP", sizeof(tw_lp), 1);

	g_tw_lp[id]->gid = gid;
	g_tw_lp[id]->id = id;
	g_tw_lp[id]->pe = pe;
}

void
tw_lp_onkp(tw_lp * lp, tw_kp * kp)
{
	if(!lp)
		tw_error(TW_LOC, "Bad LP pointer!");

	lp->kp = kp;
}

void
tw_init_lps(tw_pe * me)
{
	tw_lpid i;

	for(i = 0; i < g_tw_nlp; i++)
	{
		tw_lp * lp = g_tw_lp[i];

		if (lp->pe != me)
			continue;

		/*
		 * Allocate initial state vectors for this LP, and
		 * steal one of them for the initial state to be
		 * handed to the init function.
		 */
		if (lp->type.revent)
			tw_state_alloc(lp, 1);
		else
			tw_state_alloc(lp, g_tw_sv_growcnt + 1);

		if (lp->type.init)
		{
			me->cur_event = me->abort_event;
			me->cur_event->caused_by_me = NULL;

			(*(init_f)lp->type.init) (lp->cur_state, lp);

			if (me->cev_abort)
				tw_error(TW_LOC, "ran out of events during init");
		}
	}
}
