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

	/*
	 * LP data structures are allocated dynamically when the
	 * process starts up based on the number it requires.
	 *
	 * g_tw_nlp         -- Number of LPs in simulation.
	 * g_tw_lp_offset   -- global id of g_tw_lp[0]
	 * g_tw_nkp         -- Number of KPs in simulation.
	 * g_tw_lp          -- Public LP objects.
	 * g_tw_lp          -- Public KP objects.
	 * g_tw_sv_growcnt  -- Add this many SV's to an LP if it is empty.
	 * g_tw_fossil_attempts  -- Number of times fossil_collect is called
         * g_tw_nRNG_per_lp -- Number of RNG per LP
	 */
tw_lpid         g_tw_nlp = 0;
tw_lpid		g_tw_lp_offset = 0;
tw_kpid         g_tw_nkp = 1;
tw_lp		**g_tw_lp;
tw_kp          *g_tw_kp = NULL;
int             g_tw_sv_growcnt = 10;
int             g_tw_fossil_attempts = 0;
unsigned int    g_tw_nRNG_per_lp = 1;
unsigned int	g_tw_sim_started = 0;
size_t g_tw_msg_sz;

unsigned int	g_tw_memory_nqueues = 0;
size_t		g_tw_memory_sz = 0;
size_t		g_tw_event_msg_sz = 0;

	/*
	 * Number of messages to process at once out of the PQ before
	 * returning back to handling things like GVT, message recption,
	 * etc.
	 */
unsigned int g_tw_mblock = 16;
tw_stime        g_tw_ts_end = 100000.0;

	/*
	 * g_tw_npe             -- Number of PEs.
	 * g_tw_pe              -- Public PE objects.
	 * g_tw_events_per_pe   -- Number of events to place in for each PE.
	 *                         MUST be > 1 because of abort buffer.
	 */
tw_peid		g_tw_npe = 1;
tw_pe **g_tw_pe;
int             g_tw_events_per_pe = 2048;
unsigned int	g_tw_master = 0;

	/*
	 * Barriers and global lock objects which are needed for this
	 * _node_ and its threads running on it.  Remote nodes do not
	 * share these barriers and locks, they have their own. BUT,
	 * remote nodes must be synchronized with all other remote 
	 * nodes/pe's.
	 */
tw_barrier      g_tw_simstart;
tw_barrier      g_tw_simend;
tw_barrier      g_tw_network;
tw_barrier      g_tw_gvt_b;
tw_mutex        g_tw_debug_lck;

unsigned int	g_tw_gvt_threshold = 1000;
unsigned int	g_tw_gvt_done = 0;

	/*
	 * Network variables:
	 *
	 * g_tw_net_barrier_flag -- when set, PEs should stop in next barrier encountered
	 * g_tw_masternode -- pointer to GVT net node, for GVT comp
	 */
unsigned int	g_tw_net_device_size = 0;
tw_node		g_tw_mynode = -1;
tw_node		g_tw_masternode = -1;

FILE		*g_tw_csv = NULL;
