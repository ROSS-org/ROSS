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

tw_node mynode = 0;
static int npe = 1;
static char g_tw_net_config[256] = "./.ross_network";

static const tw_optdef net_options[] = {
	TWOPT_GROUP("ROSS-SMP Kernel"),
	TWOPT_CHAR("machines", g_tw_net_config, "CPU configuration file"),
	TWOPT_END()
};

void
tw_net_barrier(tw_pe * pe)
{
}

unsigned
tw_nnodes(void)
{
	return 1;
}

tw_statistics *
tw_net_statistics(tw_pe * me, tw_statistics * s)
{
	return s;
}

const tw_optdef *
tw_net_init(int *argc, char ***argv)
{
	FILE	*f;

	int rv = 0;

	g_tw_masternode = 0;
	g_tw_mynode = 0;

	f = fopen(g_tw_net_config, "r");

	if(!f)
		tw_error(TW_LOC, "Unable to open: %s\n", g_tw_net_config);

	while(':' != fgetc(f))
		;

	rv = fscanf(f, "%d", &npe);

	if(ferror(f))
		perror("Unable to configure ROSS");

	return net_options;
}

void
tw_net_start(void)
{
	tw_peid pi;

	tw_pe_create(npe);
	for (pi = 0; pi < npe; pi++)
		tw_pe_init(pi, pi);
}

void
tw_net_abort(void)
{
	exit(1);
}

void
tw_net_stop(void)
{
}

static void no_network(void) NORETURN;
static void no_network(void)
{
	tw_error(TW_LOC, "Compiled without network support.");
}

void tw_net_read(tw_pe *me)
{
	no_network();
}

void
tw_net_send(tw_event *evt)
{
	no_network();
}

void
tw_net_cancel(tw_event *evt)
{
	no_network();
}

void
tw_send_stats(tw_statistics *s)
{
	no_network();
}

void
tw_recv_stats(tw_statistics *s)
{
	no_network();
}

void
tw_net_send_lvt(tw_pe * pe, tw_stime ts)
{
	no_network();
}

void
tw_net_gvt_compute(tw_pe * pe, tw_stime * ts)
{
	no_network();
}
