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

#ifndef INC_gvt_mpi_allreduce_h
#define INC_gvt_mpi_allreduce_h

static tw_stime gvt_print_interval = 0.05;
static tw_stime percent_complete = 0.0;

INLINE(int)
tw_gvt_inprogress(tw_pe * pe)
{
	return pe->gvt_status;
}

INLINE(void)
gvt_print(tw_stime gvt)
{
	if(gvt_print_interval == 1.0)
		return;

	if(percent_complete == 0.0)
	{
		percent_complete = gvt_print_interval;
		return;
	}

	printf("GVT #%d: simulation %d%% complete (",
		g_tw_gvt_done,
		(int) min(100, floor(100 * (gvt/g_tw_ts_end))));

	if (gvt == DBL_MAX)
		printf("GVT = %s", "MAX");
	else
		printf("GVT = %.4f", gvt);

	printf(").\n");
	percent_complete += gvt_print_interval;
}

#endif
