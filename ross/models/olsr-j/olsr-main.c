#include "olsr.h"

const tw_optdef olsr_opts[] = {
    TWOPT_GROUP("OLSR Model"),
    TWOPT_UINT("lp_per_pe", nlp_per_pe, "number of LPs per processor"),
    TWOPT_STIME("lookahead", g_tw_lookahead, "lookahead for the simulation"),
    TWOPT_CHAR("rwalk", g_olsr_mobility, "random walk [Y/N]"),
    TWOPT_END(),
};

// Done mainly so doxygen will catch and differentiate this main
// from other mains while allowing smooth compilation.
#define olsr_main main

int olsr_main(int argc, char *argv[])
{
    int i;
    char log[32];
    
    tw_opt_add(olsr_opts);
    tw_init(&argc, &argv);
    
#if DEBUG
    sprintf( log, "olsr-log.%d", g_tw_mynode );
    
    olsr_event_log = fopen( log, "w+");
    if( olsr_event_log == NULL )
        tw_error( TW_LOC, "Failed to Open OLSR Event Log file \n");
#endif
    
    g_tw_mapping = CUSTOM;
    g_tw_custom_initial_mapping = &olsr_custom_mapping;
    g_tw_custom_lp_global_to_local_map = &olsr_mapping_to_lp;
    
    // nlp_per_pe = OLSR_MAX_NEIGHBORS;// / tw_nnodes();
    //g_tw_lookahead = SA_INTERVAL;
    
    SA_range_start = nlp_per_pe;
    
    // Increase nlp_per_pe by nlp_per_pe / OMN
    nlp_per_pe += nlp_per_pe / OLSR_MAX_NEIGHBORS;
    
    g_tw_events_per_pe =  OLSR_MAX_NEIGHBORS / 2 * nlp_per_pe  + 65536;
    tw_define_lps(nlp_per_pe, sizeof(olsr_msg_data), 0);
    
    for(i = 0; i < OLSR_END_EVENT; i++)
        g_olsr_event_stats[i] = 0;
    
    for (i = 0; i < SA_range_start; i++) {
        tw_lp_settype(i, &olsr_lps[0]);
    }
    
    for (i = SA_range_start; i < nlp_per_pe; i++) {
        tw_lp_settype(i, &olsr_lps[1]);
    }
    
#if DEBUG
    printf("g_tw_nlp is %lu\n", g_tw_nlp);
#endif
    
    tw_run();
    
    if( g_tw_synchronization_protocol != 1 )
    {
        MPI_Reduce( g_olsr_event_stats, g_olsr_root_event_stats, OLSR_END_EVENT, MPI_LONG_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
    }
    else {
        for (i = 0; i < OLSR_END_EVENT; i++) {
            g_olsr_root_event_stats[i] = g_olsr_event_stats[i];
        }
    }
    
    if (tw_ismaster()) {
        for( i = 0; i < OLSR_END_EVENT; i++ )
            printf("OLSR Type %s Event Count = %llu \n", event_names[i], g_olsr_root_event_stats[i]);
        printf("Complete.\n");
    }
    
    tw_end();
    
    return 0;
}