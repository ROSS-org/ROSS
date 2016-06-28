#include <ross.h>

/**
 * @file tw-lp.c
 * @brief tw_lp_settype is defined here!
 */

// IMPORTANT: this function replaces tw_lp_settype
// g_tw_lp_types must be defined
// g_tw_lp_typemap must be defined
void tw_lp_setup_types () {
	if ( !g_tw_lp_types ) {
		tw_error(TW_LOC, "No LP types are defined");
	}

	if ( !g_tw_lp_typemap ) {
		tw_error(TW_LOC, "No LP type mapping is defined");
	}

	int i;
	for (i = 0; i < g_tw_nlp; i++) {
		tw_lp *lp = g_tw_lp[i];
		lp->type = &g_tw_lp_types[g_tw_lp_typemap(lp->gid)];
	}
}

/**
 * IMPORTANT: This function should be called after tw_define_lps.  It
 * copies the function pointers which define the LP to the appropriate
 * location for *each* LP, i.e. you probably want to call this more than
 * once.
 */
void
tw_lp_settype(tw_lpid id, tw_lptype * type)
{
	tw_lp *lp = g_tw_lp[id];

	if(id >= g_tw_nlp)
		tw_error(TW_LOC, "ID %ld exceeded MAX LPs (%ld)", id, g_tw_nlp);

	if(!lp || !lp->pe)
		tw_error(TW_LOC, "LP %u has no PE assigned.", lp->gid);

	// memcpy(&lp->type, type, sizeof(*type));
	lp->type = type;

        if (type->state_sz > g_tw_delta_sz) {
            g_tw_delta_sz = type->state_sz;
        }
}

void
tw_lp_onpe(tw_lpid id, tw_pe * pe, tw_lpid gid)
{
	if(id >= g_tw_nlp)
		tw_error(TW_LOC, "ID %d exceeded MAX LPs", id);

	if(g_tw_lp[id])
		tw_error(TW_LOC, "LP already allocated: %lld\n", id);

	g_tw_lp[id] = (tw_lp *) tw_calloc(TW_LOC, "Local LP", sizeof(tw_lp), 1);

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
	kp->lp_count++;
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

		// Allocate initial state vector for this LP
		if(!lp->cur_state) {
			lp->cur_state = tw_calloc(TW_LOC, "state vector", lp->type->state_sz, 1);
		}

#ifndef USE_RIO
		if (lp->type->init)
		{
			me->cur_event = me->abort_event;
			me->cur_event->caused_by_me = NULL;

			(*(init_f)lp->type->init) (lp->cur_state, lp);

			if (me->cev_abort)
				tw_error(TW_LOC, "ran out of events during init");
		}
#endif
	}
#ifdef USE_RIO
	// RIO requires that all LPs have been allocated
	if (g_io_load_at == PRE_INIT || g_io_load_at == INIT) {
		tw_clock start = tw_clock_read();
        io_read_checkpoint();
        me->stats.s_rio_load += (tw_clock_read() - start);
    }
    if (g_io_load_at != INIT) {
    	tw_clock start = tw_clock_read();
    	for (i = 0; i < g_tw_nlp; i++) {
			tw_lp * lp = g_tw_lp[i];
			me->cur_event = me->abort_event;
			me->cur_event->caused_by_me = NULL;

			(*(init_f)lp->type->init) (lp->cur_state, lp);

			if (me->cev_abort) {
				tw_error(TW_LOC, "ran out of events during init");
			}
		}
		me->stats.s_rio_lp_init += (tw_clock_read() - start);
	}
    if (g_io_load_at == POST_INIT) {
		tw_clock start = tw_clock_read();
        io_read_checkpoint();
        me->stats.s_rio_load += (tw_clock_read() - start);
    }
#endif
}

void tw_pre_run_lps (tw_pe * me) {
	tw_lpid i;

	for(i = 0; i < g_tw_nlp; i++) {
		tw_lp * lp = g_tw_lp[i];

		if (lp->pe != me)
			continue;

		if (lp->type->pre_run) {
			me->cur_event = me->abort_event;
			me->cur_event->caused_by_me = NULL;

			(*(pre_run_f)lp->type->pre_run) (lp->cur_state, lp);

			if (me->cev_abort)
				tw_error(TW_LOC, "ran out of events during pre_run");
		}
	}
}
