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

/********************************************************************//**
            LP Suspension Design Notes! (John Jenkins, ANL)

Many times, when developing optimistic models, we are able to
determine < LP state, event > pairs which represent infeasible model
behavior. These types of simulation states typically arise when time
warp causes us to receive and potentially process messages in an order
we don't expect.

For example, consider a client/server protocol in which a server sends
an ACK to a client upon completion of some event. In optimistic mode,
the client can see what amounts to duplicate ACKs from the server due
to the server LP rolling back and re-sending an ACK.

While some models can gracefully cope with such issues, more complex
models can have troubles (the client in the example could for instance
destroy the request metadata after receiving an ACK).

A solution, as noted in the "Dark Side of Risk" paper, is to introduce
LP "self-suspend" functionality. If an LP is able to detect a < state,
message > pair which is incorrect / unexpected in a well-behaved
simulation, the LP should be able to put itself into suspend mode,
refusing to process messages until rolled back to a pre < state,
message > state. There are two benefits: 1) it greatly reduces the
difficulty in tracking down and distinguishing proper model bugs from
bugs arising from time-warp related issues such as out-of-order event
receipt and 2) it improves simulation performance by pruning the
number of processed events that we know are invalid and will be rolled
back anyways.

I suggest the function signature tw_suspend(tw_lp *lp, int
do_suspend_event_rc, const char * format, ...), with the following
semantics:

After a call to tw_suspend, all subsequent events (both forward and
reverse) that arrive at the suspended LP shall be processed as if they
were no-ops. The reverse event handler of the event that caused the
suspend will be run if do_orig_event_rc is nonzero; otherwise, the
reverse event handler shall additionally be a no-op. Typically,
do_orig_event_rc == 0 is desired, as good coding practices for
moderate-or-greater complexity simulations dictate state/event
validation prior to modifying LP state (partial rollbacks are very
undesirable), but there may be messy logic in the user code for which
a partial rollback is warranted (operations that free memory as a side
effect of operations, for example).  An LP exits suspend state upon
rolling back the event that caused the suspend (whether or not that
event is processed as a no-op).  Upon GVT, if an LP is in self-suspend
mode and the event that caused the suspend has a timestamp less than
that of GVT, then the simulator shall report the format string of
suspended LP(s) and exit.  A NULL format string is acceptable for
performance purposes, e.g. when doing "production" simulation runs.

@param lp Pointer to the LP we're suspending
@param do_orig_event_rc A bool indicating whether or not to skip the RC function
@param error_num User-specified value for tracking purposes; ROSS ignores this

*************************************************************************/

void
tw_lp_suspend(tw_lp * lp, int do_orig_event_rc, int error_num )
{
  if(!lp)
    tw_error(TW_LOC, "Bad LP pointer!");

  lp->suspend_flag=1;
  lp->suspend_event = lp->pe->cur_event; // only valid prior to GVT
  lp->suspend_time = tw_now(lp);
  lp->suspend_error_number = error_num;
  lp->suspend_do_orig_event_rc = do_orig_event_rc;

}
