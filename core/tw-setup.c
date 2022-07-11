#include <ross.h>
#include "lz4.h"
#include <sys/stat.h>

/**
 * @file tw-setup.c
 * @brief tw_define_lps is defined here!
 */

#define VERIFY_MAPPING 0

static tw_pe *setup_pes(void);
unsigned int nkp_per_pe = 16;
static tw_clock init_start = 0;

static int32_t ross_rng_seed1;
static int32_t ross_rng_seed2;
static int32_t ross_rng_seed3;
static int32_t ross_rng_seed4;

static int32_t ross_core_rng_seed1;
static int32_t ross_core_rng_seed2;
static int32_t ross_core_rng_seed3;
static int32_t ross_core_rng_seed4;

static const tw_optdef kernel_options[] = {
    TWOPT_GROUP("ROSS Kernel"),
    TWOPT_UINT("synch", g_tw_synchronization_protocol, "Sychronization Protocol: SEQUENTIAL=1, CONSERVATIVE=2, OPTIMISTIC=3, OPTIMISTIC_DEBUG=4, OPTIMISTIC_REALTIME=5"),
    TWOPT_UINT("nkp", nkp_per_pe, "number of kernel processes (KPs) per pe"),
    TWOPT_DOUBLE("end", g_tw_ts_end, "simulation end timestamp"),
    TWOPT_UINT("batch", g_tw_mblock, "messages per scheduler block"),
    TWOPT_UINT("extramem", g_tw_events_per_pe_extra, "Number of extra events allocated per PE."),
    TWOPT_UINT("buddy-size", g_tw_buddy_alloc, "delta encoding buddy system allocation (2^X)"),
    TWOPT_UINT("lz4-knob", g_tw_lz4_knob, "LZ4 acceleration factor (higher = faster)"),
    TWOPT_DOUBLE("cons-lookahead", g_tw_lookahead, "Set g_tw_lookahead"),
    TWOPT_ULONGLONG("max-opt-lookahead", g_tw_max_opt_lookahead, "Optimistic simulation: maximum lookahead allowed in virtual clock time"),
#ifdef AVL_TREE
    TWOPT_UINT("avl-size", g_tw_avl_node_count, "AVL Tree contains 2^avl-size nodes"),
#endif
    TWOPT_UINT("rng-seed1",ross_rng_seed1, "ROSS RNG Seed 1 of 4"),
    TWOPT_UINT("rng-seed2",ross_rng_seed2, "ROSS RNG Seed 2 of 4"),
    TWOPT_UINT("rng-seed3",ross_rng_seed3, "ROSS RNG Seed 3 of 4"),
    TWOPT_UINT("rng-seed4",ross_rng_seed4, "ROSS RNG Seed 4 of 4"),
    TWOPT_UINT("core-rng-seed1",ross_core_rng_seed1, "ROSS Core RNG Seed 1 of 4"),
    TWOPT_UINT("core-rng-seed2",ross_core_rng_seed2, "ROSS Core RNG Seed 2 of 4"),
    TWOPT_UINT("core-rng-seed3",ross_core_rng_seed3, "ROSS Core RNG Seed 3 of 4"),
    TWOPT_UINT("core-rng-seed4",ross_core_rng_seed4, "ROSS Core RNG Seed 4 of 4"),
    TWOPT_END()
};

void tw_init(int *argc, char ***argv) {
    int i;
    init_start = tw_clock_read();
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
        printf("ROSS Version: %s\n\n", ROSS_VERSION);
    }
#endif

    tw_opt_add(kernel_options);
    tw_opt_add(tw_gvt_setup());
    tw_opt_add(tw_clock_setup());
    tw_opt_add(st_inst_opts());
#ifdef USE_DAMARIS
    tw_opt_add(st_damaris_opts());
#endif
    tw_opt_add(st_special_lp_opts());
#ifdef USE_RIO
    tw_opt_add(io_opts);
#endif

    // by now all options must be in
    tw_opt_parse(argc, argv);

