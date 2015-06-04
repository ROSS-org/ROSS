#include <ross.h>

/**
 * @file tw-setup.c
 * @brief tw_define_lps is defined here!
 */

#define VERIFY_MAPPING 0

static tw_pe *setup_pes(void);
unsigned int nkp_per_pe = 16;

static const tw_optdef kernel_options[] = {
    TWOPT_GROUP("ROSS Kernel"),
    TWOPT_UINT("synch", g_tw_synchronization_protocol, "Sychronization Protocol: SEQUENTIAL=1, CONSERVATIVE=2, OPTIMISTIC=3, OPTIMISTIC_DEBUG=4"),
    TWOPT_UINT("nkp", nkp_per_pe, "number of kernel processes (KPs) per pe"),
    TWOPT_STIME("end", g_tw_ts_end, "simulation end timestamp"),
    TWOPT_UINT("batch", g_tw_mblock, "messages per scheduler block"),
    TWOPT_UINT("extramem", g_tw_events_per_pe_extra, "Number of extra events allocated per PE."),
    TWOPT_UINT("buddy_size", g_tw_buddy_alloc, "delta encoding buddy system allocation (2^X)"),
#ifdef AVL_TREE
    TWOPT_UINT("avl_size", g_tw_avl_node_count, "AVL Treet contians 2^avl_size nodes"),
#endif
    TWOPT_END()
};

void tw_init(int *argc, char ***argv) {
    int i;
#if HAVE_CTIME
    time_t raw_time;
#endif

    tw_opt_add(tw_net_init(argc, argv));

    // Print out the command-line so we know what we passed in
    if (tw_ismaster()) {
        for (i = 0; i < *argc; i++) {
            printf("%s ", (*argv)[i]);
        }
        printf("\n\n");
    }

    // Print our revision if we have it
#ifdef ROSS_VERSION
    if (tw_ismaster()) {
#if HAVE_CTIME
        time(&raw_time);
        printf("%s\n", ctime(&raw_time));
#endif
        printf("ROSS Revision: %s\n\n", ROSS_VERSION);
    }
#endif

    tw_opt_add(kernel_options);
    tw_opt_add(tw_gvt_setup());
    tw_opt_add(tw_clock_setup());

    // by now all options must be in
    tw_opt_parse(argc, argv);

    if(tw_ismaster() && NULL == (g_tw_csv = fopen("ross.csv", "a"))) {
        tw_error(TW_LOC, "Unable to open: ross.csv\n");
    }

    tw_opt_print();

    tw_net_start();
    tw_gvt_start();
}

static void early_sanity_check(void) {
#if ROSS_MEMORY
    if(0 == g_tw_memory_nqueues) {
        tw_error(TW_LOC, "ROSS memory library enabled!");
    }
#endif

    if (!g_tw_npe) {
        tw_error(TW_LOC, "need at least one PE");
    }
    if (!g_tw_nlp) {
        tw_error(TW_LOC, "need at least one LP");
    }
    if (!nkp_per_pe) {
        tw_printf(TW_LOC, "number of KPs (%u) must be >= PEs (%u), adjusting.", g_tw_nkp, g_tw_npe);
        g_tw_nkp = g_tw_npe;
    }
}

/*
 * map: map LPs->KPs->PEs linearly
 */
