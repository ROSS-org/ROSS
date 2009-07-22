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
  lp->cur_state = tw_calloc(TW_LOC, "state vectors", lp->type.state_sz, 1);
}
