/*
 * ROSS: Rensselaer's Optimistic Simulation System.
 * Copyright (c) 1999-2003 Rensselaer Polytechnic Instutitute.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *
 *      This product includes software developed by David Bauer,
 *      Dr. Christopher D.  Carothers, and Shawn Pearce of the
 *      Department of Computer Science at Rensselaer Polytechnic
 *      Institute.
 *
 * 4. Neither the name of the University nor of the developers may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * 5. The use or inclusion of this software or its documentation in
 *    any commercial product or distribution of this software to any
 *    other party without specific, written prior permission is
 *    prohibited.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS
 * IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *
 */

#include <ross.h>

#if 0
tw_lp *
tw_lp_next_onpe(tw_lp * last, tw_pe * pe)
{
	if (!last)
		return pe->lp_list;

	if (!last->pe_next || last->pe != pe)
		return NULL;

	return last->pe_next;
}
#endif

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

	if(g_tw_lp[id] && g_tw_lp[id]->pe)
		tw_error(TW_LOC, "refusing to move LP %u from PE %u to PE %u",
			id, g_tw_lp[id]->pe->id, pe->id);

	g_tw_lp[id] = tw_calloc(TW_LOC, "Local LP", sizeof(tw_lp), 1);

	if(!tw_rand_configured())
		g_tw_lp[id]->rng = tw_calloc(TW_LOC, "LP RNG", 
					sizeof(tw_generator), g_tw_nRNG_per_lp);

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
	tw_lp *prev_lp = NULL;

	tw_lpid i;
	tw_lpid j;

	for (i = 0; i < g_tw_nlp; i++)
	{
		tw_lp *lp = g_tw_lp[i];

		if (lp->pe != me)
			continue;

		if(!tw_rand_configured())
		{
			for(j = 0; j < g_tw_nRNG_per_lp; j++)
			{
				//lp->rng[j].id = (lp->gid * g_tw_nRNG_per_lp) + j;
				tw_rand_initial_seed(&lp->rng[j], ((lp->gid * g_tw_nRNG_per_lp) + j));
			}
		}

#if 0
		if (prev_lp)
			prev_lp->pe_next = lp;
		else
			me->lp_list = lp;
#endif

		prev_lp = lp;

		/*
		 * Allocate initial state vectors for this LP, and
		 * steal one of them for the initial state to be
		 * handed to the init function.
		 */
		if (lp->type.revent)
			tw_state_alloc(lp, 1);
		else
			tw_state_alloc(lp, g_tw_sv_growcnt + 1);

/*
		lp->cur_state = lp->state_qh;
		lp->state_qh = lp->state_qh->next;
*/

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