#ifdef USE_DAMARIS
    st_damaris_ross_init();
    if (!g_st_ross_rank) // Damaris ranks only
        return;
    else
    {
#endif

    if(tw_ismaster())
    {
        struct stat buffer;
        int csv_check = stat("ross.csv", &buffer);

        if (NULL == (g_tw_csv = fopen("ross.csv", "a")))
            tw_error(TW_LOC, "Unable to open: ross.csv\n");

        if (csv_check == -1)
        {
            fprintf(g_tw_csv, "total_pe,total_kp,total_lp,end_ts,mapping,model_events,network_events,total_events,"
                    "runtime,events_processed,events_aborted,events_rb,pe_event_ties,eff,send_loc_remote,"
                    "percent_loc,send_net_remote,percent_net,rb_total,rb_primary,rb_secondary,fc_attempts,"
                    "num_gvt,net_events,event_rate,events_past_end,event_alloc,memory_alloc,memory_wasted,"
                    "remote_sends,remote_recvs,pe_struct,kp_struct,lp_struct,lp_model_struct,lp_rngs,"
                    "total_lp_size,event_struct,event_struct_model,init_time,pq_time,avl_time,lz4_time,"
                    "buddy_time,");
#ifdef USE_RIO
            fprintf(g_tw_csv, "rio_load,rio_init,");
#endif
            fprintf(g_tw_csv, "event_proc_time,event_cancel_time,event_abort_time,gvt_time,fc_time,"
                    "primary_rb_time,net_read_time,net_other_time,inst_comp_time,inst_write_time,total_time");
            if (g_st_use_analysis_lps)
                fprintf(g_tw_csv, ",model_nevent_proc,model_nevent_rb,model_net_events,model_eff,"
                        "alp_nevent_proc,alp_nevent_rb,alp_net_events,alp_eff");
            fprintf(g_tw_csv, "\n");
        }
    }

    if(ross_rng_seed1 && ross_rng_seed2 && ross_rng_seed3 && ross_rng_seed4)
    {
        if (!g_tw_mynode)
            printf("RNG Seeds: %d %d %d %d\n",ross_rng_seed1, ross_rng_seed2, ross_rng_seed3, ross_rng_seed4);
        g_tw_rng_seed = (int32_t*)calloc(4, sizeof(int32_t));
        g_tw_rng_seed[0] = ross_rng_seed1;
        g_tw_rng_seed[1] = ross_rng_seed2;
        g_tw_rng_seed[2] = ross_rng_seed3;
        g_tw_rng_seed[3] = ross_rng_seed4;
    }
    else if(ross_rng_seed1 || ross_rng_seed2 || ross_rng_seed3 || ross_rng_seed4)
        tw_error(TW_LOC, "If using --rng-seed#, all four seeds must be specified\n");

    if(ross_core_rng_seed1 && ross_core_rng_seed2 && ross_core_rng_seed3 && ross_core_rng_seed4)
    {
        if (!g_tw_mynode)
            printf("Core RNG Seeds: %d %d %d %d\n",ross_core_rng_seed1, ross_core_rng_seed2, ross_core_rng_seed3, ross_core_rng_seed4);
        g_tw_core_rng_seed = (int32_t*)calloc(4, sizeof(int32_t));
        g_tw_core_rng_seed[0] = ross_core_rng_seed1;
        g_tw_core_rng_seed[1] = ross_core_rng_seed2;
        g_tw_core_rng_seed[2] = ross_core_rng_seed3;
        g_tw_core_rng_seed[3] = ross_core_rng_seed4;
    }
    else if(ross_core_rng_seed1 || ross_core_rng_seed2 || ross_core_rng_seed3 || ross_core_rng_seed4)
        tw_error(TW_LOC, "If using --core-rng-seed#, all four seeds must be specified\n");

    //tw_opt_print();

    tw_net_start();
    tw_gvt_start();
#ifdef USE_DAMARIS
    } // end of if(g_st_ross_rank)
#endif
}

static void early_sanity_check(void) {
    if (!g_tw_nlp) {
        tw_error(TW_LOC, "need at least one LP");
    }
    if (!nkp_per_pe) {
        tw_printf(TW_LOC, "number of KPs (%u) must be >= 1, adjusting.", g_tw_nkp);
        g_tw_nkp = 1;
    }
}

/*
 * map: map LPs->KPs->PEs linearly
 */
