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

#ifndef INC_gvt_7oclock_h
#define INC_gvt_7oclock_h

	/* Clock Computation Variables:
	 *
	 * The clock is used to implement the 7 O'Clock Algorithm, but
	 * is useful in other areas, such as determining how long it takes to
	 * actually complete tasks, such as enq's and deq's.
	 */
static tw_volatile int g_tw_7oclock_node_flag;
static tw_volatile tw_clock g_tw_clock_max_send_delta_t;
static tw_volatile tw_clock g_tw_clock_gvt_interval;
static tw_volatile tw_clock g_tw_clock_gvt_window_size;

static tw_stime gvt_print_interval = 0.1;
static tw_stime percent_complete = 0.0;

static inline int 
tw_gvt_inprogress(tw_pe * pe)
{
#if 0
	return (g_tw_7oclock_node_flag == -g_tw_npe && 
		tw_clock_now(pe) < g_tw_clock_gvt_interval ? 0 : 1);
#endif
	return (g_tw_7oclock_node_flag >= 0 || 
		tw_clock_now(pe) + g_tw_clock_max_send_delta_t >= g_tw_clock_gvt_interval);
}

static inline void 
gvt_print(tw_stime gvt)
{
	if(gvt_print_interval > 1.0)
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
