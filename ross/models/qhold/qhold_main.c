#include <ross.h>
#include "qhold.h"

// Add your command line opts
const tw_optdef qhold_opts[] = {
	TWOPT_GROUP("ROSS Model"),
    TWOPT_UINT("nlp_per_pe", nlp_per_pe, "Number of LPs per PE"),
    TWOPT_UINT("population", population, "number of initial messages per LP"),
    TWOPT_STIME("lookahead", g_tw_lookahead, "Lookahead for qhold"),
    TWOPT_ULONG("RSV", randomSeedVariation, "Random seed variation"),
    TWOPT_STIME("remote", percent_remote, "desired remote event rate"),
    TWOPT_UINT("memory", optimistic_memory, "additional memory buffers"),
	TWOPT_END(),
};


// For doxygen
#define qhold_main main

int qhold_main(int argc, char *argv[])
{
    int i;
    
	tw_opt_add(qhold_opts);
	tw_init(&argc, &argv);
    
    if (nlp_per_pe < 1) {
        nlp_per_pe = 1;
    }
    
    if (population < 1) {
        population = 1;
    }
    
    remoteFractNumerator = percent_remote * UINT_MAX;
    remoteThreshold = ((unsigned long int)remoteFractNumerator * (1UL<<32) ) / (unsigned long int)remoteFractDenominator;
    
    g_tw_events_per_pe = (nlp_per_pe * population) +
    optimistic_memory;
    
	tw_define_lps(nlp_per_pe, sizeof(q_message), 0);
    
	for (i = 0; i < g_tw_nlp; i++) {
		tw_lp_settype(i, &qhold_lps[0]);
	}
    
	tw_run();
    
    if (g_tw_synchronization_protocol != SEQUENTIAL) {
        MPI_Reduce(&globalHash, &globalHashR, 1, MPI_UNSIGNED_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
        MPI_Reduce(&globalEvents, &globalEventsR, 1, MPI_UNSIGNED_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
        MPI_Reduce(&globalEventsScheduled, &globalEventsScheduledR, 1, MPI_UNSIGNED_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
        MPI_Reduce(&globalTies, &globalTiesR, 1, MPI_UNSIGNED_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
        MPI_Reduce(&globalZeroDelays, &globalZeroDelaysR, 1, MPI_UNSIGNED_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
    }
    else {
        globalHashR = globalHash;
        globalEventsR = globalEvents;
        globalEventsScheduledR = globalEventsScheduled;
        globalTiesR = globalTies;
        globalZeroDelaysR = globalZeroDelays;
    }
    
    if (tw_ismaster()) {
        
        /* Main output */
		//output globalHash, globalEvents, globalEventsScheduled, globalTies, globalZeroDelays;
        printf("\n\nStatistics:\n");
        printf("globalHash: %lu\n", globalHashR);
        printf("globalEvents: %lu\n", globalEventsR);
        printf("globalEventsScheduled: %lu\n", globalEventsScheduledR);
        printf("globalTies: %lu\n", globalTiesR);
        printf("globalZeroDelays: %lu\n", globalZeroDelaysR);
        
        if (globalTiesR > 0) {
            printf("*** Warning: globalTies > 0\n");
        }
        if (globalZeroDelaysR > 0) {
            printf("*** Warning: globalZeroDelays > 0\n");
        }
        if (globalEventsR + nLPs * population != globalEventsScheduledR) {
            printf("*** ERROR: globalEvents does not correspond to globalEventsScheduled\n");
            printf("globalEventsR + nLPs * population (%lu, %lu, %ud) = %ld\n", globalEventsR, nLPs, population,
                   globalEventsR + nLPs * population);
            printf("globalEventsScheduledR: %lu", globalEventsScheduledR);
        }
    }
    
	tw_end();
    
    return 0;
}