void map_linear(void) {
    tw_pe   *pe;

    int  nlp_per_kp;
    int  lpid;
    int  kpid;
    int  i;
    int  j;

    // may end up wasting last KP, but guaranteed each KP has == nLPs
    nlp_per_kp = (int)ceil((double) g_tw_nlp / (double) g_tw_nkp);

    if(!nlp_per_kp) {
        tw_error(TW_LOC, "Not enough KPs defined: %d", g_tw_nkp);
    }

    g_tw_lp_offset = g_tw_mynode * g_tw_nlp;

#if VERIFY_MAPPING
    printf("NODE %d: nlp %lld, offset %lld\n", g_tw_mynode, g_tw_nlp, g_tw_lp_offset);
#endif

    for(kpid = 0, lpid = 0, pe = NULL; (pe = tw_pe_next(pe)); ) {
#if VERIFY_MAPPING
        printf("\tPE %d\n", pe->id);
#endif

        for(i = 0; i < nkp_per_pe; i++, kpid++) {
            tw_kp_onpe(kpid, pe);

#if VERIFY_MAPPING
            printf("\t\tKP %d", kpid);
#endif

            for(j = 0; j < nlp_per_kp && lpid < g_tw_nlp; j++, lpid++) {
                tw_lp_onpe(lpid, pe, g_tw_lp_offset+lpid);
                tw_lp_onkp(g_tw_lp[lpid], g_tw_kp[kpid]);

#if VERIFY_MAPPING
                if(0 == j % 20) {
                    printf("\n\t\t\t");
                }
                printf("%lld ", lpid+g_tw_lp_offset);
#endif
            }

#if VERIFY_MAPPING
            printf("\n");
#endif
        }
    }

    if(!g_tw_lp[g_tw_nlp-1]) {
        tw_error(TW_LOC, "Not all LPs defined! (g_tw_nlp=%d)", g_tw_nlp);
    }

    if(g_tw_lp[g_tw_nlp-1]->gid != g_tw_lp_offset + g_tw_nlp - 1) {
        tw_error(TW_LOC, "LPs not sequentially enumerated!");
    }
}

