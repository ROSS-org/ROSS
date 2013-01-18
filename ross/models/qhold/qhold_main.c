#include <ross.h>
#include "qhold.h"

// Add your command line opts
const tw_optdef qhold_opts[] = {
	TWOPT_GROUP("ROSS Model"),
    TWOPT_UINT("nlp_per_pe", nlp_per_pe, "Number of LPs per PE"),
    TWOPT_UINT("population", population, "number of initial messages per LP"),
    TWOPT_STIME("lookahead", g_tw_lookahead, "Lookahead for qhold"),
    TWOPT_ULONG("RSV", randomSeedVariation, "Random seed variation"),
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
    
	//Do some error checking?
	//Print out some settings?
    
	//Custom Mapping
	/*
     g_tw_mapping = CUSTOM;
     g_tw_custom_initial_mapping = &model_custom_mapping;
     g_tw_custom_lp_global_to_local_map = &model_mapping_to_lp;
     */
    
	//Useful ROSS variables and functions
	// tw_nnodes() : number of nodes/processors defined
	// g_tw_mynode : my node/processor id (mpi rank)
    
	//Useful ROSS variables (set from command line)
	// g_tw_events_per_pe
	// g_tw_lookahead
	// g_tw_nlp
	// g_tw_nkp
	// g_tw_synchronization_protocol
    
	//assume 1 lp per node
    
	//set up LPs within ROSS
	tw_define_lps(nlp_per_pe, sizeof(q_message), 0);
	// g_tw_nlp gets set by tw_define_lps
    
	for (i = 0; i < g_tw_nlp; i++) {
		tw_lp_settype(i, &qhold_lps[0]);
	}
    
	// Do some file I/O here? on a per-node (not per-LP) basis
    
	tw_run();
    
	tw_end();
    
    return 0;
}