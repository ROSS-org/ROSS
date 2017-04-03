#include <ross.h>

/**
 * @file damaris.c
 * @brief ROSS integration with Damaris data managment
 */

static int first_iteration = 1;
static double *pe_id, *kp_id, *lp_id;
static double *efficiency, *gvtd;
static long *net_events;
static damaris_pe_counters *gvt_pe_counters;
static damaris_kp_counters gvt_kp_counters;
static damaris_lp_counters gvt_lp_counters;
static tw_statistics last_stats = {0};
static tw_stat last_all_reduce_cnt = 0;

static void gvt_pe_data(tw_pe *pe, tw_statistics *s);
static void gvt_kp_data();
static void gvt_lp_data();

/**
 * @brief Set simulation parameters in Damaris
 */
void st_set_damaris_parameters(int num_lp)
{
    int err;
    int num_pe = tw_nnodes();
    int num_kp = (int) g_tw_nkp;

    if ((err = damaris_parameter_set("num_lp", &num_lp, sizeof(num_lp))) != 0)
        tw_error(TW_LOC, "Error with Damaris num_lp parameter!\n");

    if ((err = damaris_parameter_set("num_pe", &num_pe, sizeof(num_pe))) != 0)
        tw_error(TW_LOC, "Error with Damaris num_pe parameter!\n");

    if ((err = damaris_parameter_set("num_kp", &num_kp, sizeof(num_kp))) != 0)
        tw_error(TW_LOC, "Error with Damaris num_kp parameter!\n");
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
void st_expose_gvt_data_damaris(tw_pe *me, tw_stime gvt, tw_statistics *s)
{
    int err, i;
    double *dummy;
    /* NOTE: when writing more than one block with a PE, need to make sure number of domains is
     * appropriately updated in the XML file */
    
    if (first_iteration)
    {
        if (g_tw_mynode == g_tw_masternode)
        {
            pe_id = tw_calloc(TW_LOC, "damaris", sizeof(double), tw_nnodes()+1);
            for (i = 0; i < tw_nnodes()+1; i++)
            {
                pe_id[i] = (double) i;
            }
            damaris_write("ross/pes/pe_id", pe_id);

            dummy = tw_calloc(TW_LOC, "damaris", sizeof(double), 2);
            dummy[0] = 0;
            dummy[1] = 1;
            damaris_write("ross/pes/dummy", dummy);

            lp_id = tw_calloc(TW_LOC, "damaris", sizeof(double), g_tw_nlp+1);
            for (i = 0; i < g_tw_nlp + 1; i++)
                lp_id[i] = (double) i;
                //lp_id[i] = (double)(g_tw_mynode * g_tw_nlp) + i;
            damaris_write("ross/lps/lp_id", lp_id);

            kp_id = tw_calloc(TW_LOC, "damaris", sizeof(double), g_tw_nkp+1);
            for (i = 0; i < g_tw_nkp + 1; i++)
                kp_id[i] = (double) i;
            damaris_write("ross/kps/kp_id", kp_id);
        }

        gvt_lp_counters.s_nevent_processed = tw_calloc(TW_LOC, "damaris", sizeof(int), g_tw_nlp);
        gvt_lp_counters.s_e_rbs = tw_calloc(TW_LOC, "damaris", sizeof(int), g_tw_nlp);
        gvt_lp_counters.s_net_events = tw_calloc(TW_LOC, "damaris", sizeof(int), g_tw_nlp);
        gvt_lp_counters.s_nsend_network = tw_calloc(TW_LOC, "damaris", sizeof(int), g_tw_nlp);
        gvt_lp_counters.s_nread_network = tw_calloc(TW_LOC, "damaris", sizeof(int), g_tw_nlp);
        gvt_lp_counters.s_nsend_net_remote = tw_calloc(TW_LOC, "damaris", sizeof(int), g_tw_nlp);

        gvt_kp_counters.s_rb_total = tw_calloc(TW_LOC, "damaris", sizeof(int), g_tw_nkp);
        gvt_kp_counters.s_rb_primary = tw_calloc(TW_LOC, "damaris", sizeof(int), g_tw_nkp);
        gvt_kp_counters.s_rb_secondary = tw_calloc(TW_LOC, "damaris", sizeof(int), g_tw_nkp);

        gvt_pe_counters = tw_calloc(TW_LOC, "damaris", sizeof(gvt_pe_counters), 1);

        first_iteration = 0;
    }

    gvt_pe_data(me, s);
    gvt_kp_data();
    gvt_lp_data();

    memcpy(&last_stats, s, sizeof(tw_statistics));
}

static void gvt_pe_data(tw_pe *pe, tw_statistics *s)
{
    int err;

    gvt_pe_counters->s_pq_qsize = (int) tw_pq_get_size(pe->pq);
    gvt_pe_counters->pe_event_ties = (int)(s->s_pe_event_ties-last_stats.s_pe_event_ties);
    gvt_pe_counters->fc_attempts = (int)(s->s_fc_attempts-last_stats.s_fc_attempts);
    gvt_pe_counters->efficiency = (float)100.0 * (1.0 - ((float) (s->s_e_rbs-last_stats.s_e_rbs)/(float) (s->s_net_events-last_stats.s_net_events)));

    err = damaris_write_block("ross/pes/pq_qsize", g_tw_mynode, &gvt_pe_counters->s_pq_qsize);
    err = damaris_write_block("ross/pes/event_ties", g_tw_mynode, &gvt_pe_counters->pe_event_ties);
    err = damaris_write_block("ross/pes/fc_attempts", g_tw_mynode, &gvt_pe_counters->fc_attempts);
    err = damaris_write_block("ross/pes/efficiency", g_tw_mynode, &gvt_pe_counters->efficiency);
}

static void gvt_kp_data()
{
    tw_kp *kp;
    int i, err;

    for (i = 0; i < g_tw_nkp; i++)
    {
        kp = tw_getkp(i);

        gvt_kp_counters.s_rb_total[i] = (int)(kp->s_rb_total - kp->last_s_rb_total_gvt);
        kp->last_s_rb_total_gvt = kp->s_rb_total;

        gvt_kp_counters.s_rb_secondary[i] = (int)(kp->s_rb_secondary - kp->last_s_rb_secondary_gvt);
        kp->last_s_rb_secondary_gvt = kp->s_rb_secondary;

        gvt_kp_counters.s_rb_primary[i] = gvt_kp_counters.s_rb_total[i] - gvt_kp_counters.s_rb_secondary[i];
    }

    err = damaris_write_block("ross/kps/total_rb", g_tw_mynode, gvt_kp_counters.s_rb_total);
    err = damaris_write_block("ross/kps/primary_rb", g_tw_mynode, gvt_kp_counters.s_rb_primary);
    err = damaris_write_block("ross/kps/secondary_rb", g_tw_mynode, gvt_kp_counters.s_rb_secondary);
}

static void gvt_lp_data()
{
    tw_lp *lp;
    int i, err;

    for (i = 0; i < g_tw_nlp; i++)
    {
        lp = tw_getlp(i);

        //net_events[i] = (long)(s->s_net_events-last_stats.s_net_events);
        //efficiency[i] = 100.0 * (1.0 - ((double) (s->s_e_rbs-last_stats.s_e_rbs)/(s->s_net_events-last_stats.s_net_events)));

        gvt_lp_counters.s_nevent_processed[i] = (int)(lp->event_counters->s_nevent_processed - lp->prev_event_counters_gvt->s_nevent_processed);
        lp->prev_event_counters_gvt->s_nevent_processed = lp->event_counters->s_nevent_processed;

        gvt_lp_counters.s_e_rbs[i] = (int)(lp->event_counters->s_e_rbs - lp->prev_event_counters_gvt->s_e_rbs);
        lp->prev_event_counters_gvt->s_e_rbs = lp->event_counters->s_e_rbs;

        gvt_lp_counters.s_net_events[i] = gvt_lp_counters.s_nevent_processed[i] - gvt_lp_counters.s_e_rbs[i];

        gvt_lp_counters.s_nsend_network[i] = (int)(lp->event_counters->s_nsend_network - lp->prev_event_counters_gvt->s_nsend_network);
        lp->prev_event_counters_gvt->s_nsend_network = lp->event_counters->s_nsend_network;

        gvt_lp_counters.s_nsend_network[i] = (int)(lp->event_counters->s_nread_network - lp->prev_event_counters_gvt->s_nread_network);
        lp->prev_event_counters_gvt->s_nread_network = lp->event_counters->s_nread_network;

        // next stat not guaranteed to always be non-decreasing
        // can't use tw_stat (long long) for negative number
        gvt_lp_counters.s_nsend_net_remote[i] = (int)lp->event_counters->s_nsend_net_remote - (int)lp->prev_event_counters_gvt->s_nsend_net_remote;
        lp->prev_event_counters_gvt->s_nsend_net_remote = lp->event_counters->s_nsend_net_remote;

    }

    err = damaris_write_block("ross/lps/events_processed", g_tw_mynode, gvt_lp_counters.s_nevent_processed);
    err = damaris_write_block("ross/lps/events_rb", g_tw_mynode, gvt_lp_counters.s_e_rbs);
    err = damaris_write_block("ross/lps/net_events", g_tw_mynode, gvt_lp_counters.s_net_events);
    err = damaris_write_block("ross/lps/network_sends", g_tw_mynode, gvt_lp_counters.s_nsend_network);
    err = damaris_write_block("ross/lps/network_recvs", g_tw_mynode, gvt_lp_counters.s_nread_network);
    err = damaris_write_block("ross/lps/remote_events", g_tw_mynode, gvt_lp_counters.s_nsend_net_remote);

    //err = damaris_write_block("ross/pes/net_events", g_tw_mynode, net_events);
    //err = damaris_write_block("ross/pes/efficiency", g_tw_mynode, efficiency);
}