void map_linear(void) {

    unsigned int nlp_per_kp;
    tw_lpid  lpid;
    tw_kpid  kpid;
    unsigned int j;

    // may end up wasting last KP, but guaranteed each KP has == nLPs
    nlp_per_kp = (int)ceil((double) g_tw_nlp / (double) g_tw_nkp);

    if(!nlp_per_kp) {
        tw_error(TW_LOC, "Not enough KPs defined: %d", g_tw_nkp);
    }

    g_tw_lp_offset = g_tw_mynode * g_tw_nlp;

#if VERIFY_MAPPING
    printf("NODE %d: nlp %lld, offset %lld\n", g_tw_mynode, g_tw_nlp, g_tw_lp_offset);
    printf("\tPE %d\n", g_tw_pe->id);
#endif

    for(kpid = 0, lpid = 0; kpid < nkp_per_pe; kpid++) {
        tw_kp_onpe(kpid, g_tw_pe);

#if VERIFY_MAPPING
        printf("\t\tKP %d", kpid);
#endif

        for(j = 0; j < nlp_per_kp && lpid < g_tw_nlp; j++, lpid++) {
            tw_lp_onpe(lpid, g_tw_pe, g_tw_lp_offset+lpid);
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

    if(!g_tw_lp[g_tw_nlp-1]) {
        tw_error(TW_LOC, "Not all LPs defined! (g_tw_nlp=%d)", g_tw_nlp);
    }

    if(g_tw_lp[g_tw_nlp-1]->gid != g_tw_lp_offset + g_tw_nlp - 1) {
        tw_error(TW_LOC, "LPs not sequentially enumerated!");
    }
}

void map_round_robin(void) {
    tw_pe   * pe = g_tw_pe; // ASSUMPTION: only 1 pe

    tw_kpid kpid;
    tw_lpid lpid;
    unsigned int  i;

    // ASSUMPTION: g_tw_nlp is the same on each rank
    int lp_stride = tw_nnodes();
    int lp_offset = g_tw_mynode;

    for(i = 0, lpid=lp_offset; i < g_tw_nlp; i++, lpid+=lp_stride) {
        kpid = i % g_tw_nkp;

        tw_lp_onpe(i, pe, lpid);
        tw_kp_onpe(kpid, pe);
        tw_lp_onkp(g_tw_lp[i], g_tw_kp[kpid]);

#if VERIFY_MAPPING
        printf("LP %4d KP %4d PE %4d\n", lpid, kpid, pe->id);
#endif
    }
}

/**
 * IMPORTANT: This function sets the value for g_tw_nlp which is a rather
 * important global variable.  It is also set in (very few) other places,
 * but mainly just here.
 */
void tw_define_lps(tw_lpid nlp, size_t msg_sz) {
    unsigned int  i;

    g_tw_nlp = nlp;

    /** We can't assume that every model uses equivalent values for g_tw_nlp across PEs
     *  so let's allreduce here to calculate how many total LPs there are in the sim.
     */
    if(MPI_Allreduce(&g_tw_nlp, &g_tw_total_lps, 1, MPI_UINT64_T, MPI_SUM, MPI_COMM_ROSS))
        tw_error(TW_LOC,"MPI_Allreduce in tw_define_lps() failed: Attempted to calculate total LPs");

    g_tw_msg_sz = msg_sz;

    early_sanity_check();

    /*
     * Construct the KP array.
     */
    if( g_tw_nkp == 1 && g_tw_synchronization_protocol != OPTIMISTIC_DEBUG ) { // if it is the default, then check with the overide param
        g_tw_nkp = nkp_per_pe;
    }
    // else assume the application overloaded and has BRAINS to set its own g_tw_nkp

    g_tw_kp = (tw_kp **) tw_calloc(TW_LOC, "KPs", sizeof(*g_tw_kp), g_tw_nkp);

    st_buffer_allocate();
    //if (g_tw_mapping == CUSTOM) // analysis LPs currently only supported for custom mapping
        specialized_lp_setup(); // for ROSS analysis LPs, important for setting g_st_analysis_nlp

    /*
     * Construct the LP array.
     */
    g_tw_lp = (tw_lp **) tw_calloc(TW_LOC, "LPs", sizeof(*g_tw_lp), g_tw_nlp + g_st_analysis_nlp);

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

    //if (g_tw_mapping == CUSTOM)
        specialized_lp_init_mapping();

    // init LP RNG stream(s)
    for(i = 0; i < g_tw_nlp + g_st_analysis_nlp; i++) {
        if(g_tw_rng_default == 1) {
            tw_rand_init_streams(g_tw_lp[i], g_tw_nRNG_per_lp, g_tw_nRNG_core_per_lp);
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
    for (i = 0; i < g_tw_nlp + g_st_analysis_nlp; i++) {
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

        if (!memcmp(lp->type, &null_type, sizeof(null_type))) {
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

    // init instrumentation
    st_inst_init();
#ifdef USE_DAMARIS
    if (g_st_damaris_enabled)
        st_damaris_inst_init();
#endif

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
    me->stats.s_init = tw_clock_read() - init_start;

    switch(g_tw_synchronization_protocol) {
        case SEQUENTIAL:            // case 1
            tw_scheduler_sequential(me);
            break;

        case CONSERVATIVE:         // case 2
            tw_scheduler_conservative(me);
            break;

        case OPTIMISTIC:          // case 3
            tw_scheduler_optimistic(me);
            break;

        case OPTIMISTIC_DEBUG:    // case 4
            tw_scheduler_optimistic_debug(me);
            break;

        case OPTIMISTIC_REALTIME: // case 5
            tw_scheduler_optimistic_realtime(me);
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
#ifdef USE_DAMARIS
    if(g_st_ross_rank)
    {
#endif
    if(tw_ismaster()) {
        fprintf(g_tw_csv, "\n");
        fclose(g_tw_csv);
    }
#ifdef USE_DAMARIS
    } // end if(g_st_ross_rank)
#endif

    tw_net_stop();
}

/**
 * By the time this function gets called, g_tw_delta_sz should be as large
 * as it will ever get.
 */
static void tw_delta_alloc(tw_pe *pe) {
    g_tw_delta_sz = LZ4_compressBound(g_tw_delta_sz);

    pe->delta_buffer[0] = (unsigned char *)tw_calloc(TW_LOC, "Delta buffers", g_tw_delta_sz, 1);
    pe->delta_buffer[1] = (unsigned char *)tw_calloc(TW_LOC, "Delta buffers", g_tw_delta_sz, 1);
    pe->delta_buffer[2] = (unsigned char *)tw_calloc(TW_LOC, "Delta buffers", g_tw_delta_sz, 1);
}

static tw_pe * setup_pes(void) {
    tw_pe   *pe = g_tw_pe;

    unsigned int num_events_per_pe;

    num_events_per_pe = 1 + g_tw_events_per_pe + g_tw_events_per_pe_extra;

    if (!pe) {
        tw_error(TW_LOC, "No PE configured on this node.");
    }

    // TODO need to make sure we don't break this stuff
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
#ifdef USE_RIO
    int i;
    tw_clock start = tw_clock_read();
    for (i = 0; i < g_io_events_buffered_per_rank; i++) {
        tw_eventq_push(&g_io_free_events, tw_eventq_pop(&g_tw_pe->free_q));
    }
    pe->stats.s_rio_load = (tw_clock_read() - start);
#endif

    if (g_tw_mynode == g_tw_masternode) {
        printf("\nROSS Core Configuration: \n");
        printf("\t%-50s %11u\n", "Total PEs", tw_nnodes());
        fprintf(g_tw_csv, "%u,", tw_nnodes());

        printf("\t%-50s [Nodes (%u) x KPs (%lu)] %lu\n", "Total KPs", tw_nnodes(), g_tw_nkp, (tw_nnodes() * g_tw_nkp));
        fprintf(g_tw_csv, "%lu,", (tw_nnodes() * g_tw_nkp));

        printf("\t%-50s %11llu\n", "Total LPs",
	       g_tw_total_lps);
        fprintf(g_tw_csv, "%llu,", g_tw_total_lps);

        printf("\t%-50s %11.2lf\n", "Simulation End Time", g_tw_ts_end);
        fprintf(g_tw_csv, "%.2lf,", g_tw_ts_end);


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

#ifdef USE_DAMARIS
    st_damaris_init_print();
#endif

        // moved these so ross.csv stays consistent
        fprintf(g_tw_csv, "%d,", num_events_per_pe);
        fprintf(g_tw_csv, "%d,", g_tw_gvt_threshold);
        fprintf(g_tw_csv, "%d,", g_tw_events_per_pe);
#ifndef ROSS_DO_NOT_PRINT
        printf("\nROSS Event Memory Allocation:\n");
        printf("\t%-50s %11d\n", "Model events", num_events_per_pe);

        printf("\t%-50s %11d\n", "Network events", g_tw_gvt_threshold);

        printf("\t%-50s %11d\n", "Total events", g_tw_events_per_pe);
        printf("\n");
#endif
    }
    return pe;
}

// This is the default lp type mapping function
// valid ONLY if there is one lp type
tw_lpid map_onetype (tw_lpid gid) {
    (void) gid;
    return 0;
}
