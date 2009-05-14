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

#ifndef INC_ross_network_h
#define INC_ross_network_h

/* Initalize the network library and parse options.
 *
 * argc and argv are pointers to the original command line; the
 * network library may edit these before the option parser sees
 * them allowing for network implementation specific argument
 * handling to occur.
 *
 * returned tw_optdef array will be included in the overall process
 * command line argument display and parsing; NULL may be returned
 * to indicate the implementation has no options it wants included.
 */
const tw_optdef *tw_net_init(int *argc, char ***argv);

/* Starts the network library after option parsing. */
void tw_net_start(void);

/* Stops the network library after simulation end. */
void tw_net_stop(void);

/* Aborts the entire simulation when a grave error is found. */
void tw_net_abort(void) NORETURN;

/* TODO: Old function definitions not yet replaced/redefined. */
extern void tw_net_read(tw_pe *);
extern void tw_net_send(tw_event *);
extern void tw_net_cancel(tw_event *);

/* Determine the identification of the node a pe is running on. */
INLINE(tw_node *) tw_net_onnode(tw_peid gid);

/* Global PE id -> local PE id mapping function. */
INLINE(tw_peid) tw_net_pemap(tw_peid gid);

/* Determine if two nodes are the same (0 == no, 1 == yes). */
INLINE(int) tw_node_eq(tw_node *a, tw_node *b);

/* Obtain the total number of nodes executing the simulation. */
extern unsigned tw_nnodes(void);

/* Block until all nodes call the barrier. */
extern void tw_net_barrier(tw_pe * pe);

/* Obtain the lowest timestamp inside the network buffers. */
extern tw_stime tw_net_minimum(tw_pe *);

/* Send / receive tw_statistics objects.  */
extern tw_statistics *tw_net_statistics(tw_pe *, tw_statistics *);

/* Communicate LVT / GVT values to compute node network.  */
extern void tw_net_gvt_compute(tw_pe *, tw_stime *);

/* Provide a mechanism to send the PE LVT value to remote processors */
extern void tw_net_send_lvt(tw_pe *, tw_stime);

#endif
