#include <ross.h>

/**
 * @file damaris.c
 * @brief ROSS integration with Damaris data managment
 */

static int first_iteration = 1;
static double *pe_id, *lp_id;
static double *efficiency, *gvtd;
static long *net_events;
static tw_statistics last_stats = {0};
static tw_stat last_all_reduce_cnt = 0;


/**
 * @brief Set simulation parameters in Damaris
 */
void st_set_damaris_parameters(int num_lp)
{
    int err;
    int num_pe = tw_nnodes();

    if ((err = damaris_parameter_set("num_lp", &num_lp, sizeof(num_lp))) != 0)
        tw_error(TW_LOC, "Error with Damaris num_lp parameter!\n");

    if ((err = damaris_parameter_set("num_pe", &num_pe, sizeof(num_pe))) != 0)
        tw_error(TW_LOC, "Error with Damaris num_pe parameter!\n");
}

/**
 * @brief Sets up ROSS to use Damaris
 *
 * This must be called after MPI is initialized. Damaris splits the MPI 
 * communicator and we set MPI_COMM_ROSS to the subcommunicator returned by Damaris.
 * This sets the g_tw_is_ross_rank.  Need to make sure that Damaris ranks
 * (g_tw_is_ross_rank == 0) return to model after this point. 
 *
 */
void st_ross_damaris_init()
{
    int err;
    MPI_Comm ross_comm;

    // TODO don't hardcode Damaris config file
    if ((err = damaris_initialize("/home/rossc3/ROSS-Vis/damaris/test.xml", MPI_COMM_WORLD)) != 0)
        tw_error(TW_LOC, "Error initializing Damaris!\n");

    // g_tw_is_ross_rank > 0: ROSS MPI rank
    // g_tw_is_ross_rank == 0: Damaris MPI rank
    damaris_start(&g_tw_is_ross_rank);
    if(g_tw_is_ross_rank) 
    {
        //get ROSS sub comm
        damaris_client_comm_get(&ross_comm);  
        tw_comm_set(ross_comm);
    }

}

/**
 * @brief Shuts down Damaris and calls MPI_Finalize
 */
void st_ross_damaris_finalize()
{
    damaris_finalize();
    if (MPI_Finalize() != MPI_SUCCESS)
      tw_error(TW_LOC, "Failed to finalize MPI");
}

/**
 * @brief Expose GVT-based instrumentation data to Damaris
 */
void st_expose_gvt_data_damaris(tw_pe *me, tw_stime gvt, tw_statistics *s, tw_stat all_reduce_cnt)
{
    int err, i;
    /* NOTE: when writing more than one block with a PE, need to make sure number of domains is
     * appropriately updated in the XML file */
    
    if (first_iteration)
    {
        if (g_tw_mynode == g_tw_masternode)
        {
            pe_id = tw_calloc(TW_LOC, "damaris", sizeof(double), 4);
            for (i = 0; i < 4; i++)
            {
                pe_id[i] = (double) i;
            }
            damaris_write("ross/pes/pe_id", pe_id);

            lp_id = tw_calloc(TW_LOC, "damaris", sizeof(double), g_tw_nlp+1);
            for (i = 0; i < g_tw_nlp + 1; i++)
                lp_id[i] = (double) i;
                //lp_id[i] = (double)(g_tw_mynode * g_tw_nlp) + i;
            damaris_write("ross/lps/lp_id", lp_id);
        }

        net_events = tw_calloc(TW_LOC, "damaris", sizeof(long), g_tw_nlp);
        efficiency = tw_calloc(TW_LOC, "damaris", sizeof(double), g_tw_nlp);

        first_iteration = 0;
    }

    for (i = 0; i < g_tw_nlp; i++)
    {
        net_events[i] = (long)(s->s_net_events-last_stats.s_net_events);
        efficiency[i] = 100.0 * (1.0 - ((double) (s->s_e_rbs-last_stats.s_e_rbs)/(s->s_net_events-last_stats.s_net_events)));
    }

    err = damaris_write_block("ross/pes/net_events", g_tw_mynode, net_events);
    err = damaris_write_block("ross/pes/efficiency", g_tw_mynode, efficiency);
    //err = damaris_write("ross/gvt", gvtd);

    //*pe_id = g_tw_mynode;
    //*event_ties = s->s_pe_event_ties-last_stats.s_pe_event_ties; 
    //*events_aborted = s->s_nevent_abort-last_stats.s_nevent_abort;
    //*all_reduce_count = all_reduce_cnt-last_all_reduce_cnt;
    //*fc_attempts = s->s_fc_attempts-last_stats.s_fc_attempts;
    //*gvtd = gvt;
    //*rt = current_rt;

    memcpy(&last_stats, s, sizeof(tw_statistics));
    last_all_reduce_cnt = all_reduce_cnt;
}
