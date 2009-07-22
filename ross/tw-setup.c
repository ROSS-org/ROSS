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

#define VERIFY_MAPPING 0

static void init_locks(void);
static tw_pe *setup_pes(void);
static unsigned int nkp_per_pe = 1;

static const tw_optdef kernel_options[] = {
	TWOPT_GROUP("ROSS Kernel"),
	TWOPT_UINT("nkp", nkp_per_pe, "number of kernel processes (KPs) per pe"),
	TWOPT_STIME("end", g_tw_ts_end, "simulation end timestamp"),
	TWOPT_UINT("batch-size", g_tw_mblock, "messages per scheduler block"),
	TWOPT_END()
};

void
tw_init(int *argc, char ***argv)
{
	tw_opt_add(tw_net_init(argc, argv));
	tw_opt_add(kernel_options);
	tw_opt_add(tw_gvt_setup());

	// by now all options must be in
	tw_opt_parse(argc, argv);

	if(tw_ismaster() && NULL == (g_tw_csv = fopen("ross.csv", "a")))
		tw_error(TW_LOC, "Unable to open: ross.csv\n");

	tw_opt_print();

	tw_net_start();
	tw_gvt_start();
	//tw_register_signals();
}

static void
early_sanity_check(void)
{
	if (!g_tw_npe)
		tw_error(TW_LOC, "need at least one PE");
	if (!g_tw_nlp)
		tw_error(TW_LOC, "need at least one LP");
	if (!nkp_per_pe)
	{
		tw_printf(TW_LOC,
			"number of KPs (%u) must be >= PEs (%u), adjusting.",
			g_tw_nkp, g_tw_npe);
		g_tw_nkp = g_tw_npe;
	}
}

/*
 * map: perform mapping function as defined by compiler option
 *
 * 1. none: the model provides all LP to KP to PE mappings
 * 2. linear: linear mapping of LPs to KPs to PEs
 * 3. rrobin: round robin mapping of LPs to KPs to PEs
 */
void
map(void)
{
#ifdef ROSS_MAPPING_none
	// just a place-holder, move on, nothing to see here
#elif ROSS_MAPPING_linear
	tw_pe	*pe;
	tw_kp	*kp;

	int	 nlp_per_kp;
	int	 lpid;
	int	 kpid;
	int	 i;
	int	 j;

	// may end up wasting last KP, but guaranteed each KP has == nLPs
	nlp_per_kp = ceil((double) g_tw_nlp / (double) g_tw_nkp);

	if(!nlp_per_kp)
		tw_error(TW_LOC, "Not enough KPs defined: %d", g_tw_nkp);

	g_tw_memory_nqueues *= 2;
	g_tw_lp_offset = g_tw_mynode * g_tw_nlp;

	for(kpid = 0, lpid = 0, pe = NULL; (pe = tw_pe_next(pe)); )
	{
#if VERIFY_MAPPING
		printf("\tPE %d\n", pe->id);
#endif

		for(i = 0; i < nkp_per_pe; i++, kpid++)
		{
			kp = &g_tw_kp[kpid];
			kp->id = kpid;

			tw_kp_onpe(kp, pe);

#if 0
			kp->queues = tw_calloc(TW_LOC, "KP queue",
					sizeof(tw_memoryq),
					g_tw_memory_nqueues);

			for(j = 0; j < g_tw_memory_nqueues; j++)
				kp->queues[j].size = -1;
#endif

#if VERIFY_MAPPING
			printf("\t\tKP %d", kp->id);
#endif

			for(j = 0; j < nlp_per_kp && lpid < g_tw_nlp; j++, lpid++)
			{
				tw_lp_onpe(lpid, pe, g_tw_lp_offset+lpid);
				tw_lp_onkp(g_tw_lp[lpid], kp); 

#if VERIFY_MAPPING
				if(0 == j % 20)
					printf("\n\t\t\t");
				printf("%d ", lpid);
#endif
			}

#if VERIFY_MAPPING
			printf("\n");
#endif
		}
	}

	if(!g_tw_lp[g_tw_nlp-1])
		tw_error(TW_LOC, "Not all LPs defined! (g_tw_nlp=%d)", g_tw_nlp);

	if(g_tw_lp[g_tw_nlp-1]->gid != g_tw_lp_offset + g_tw_nlp - 1)
		tw_error(TW_LOC, "LPs not sequentially enumerated!");

#elif ROSS_MAPPING_rrobin
	tw_pe	*pe;
	tw_kp	*kp;

	int	 kpid;
	int	 i;

	for(i = 0; i < g_tw_nlp; i++)
	{
		kpid = i % g_tw_nkp;
		kp = &g_tw_kp[kpid];
		pe = tw_getpe(kpid % g_tw_npe);

		kp->id = kpid;
	
		tw_lp_onpe(i, pe, g_tw_lp_offset+i);
		tw_lp_onkp(g_tw_lp[i], kp);
		tw_kp_onpe(kp, pe);

#if VERIFY_MAPPING
		printf("LP %4d KP %4d PE %4d\n", i, kp->id, pe->id);
#endif
	}
#endif
}

void
tw_define_lps(tw_lpid nlp, size_t msg_sz, tw_lpid nrng)
{
	g_tw_nlp = nlp;
	g_tw_msg_sz = msg_sz;
	early_sanity_check();

	/*
	 * Construct the KP array.
	 * Default KP on PE mapping is linear.
	 */
	g_tw_nkp = nkp_per_pe * g_tw_npe;
	g_tw_kp = tw_calloc(TW_LOC, "KPs", sizeof(tw_kp), g_tw_nkp);

	/*
	 * Construct the LP array.
	 * LP to PE mapping is model-defined.
	 */
	g_tw_lp = tw_calloc(TW_LOC, "LPs", sizeof(*g_tw_lp), g_tw_nlp);

	/* Setup our random number generators. */
	if(nrng)
		tw_rand_init(31, 41, NULL, nrng);
	else
		tw_rand_init(31, 41, NULL, 0);

	map();
}

