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

static tw_pe *setup_pes(void);
static unsigned int nkp_per_pe = 1;

static const tw_optdef kernel_options[] = {
	TWOPT_GROUP("ROSS Kernel"),
	TWOPT_UINT("synch", g_tw_synchronization_protocol, 
		   "Sychronization Protocol: SEQUENTIAL=1, CONSERVATIVE=2, OPTIMISTIC=3"),
	TWOPT_UINT("nkp", nkp_per_pe, "number of kernel processes (KPs) per pe"),
	TWOPT_STIME("end", g_tw_ts_end, "simulation end timestamp"),
	TWOPT_UINT("batch", g_tw_mblock, "messages per scheduler block"),
	TWOPT_END()
};

void
tw_init(int *argc, char ***argv)
{
	tw_opt_add(tw_net_init(argc, argv));
	tw_opt_add(kernel_options);
	tw_opt_add(tw_gvt_setup());
	tw_opt_add(tw_clock_setup());

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
#if ROSS_MEMORY
	if(0 == g_tw_memory_nqueues)
		tw_error(TW_LOC, "ROSS memory library enabled!");
#endif

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

void
map_none(void)
{
}

/*
 * map: map LPs->KPs->PEs linearly
 */
void
map_linear(void)
{
	tw_pe	*pe;

	int	 nlp_per_kp;
	int	 lpid;
	int	 kpid;
	int	 i;
	int	 j;

	// may end up wasting last KP, but guaranteed each KP has == nLPs
	nlp_per_kp = ceil((double) g_tw_nlp / (double) g_tw_nkp);

	if(!nlp_per_kp)
		tw_error(TW_LOC, "Not enough KPs defined: %d", g_tw_nkp);

	g_tw_lp_offset = g_tw_mynode * g_tw_nlp;

#if VERIFY_MAPPING
	printf("NODE %d: nlp %lld, offset %lld\n", 
		g_tw_mynode, g_tw_nlp, g_tw_lp_offset);
#endif

	for(kpid = 0, lpid = 0, pe = NULL; (pe = tw_pe_next(pe)); )
	{
#if VERIFY_MAPPING
		printf("\tPE %d\n", pe->id);
#endif

		for(i = 0; i < nkp_per_pe; i++, kpid++)
		{
			tw_kp_onpe(kpid, pe);

#if VERIFY_MAPPING
			printf("\t\tKP %d", kpid);
#endif

			for(j = 0; j < nlp_per_kp && lpid < g_tw_nlp; j++, lpid++)
			{
				tw_lp_onpe(lpid, pe, g_tw_lp_offset+lpid);
				tw_lp_onkp(g_tw_lp[lpid], g_tw_kp[kpid]); 

#if VERIFY_MAPPING
				if(0 == j % 20)
					printf("\n\t\t\t");
				printf("%lld ", lpid+g_tw_lp_offset);
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
}

void
map_round_robin(void)
{
	tw_pe	*pe;

	int	 kpid;
	int	 i;

	for(i = 0; i < g_tw_nlp; i++)
	{
		kpid = i % g_tw_nkp;
		pe = tw_getpe(kpid % g_tw_npe);

		tw_lp_onpe(i, pe, g_tw_lp_offset+i);
		tw_kp_onpe(kpid, pe);
		tw_lp_onkp(g_tw_lp[i], g_tw_kp[kpid]);

#if VERIFY_MAPPING
		printf("LP %4d KP %4d PE %4d\n", i, kp->id, pe->id);
#endif
	}
}

void
tw_define_lps(tw_lpid nlp, size_t msg_sz, tw_seed * seed)
{
	int	 i;

	1 == tw_nnodes() ? g_tw_nlp = nlp * g_tw_npe : (g_tw_nlp = nlp);

#ifdef ROSS_MEMORY
	g_tw_memory_sz = sizeof(tw_memory);
#endif

	g_tw_msg_sz = msg_sz;
	g_tw_rng_seed = seed;

	early_sanity_check();

	/*
	 * Construct the KP array.
	 */
	if( g_tw_nkp == 1 ) // if it is the default, then check with the overide param
	  g_tw_nkp = nkp_per_pe * g_tw_npe;
	// else assume the application overloaded and has BRAINS to set its own g_tw_nkp

	g_tw_kp = tw_calloc(TW_LOC, "KPs", sizeof(*g_tw_kp), g_tw_nkp);

	/*
	 * Construct the LP array.
	 */
	g_tw_lp = tw_calloc(TW_LOC, "LPs", sizeof(*g_tw_lp), g_tw_nlp);

	switch(g_tw_mapping)
	  {
	  case LINEAR:
	    map_linear();
	    break;
	    
	  case ROUND_ROBIN:
	    map_round_robin();
	    break;
	    
	  case CUSTOM:
	    if( g_tw_custom_initial_mapping )
	      {
		g_tw_custom_initial_mapping();
	      }
	    else
	      {
		tw_error(TW_LOC, "CUSTOM mapping flag set but not custom mapping function! \n");
	      }
	    break;
	    
	  default:
	    tw_error(TW_LOC, "Bad value for g_tw_mapping %d \n", g_tw_mapping);
	  }

	// init LP RNG stream(s)
	for(i = 0; i < g_tw_nlp; i++)
		if(g_tw_rng_default == TW_TRUE)
			tw_rand_init_streams(g_tw_lp[i], g_tw_nRNG_per_lp);
}

static void
late_sanity_check(void)
{
	tw_kpid	 i;
	tw_lptype null_type;
        tw_kp *kp;

	memset(&null_type, 0, sizeof(null_type));

	/* KPs must be mapped . */
	// should we worry if model doesn't use all defined KPs?  probably not.
	for (i = 0; i < g_tw_nkp; i++)
	  {
	    kp = tw_getkp(i);
	    if( kp == NULL )
	      tw_error(TW_LOC, "Local KP %u is NULL \n", i);

	    if (kp->pe == NULL)
	      tw_error(TW_LOC, "Local KP %u has no PE assigned.", i);
	}

	/* LPs KP and PE must agree. */
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
  
  late_sanity_check();
  
  me = setup_pes();
  
  switch(g_tw_synchronization_protocol)
    {
    case SEQUENTIAL:    // case 1
      tw_scheduler_sequential(me);
      break;
      
    case CONSERVATIVE: // case 2
      tw_scheduler_conservative(me);
      break;
      
    case OPTIMISTIC:   // case 3
      tw_scheduler_optimistic(me);
      break;

    default:
      tw_error(TW_LOC, "No Synchronization Protocol Specified! \n");
    }
}

void
tw_end(void)
{
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
	}

	if(tw_node_eq(&g_tw_mynode, &g_tw_masternode))
	{
		printf("\nROSS Core Configuration: \n");
		printf("\t%-50s %11u\n", "Total Nodes", tw_nnodes());
		fprintf(g_tw_csv, "%u,", tw_nnodes());

		printf("\t%-50s [Nodes (%u) x PE_per_Node (%u)] %u\n", "Total Processors", tw_nnodes(), g_tw_npe, (tw_nnodes() * g_tw_npe));
		fprintf(g_tw_csv, "%u,", (tw_nnodes() * g_tw_npe));

		printf("\t%-50s [Nodes (%u) x KPs (%u)] %11u\n", "Total KPs", tw_nnodes(), g_tw_nkp, (tw_nnodes() * g_tw_nkp));
		fprintf(g_tw_csv, "%u,", (tw_nnodes() * g_tw_nkp));

		printf("\t%-50s %11llu\n", "Total LPs", (tw_nnodes() * g_tw_npe * g_tw_nlp));
		fprintf(g_tw_csv, "%llu,", (tw_nnodes() * g_tw_npe * g_tw_nlp));

		printf("\t%-50s %11.2lf\n", "Simulation End Time", g_tw_ts_end);

		switch(g_tw_mapping)
		  {
		  case LINEAR:
		    printf("\t%-50s %11s\n", "LP-to-PE Mapping", "linear");
		    fprintf(g_tw_csv, "%s,", "linear");
		    break;

		  case ROUND_ROBIN:
		    printf("\t%-50s %11s\n", "LP-to-PE Mapping", "round robin");
		    fprintf(g_tw_csv, "%s,", "round robin");
		    break;

		  case CUSTOM:
		    printf("\t%-50s %11s\n", "LP-to-PE Mapping", "model defined");
		    fprintf(g_tw_csv, "%s,", "model defined");
		    break;
		  }
		printf("\n");

#ifndef ROSS_DO_NOT_PRINT
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
#endif
	}
	return master;
}


