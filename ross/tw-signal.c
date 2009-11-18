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
#include <signal.h>

static unsigned int once = 1;

void
tw_sigsegv(int sig)
{
	if (sig == SIGBUS)
		tw_error(TW_LOC, "Caught SIGBUS; terminating.");
	else if (sig == SIGSEGV)
		tw_error(TW_LOC, "Caught SIGSEGV; terminating.");
	else if (sig != SIGINT)
		tw_error(TW_LOC, "Caught unknown signal %d; terminating.", sig);

	/* nothing to report, just exit */
	if(0 == g_tw_sim_started)
		tw_net_abort();

	if (once)
	{
		tw_pe *master = NULL;
		tw_pe *pe = NULL;

		while(NULL != (pe = tw_pe_next(pe)))
		{
			if (pe->local_master == 1)
				master = pe;
		}

		if (master)
			tw_stats(master);
	}

	once = 0;

	tw_printf(TW_LOC, "Caught SIGINT; terminating.");
	tw_net_abort();
}

void
tw_sigterm(int sig)
{
	if (sig != SIGTERM)
		tw_error(TW_LOC, "Caught unknown signal %d; terminating.", sig);
	tw_net_abort();
}