static void
late_sanity_check(void)
{
	tw_kp	*kp;
	tw_kpid	 i;
	tw_lptype null_type;

	memset(&null_type, 0, sizeof(null_type));

	/* KPs must be mapped . */
	// should we worry if model doesn't use all defined KPs?  probably not.
	for (i = 0; i < g_tw_nkp; i++)
	{
		if (!tw_getkp(i)->pe)
			tw_error(TW_LOC, "KP %u has no PE assigned.", i);
	}

	/* LPs KP and PE must agree. */
	kp = &g_tw_kp[0];
	for (i = 0; i < g_tw_nlp; i++)
	{
		tw_lp *lp = g_tw_lp[i];

		if (!lp || !lp->pe)
			tw_error(TW_LOC, "LP %u has no PE assigned.", i);

		if (!lp->kp)
			tw_error(TW_LOC, "LP not mapped to a KP!");

		if (lp->pe != lp->kp->pe)
			tw_error(TW_LOC, "LP %u has mismatched KP and PE.", lp->id);

		if (!memcmp(&lp->type, &null_type, sizeof(null_type)))
			tw_error(TW_LOC, "LP %u has no type.", lp->id);
	}
}

void
tw_run(void)
{
	tw_pe *me;

	early_sanity_check();
	late_sanity_check();

	init_locks();
	me = setup_pes();
	if(g_tw_npe == 1 && tw_nnodes() == 1)
		tw_scheduler_seq(me);
	else
		tw_scheduler(me);

	if(tw_ismaster())
	{
		fprintf(g_tw_csv, "\n");
		fclose(g_tw_csv);
	}

	tw_net_stop();
}

static tw_pe *
setup_pes(void)
{
	tw_pe	*pe;
	tw_pe	*master;

	int	 i;

	master = g_tw_pe[0];

	if (!master)
		tw_error(TW_LOC, "No PE configured on this node.");

	if(tw_node_eq(&g_tw_mynode, &g_tw_masternode))
		master->master = 1;
	master->local_master = 1;

	for(i = 0; i < g_tw_npe; i++)
	{
		pe = g_tw_pe[i];
		pe->pq = tw_pq_create();

		tw_eventq_alloc(&pe->free_q, 1 + g_tw_events_per_pe);
		pe->abort_event = tw_eventq_shift(&pe->free_q);

		//g_tw_gvt_threshold = ceil(g_tw_events_per_pe / 10);
	}

	if(tw_node_eq(&g_tw_mynode, &g_tw_masternode))
	{
		printf("\nROSS Configuration: \n");
		printf("\t%-50s %11d\n", "Total Nodes", tw_nnodes());
		fprintf(g_tw_csv, "%d,", tw_nnodes());

		printf("\t%-50s %11d\n", "Total Processors", tw_nnodes() * g_tw_npe);
		fprintf(g_tw_csv, "%d,", tw_nnodes() * g_tw_npe);

		printf("\t%-50s %11d\n", "Total KPs", tw_nnodes() * g_tw_nkp);
		fprintf(g_tw_csv, "%d,", tw_nnodes() * g_tw_nkp);

		printf("\t%-50s %11lld\n", "Total LPs", (tw_nnodes() * g_tw_npe * g_tw_nlp));
		fprintf(g_tw_csv, "%lld,", (tw_nnodes() * g_tw_npe * g_tw_nlp));

		printf("\t%-50s %11.2lf\n", "Simulation End Time", g_tw_ts_end);

#ifdef ROSS_MAPPING_linear
		printf("\t%-50s %11s\n", "LP-to-PE Mapping", "linear");
		fprintf(g_tw_csv, "%s,", "linear");
#elif ROSS_MAPPING_rrobin
		printf("\t%-50s %11s\n", "LP-to-PE Mapping", "round robin");
		fprintf(g_tw_csv, "%s,", "round robin");
#else
		printf("\t%-50s %11s\n", "LP-to-PE Mapping", "model defined");
		fprintf(g_tw_csv, "%s,", "model defined");
#endif

		printf("\nROSS Event Memory Allocation:\n");
		printf("\t%-50s %11d\n", "Model events", 
			g_tw_events_per_pe - g_tw_gvt_threshold);
		fprintf(g_tw_csv, "%d,", g_tw_events_per_pe - g_tw_gvt_threshold);

		printf("\t%-50s %11d\n", "Network events", 
			g_tw_gvt_threshold);
		fprintf(g_tw_csv, "%d,", g_tw_gvt_threshold);

		printf("\t%-50s %11d\n", "Total events", 
			g_tw_events_per_pe);
		fprintf(g_tw_csv, "%d,", g_tw_events_per_pe);

		printf("\n");
	}

	for(i = 1; i < g_tw_npe; i++)
		tw_thread_create((void (*)(void *))tw_scheduler, g_tw_pe[i]);

	return master;
}

static void
init_locks(void)
{
	tw_barrier_create(&g_tw_simstart);
	tw_barrier_create(&g_tw_simend);
	tw_barrier_create(&g_tw_network);
	tw_barrier_create(&g_tw_gvt_b);
	tw_mutex_create(&g_tw_debug_lck);
}