void map_round_robin(void) {
    tw_pe   *pe;

    int  kpid;
    int  i;

    for(i = 0; i < g_tw_nlp; i++) {
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

/**
 * IMPORTANT: This function sets the value for g_tw_nlp which is a rather
 * important global variable.  It is also set in (very few) other places,
 * but mainly just here.
 */
void tw_define_lps(tw_lpid nlp, size_t msg_sz, tw_seed * seed) {
    int  i;

    g_tw_nlp = nlp;

#ifdef ROSS_MEMORY
    g_tw_memory_sz = sizeof(tw_memory);
#endif

    g_tw_msg_sz = msg_sz;
    g_tw_rng_seed = seed;

    early_sanity_check();

    /*
     * Construct the KP array.
     */
    if( g_tw_nkp == 1 && g_tw_synchronization_protocol != OPTIMISTIC_DEBUG ) { // if it is the default, then check with the overide param
        g_tw_nkp = nkp_per_pe * g_tw_npe;
    }
    // else assume the application overloaded and has BRAINS to set its own g_tw_nkp

    g_tw_kp = (tw_kp **) tw_calloc(TW_LOC, "KPs", sizeof(*g_tw_kp), g_tw_nkp);

    /*
     * Construct the LP array.
     */
    g_tw_lp = (tw_lp **) tw_calloc(TW_LOC, "LPs", sizeof(*g_tw_lp), g_tw_nlp);

    switch(g_tw_mapping) {
        case LINEAR:
            map_linear();
            break;

        case ROUND_ROBIN:
            map_round_robin();
            break;

        case CUSTOM:
            if( g_tw_custom_initial_mapping ) {
                g_tw_custom_initial_mapping();
            } else {
                tw_error(TW_LOC, "CUSTOM mapping flag set but not custom mapping function! \n");
            }
            break;

        default:
            tw_error(TW_LOC, "Bad value for g_tw_mapping %d \n", g_tw_mapping);
    }

    // init LP RNG stream(s)
    for(i = 0; i < g_tw_nlp; i++) {
        if(g_tw_rng_default == 1) {
            tw_rand_init_streams(g_tw_lp[i], g_tw_nRNG_per_lp);
        }
    }
}

static void late_sanity_check(void) {
    tw_kpid  i;
    tw_lptype null_type;
    tw_kp *kp;

    memset(&null_type, 0, sizeof(null_type));

    /* KPs must be mapped . */
    // should we worry if model doesn't use all defined KPs?  probably not.
    for (i = 0; i < g_tw_nkp; i++) {
        kp = tw_getkp(i);
        if( kp == NULL ) {
            tw_error(TW_LOC, "Local KP %u is NULL \n", i);
        }

        if (kp->pe == NULL) {
            tw_error(TW_LOC, "Local KP %u has no PE assigned.", i);
        }
    }

    /* LPs KP and PE must agree. */
    for (i = 0; i < g_tw_nlp; i++) {
        tw_lp *lp = g_tw_lp[i];

        if (!lp || !lp->pe) {
            tw_error(TW_LOC, "LP %u has no PE assigned.", i);
        }

        if (!lp->kp) {
            tw_error(TW_LOC, "LP not mapped to a KP!");
        }

        if (lp->pe != lp->kp->pe) {
            tw_error(TW_LOC, "LP %u has mismatched KP and PE.", lp->id);
        }

        if (!memcmp(&lp->type, &null_type, sizeof(null_type))) {
            tw_error(TW_LOC, "LP %u has no type.", lp->id);
        }
    }
}

#ifdef USE_BGPM
unsigned cacheList[] = {
    PEVT_L2_HITS,
    PEVT_L2_MISSES,
    PEVT_L2_FETCH_LINE,
    PEVT_L2_STORE_LINE,
};

unsigned instList[] = {
    PEVT_LSU_COMMIT_CACHEABLE_LDS,
    PEVT_L1P_BAS_MISS,
    PEVT_INST_XU_ALL,
    PEVT_INST_QFPU_ALL,
    PEVT_INST_QFPU_FPGRP1,
    PEVT_INST_ALL,
};
#endif

void tw_run(void) {
    tw_pe *me;

    late_sanity_check();

    me = setup_pes();

#ifdef USE_BGPM
    Bgpm_Init(BGPM_MODE_SWDISTRIB);

    // Create event set and add events needed to calculate CPI,
    int hEvtSet = Bgpm_CreateEventSet();

    // Change the type here to get the other performance monitor sets -- defaults to instList set
    // Sets need to be different because there are conflicts when mixing certain events.
    Bgpm_AddEventList(hEvtSet, instList, sizeof(instList)/sizeof(unsigned) );

    if( me->id == 0) {
        printf("***************************************************************************************** \n");
        printf("* NOTICE: Build configured with Blue Gene/Q specific, BGPM performance monitoring code!!* \n");
        printf("***************************************************************************************** \n");
    }

    Bgpm_Apply(hEvtSet);
    Bgpm_Start(hEvtSet);
#endif

    tw_sched_init(me);

    switch(g_tw_synchronization_protocol) {
        case SEQUENTIAL:    // case 1
            tw_scheduler_sequential(me);
            break;

        case CONSERVATIVE: // case 2
            tw_scheduler_conservative(me);
            break;

        case OPTIMISTIC:   // case 3
            tw_scheduler_optimistic(me);
            break;

        case OPTIMISTIC_DEBUG:  // case 4
            tw_scheduler_optimistic_debug(me);
            break;

        default:
            tw_error(TW_LOC, "No Synchronization Protocol Specified! \n");
    }

#ifdef USE_BGPM
    Bgpm_Stop(hEvtSet);

    if( me->id == 0 ) { // Have only RANK 0 output stats
        int i;
        uint64_t cnt;
        int numEvts = Bgpm_NumEvents(hEvtSet);
        printf("\n \n ================================= \n");
        printf( "Performance Counter Results:\n");
        printf("--------------------------------- \n");
        for (i=0; i<numEvts; i++) {
            Bgpm_ReadEvent(hEvtSet, i, &cnt);
            printf("   %40s = %20llu\n", Bgpm_GetEventLabel(hEvtSet, i), cnt);
        }
        printf("================================= \n");
    }
#endif
}

void tw_end(void) {
    if(tw_ismaster()) {
        fprintf(g_tw_csv, "\n");
        fclose(g_tw_csv);
    }

    tw_net_stop();
}

int LZ4_compressBound(int isize);

/**
 * By the time this function gets called, g_tw_delta_sz should be as large
 * as it will ever get.
 */
static void tw_delta_alloc(tw_pe *pe) {
    g_tw_delta_sz = LZ4_compressBound(g_tw_delta_sz);

    pe->delta_buffer[0] = tw_calloc(TW_LOC, "Delta buffers", g_tw_delta_sz, 1);
    pe->delta_buffer[1] = tw_calloc(TW_LOC, "Delta buffers", g_tw_delta_sz, 1);
    pe->delta_buffer[2] = tw_calloc(TW_LOC, "Delta buffers", g_tw_delta_sz, 1);
}

static tw_pe * setup_pes(void) {
    tw_pe   *pe;
    tw_pe   *master;

    int  i;
    unsigned int num_events_per_pe;

    num_events_per_pe = 1 + g_tw_events_per_pe + g_tw_events_per_pe_extra;

    master = g_tw_pe[0];

    if (!master) {
        tw_error(TW_LOC, "No PE configured on this node.");
    }

    if (g_tw_mynode == g_tw_masternode) {
        master->master = 1;
    }
    master->local_master = 1;

    for(i = 0; i < g_tw_npe; i++) {
        pe = g_tw_pe[i];
        if (g_tw_buddy_alloc) {
            g_tw_buddy_master = create_buddy_table(g_tw_buddy_alloc);
            if (g_tw_buddy_master == NULL) {
                tw_error(TW_LOC, "create_buddy_table() failed.");
            }
            tw_delta_alloc(pe);
        }
        pe->pq = tw_pq_create();

        tw_eventq_alloc(&pe->free_q, num_events_per_pe);
        pe->abort_event = tw_eventq_shift(&pe->free_q);
    }

    if (g_tw_mynode == g_tw_masternode) {
        printf("\nROSS Core Configuration: \n");
        printf("\t%-50s %11u\n", "Total Nodes", tw_nnodes());
        fprintf(g_tw_csv, "%u,", tw_nnodes());

        printf("\t%-50s [Nodes (%u) x PE_per_Node (%lu)] %lu\n", "Total Processors", tw_nnodes(), g_tw_npe, (tw_nnodes() * g_tw_npe));
        fprintf(g_tw_csv, "%lu,", (tw_nnodes() * g_tw_npe));

        printf("\t%-50s [Nodes (%u) x KPs (%lu)] %lu\n", "Total KPs", tw_nnodes(), g_tw_nkp, (tw_nnodes() * g_tw_nkp));
        fprintf(g_tw_csv, "%lu,", (tw_nnodes() * g_tw_nkp));

        printf("\t%-50s %11lu\n", "Total LPs", (tw_nnodes() * g_tw_npe * g_tw_nlp));
        fprintf(g_tw_csv, "%lu,", (tw_nnodes() * g_tw_npe * g_tw_nlp));

        printf("\t%-50s %11.2lf\n", "Simulation End Time", g_tw_ts_end);
        fprintf(g_tw_csv, "%11.2lf\n", g_tw_ts_end);


        switch(g_tw_mapping) {
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
        printf("\t%-50s %11d\n", "Model events", num_events_per_pe);
        fprintf(g_tw_csv, "%d,", num_events_per_pe);

        printf("\t%-50s %11d\n", "Network events", g_tw_gvt_threshold);
        fprintf(g_tw_csv, "%d,", g_tw_gvt_threshold);

        printf("\t%-50s %11d\n", "Total events", g_tw_events_per_pe);
        fprintf(g_tw_csv, "%d,", g_tw_events_per_pe);
        printf("\n");
#endif
    }
    return master;
}

// This is the default lp type mapping function
// valid ONLY if there is one lp type
tw_lpid map_onetype (tw_lpid gid) {
    return 0;
}
