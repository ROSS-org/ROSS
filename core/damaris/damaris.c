#include <ross.h>

/**
 * @file damaris.c
 * @brief ROSS integration with Damaris data managment
 */

#define NUM_PE_VARS 4
#define NUM_KP_VARS 3
#define NUM_LP_VARS 6

static int first_iteration = 1;
static double *pe_id, *kp_id, *lp_id;
static tw_statistics last_stats = {0};

// set up variable path names based on setup in Damaris XML file
static const char *pe_var_names[NUM_PE_VARS] = {"pes/pq_qsize", "pes/event_ties", "pes/fc_attempts", "pes/efficiency"};
static const char *kp_var_names[NUM_KP_VARS] = {"kps/total_rb", "kps/secondary_rb", "kps/primary_rb"};
static const char *lp_var_names[NUM_LP_VARS] = {"lps/events_processed", "lps/events_rb", "lps/net_events", "lps/network_sends", "lps/network_recvs", "lps/remote_events"};
static const char damaris_gvt_path[] = {"ross/gvt_inst/"};
static const char damaris_rt_path[] = {"ross/rt_inst/"};
static int rt_block_counter = 0;
static int max_block_counter = 0;

static int *pe_vars;
static float *pe_efficiency;
static int *kp_vars[NUM_KP_VARS];
static int *lp_vars[NUM_LP_VARS];

static void pe_data(tw_pe *pe, tw_statistics *s, int inst_type);
static void kp_data(int inst_type);
static void lp_data(int inst_type);

/**
 * @brief Set simulation parameters in Damaris
 */
void st_set_damaris_parameters(int num_lp)
{
    int err;
    int num_pe = tw_nnodes();
    int num_kp = (int) g_tw_nkp;

    if ((err = damaris_parameter_set("num_lp", &num_lp, sizeof(num_lp))) != DAMARIS_OK)
        damaris_error(TW_LOC, err, "num_lp");

    if ((err = damaris_parameter_set("num_pe", &num_pe, sizeof(num_pe))) != DAMARIS_OK)
        damaris_error(TW_LOC, err, "num_pe");

    if ((err = damaris_parameter_set("num_kp", &num_kp, sizeof(num_kp))) != DAMARIS_OK)
        damaris_error(TW_LOC, err, "num_kp");
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
    if ((err = damaris_initialize("/home/rossc3/ROSS-Vis/damaris/test.xml", MPI_COMM_WORLD)) != DAMARIS_OK)
        damaris_error(TW_LOC, err, NULL);

    // g_tw_is_ross_rank > 0: ROSS MPI rank
    // g_tw_is_ross_rank == 0: Damaris MPI rank
    if ((err = damaris_start(&g_tw_is_ross_rank)) != DAMARIS_OK)
        damaris_error(TW_LOC, err, NULL);
    if(g_tw_is_ross_rank) 
    {
        //get ROSS sub comm
        if ((err = damaris_client_comm_get(&ross_comm)) != DAMARIS_OK) 
            damaris_error(TW_LOC, err, NULL);
        tw_comm_set(ross_comm);
    }

}

/**
 * @brief Shuts down Damaris and calls MPI_Finalize
 */
void st_ross_damaris_finalize()
{
    //if (g_st_real_time_samp)
    //    printf("Rank %ld: Max blocks counted for Real Time instrumentation is %d\n", g_tw_mynode, max_block_counter);
    int err;
    if ((err = damaris_finalize()) != DAMARIS_OK)
        damaris_error(TW_LOC, err, NULL);
    if (MPI_Finalize() != MPI_SUCCESS)
      tw_error(TW_LOC, "Failed to finalize MPI");
}

/**
 * @brief Expose GVT-based instrumentation data to Damaris
 */
void st_expose_data_damaris(tw_pe *me, tw_stime gvt, tw_statistics *s, int inst_type)
{
    int err, i;
    double *dummy;
    
    // write the coordinate info for meshes on only the first iteration.
    // each pe needs to only write the coordinates for which it will be 
    // writing variables.
    if (first_iteration)
    {
        // PE coordinates - just need pe_id and pe_id + 1 to allow for zonal coloring
        // on the mesh
        pe_id = tw_calloc(TW_LOC, "damaris", sizeof(double), 2);
            pe_id[0] = (double) g_tw_mynode;
            pe_id[1] = (double) g_tw_mynode+1;
        if ((err = damaris_write("ross/pe_id", pe_id)) != DAMARIS_OK)
            damaris_error(TW_LOC, err, "ross/pe_id");

        // just a dummy variable to allow for creating a mesh of PE stats
        dummy = tw_calloc(TW_LOC, "damaris", sizeof(double), 2);
        dummy[0] = 0;
        dummy[1] = 1;
        if ((err = damaris_write("ross/dummy", dummy)) != DAMARIS_OK)
            damaris_error(TW_LOC, err, "ross/dummy");

        // LP coordinates in local ids
        lp_id = tw_calloc(TW_LOC, "damaris", sizeof(double), g_tw_nlp+1);
        for (i = 0; i < g_tw_nlp + 1; i++)
            lp_id[i] = (double) i;
        if ((err = damaris_write("ross/lp_id", lp_id)) != DAMARIS_OK)
            damaris_error(TW_LOC, err, "ross/lp_id");

        // KP coordinates in local ids
        kp_id = tw_calloc(TW_LOC, "damaris", sizeof(double), g_tw_nkp+1);
        for (i = 0; i < g_tw_nkp + 1; i++)
            kp_id[i] = (double) i;
        if ((err = damaris_write("ross/kp_id", kp_id)) != DAMARIS_OK)
            damaris_error(TW_LOC, err, "ross/kp_id");

        // calloc the space for the variables we want to track
        pe_vars = tw_calloc(TW_LOC, "damaris", sizeof(int), NUM_PE_VARS-1);
        pe_efficiency = tw_calloc(TW_LOC, "damaris", sizeof(float), 1);

        for (i = 0; i < NUM_KP_VARS; i++)
            kp_vars[i] = tw_calloc(TW_LOC, "damaris", sizeof(int), g_tw_nkp);

        for (i = 0; i < NUM_LP_VARS; i++)
            lp_vars[i] = tw_calloc(TW_LOC, "damaris", sizeof(int), g_tw_nlp);

        first_iteration = 0;
    }

    // collect data for each entity
    pe_data(me, s, inst_type);
    kp_data(inst_type);
    lp_data(inst_type);
    if (inst_type == RT_COL)
        rt_block_counter++;

    memcpy(&last_stats, s, sizeof(tw_statistics));

    //if ((err = damaris_signal("test")) != DAMARIS_OK)
    //    damaris_error(TW_LOC, err, "test");
}

/**
 * @brief expose PE data to Damaris
 */
static void pe_data(tw_pe *pe, tw_statistics *s, int inst_type)
{
    int err, i, block = 0;
    char var_name[2048];
    
    pe_vars[0] = (int) tw_pq_get_size(pe->pq);
    pe_vars[1] = (int)(s->s_pe_event_ties-last_stats.s_pe_event_ties);
    pe_vars[2] = (int)(s->s_fc_attempts-last_stats.s_fc_attempts);
    *pe_efficiency = (float)100.0 * (1.0 - ((float) (s->s_e_rbs-last_stats.s_e_rbs)/(float) (s->s_net_events-last_stats.s_net_events)));

    for (i = 0; i < NUM_PE_VARS; i++)
    {
        if (inst_type == GVT_COL)
            snprintf(var_name, sizeof(damaris_gvt_path) + strlen(pe_var_names[i]),"%s%s", damaris_gvt_path, pe_var_names[i]);
        else if (inst_type == RT_COL)
        {
            snprintf(var_name, sizeof(damaris_rt_path) + strlen(pe_var_names[i]),"%s%s", damaris_rt_path, pe_var_names[i]);
            block = rt_block_counter;
        }
        if (i < NUM_PE_VARS-1)
        {
            if ((err = damaris_write_block(var_name, block, &pe_vars[i])) != DAMARIS_OK)
                damaris_error(TW_LOC, err, var_name);
        }
        else
        {
            if ((err = damaris_write_block(var_name, block, pe_efficiency)) != DAMARIS_OK)
                damaris_error(TW_LOC, err, var_name);
        }
    }
}

/**
 * @brief expose KP data to Damaris
 */
static void kp_data(int inst_type)
{
    tw_kp *kp;
    int i, err, block = 0;
    char var_name[2048];

    for (i = 0; i < g_tw_nkp; i++)
    {
        kp = tw_getkp(i);

        kp_vars[0][i] = (int)(kp->s_rb_total - kp->last_s_rb_total_gvt);
        kp_vars[1][i] = (int)(kp->s_rb_secondary - kp->last_s_rb_secondary_gvt);
        kp_vars[2][i] = kp_vars[0][i] - kp_vars[1][i];

        kp->last_s_rb_total_gvt = kp->s_rb_total;
        kp->last_s_rb_secondary_gvt = kp->s_rb_secondary;
    }

    for (i = 0; i < NUM_KP_VARS; i++)
    {
        if (inst_type == GVT_COL)
            snprintf(var_name, sizeof(damaris_gvt_path) + strlen(kp_var_names[i]),"%s%s", damaris_gvt_path, kp_var_names[i]);
        else if (inst_type == RT_COL)
        {
            snprintf(var_name, sizeof(damaris_rt_path) + strlen(kp_var_names[i]),"%s%s", damaris_rt_path, kp_var_names[i]);
            block = rt_block_counter;
        }
        if ((err = damaris_write_block(var_name, block, kp_vars[i])) != DAMARIS_OK)
            damaris_error(TW_LOC, err, var_name);
    }
}

/**
 * @brief expose LP data to Damaris
 */
static void lp_data(int inst_type)
{
    tw_lp *lp;
    int i, err, block = 0;
    char var_name[2048];

    for (i = 0; i < g_tw_nlp; i++)
    {
        lp = tw_getlp(i);

        lp_vars[0][i] = (int)(lp->event_counters->s_nevent_processed - lp->prev_event_counters_gvt->s_nevent_processed);
        lp_vars[1][i] = (int)(lp->event_counters->s_e_rbs - lp->prev_event_counters_gvt->s_e_rbs);
        lp_vars[2][i] = lp_vars[0][i] - lp_vars[1][i];
        lp_vars[3][i] = (int)(lp->event_counters->s_nsend_network - lp->prev_event_counters_gvt->s_nsend_network);
        lp_vars[4][i] = (int)(lp->event_counters->s_nread_network - lp->prev_event_counters_gvt->s_nread_network);
        lp_vars[5][i] = (int)lp->event_counters->s_nsend_net_remote - (int)lp->prev_event_counters_gvt->s_nsend_net_remote;

        lp->prev_event_counters_gvt->s_nevent_processed = lp->event_counters->s_nevent_processed;
        lp->prev_event_counters_gvt->s_e_rbs = lp->event_counters->s_e_rbs;
        lp->prev_event_counters_gvt->s_nsend_network = lp->event_counters->s_nsend_network;
        lp->prev_event_counters_gvt->s_nread_network = lp->event_counters->s_nread_network;
        lp->prev_event_counters_gvt->s_nsend_net_remote = lp->event_counters->s_nsend_net_remote;
    }

    for (i = 0; i < NUM_LP_VARS; i++)
    {
        if (inst_type == GVT_COL)
            snprintf(var_name, sizeof(damaris_gvt_path) + strlen(lp_var_names[i]),"%s%s", damaris_gvt_path, lp_var_names[i]);
        else if (inst_type == RT_COL)
        {
            snprintf(var_name, sizeof(damaris_rt_path) + strlen(lp_var_names[i]),"%s%s", damaris_rt_path, lp_var_names[i]);
            block = rt_block_counter;
        }
        if ((err = damaris_write(var_name, lp_vars[i])) != DAMARIS_OK)
            damaris_error(TW_LOC, err, var_name);
    }
}

/** 
 * @brief
 */
void reset_block_counter()
{
    if (rt_block_counter > max_block_counter)
        max_block_counter = rt_block_counter;
    rt_block_counter = 0;
}

/**
 * @brief Make Damaris error checking easier.
 *
 * Some errors will stop simulation, but some will only warn and keep going. 
 */
void damaris_error(const char *file, int line, int err, char *variable)
{
    switch(err)
    {
        case DAMARIS_ALLOCATION_ERROR:
            tw_warning(file, line, "Damaris allocation error for variable %s\n", variable);
            break;
        case DAMARIS_ALREADY_INITIALIZED:
            tw_warning(file, line, "Damaris was already initialized\n");
            break;
        case DAMARIS_BIND_ERROR:
            tw_error(file, line, "Damaris bind error for Damaris event:  %s\n", variable);
            break;
        case DAMARIS_BLOCK_NOT_FOUND:
            tw_error(file, line, "Damaris not able to find block for variable:  %s\n", variable);
            break;
        case DAMARIS_CORE_IS_SERVER:
            tw_error(file, line, "This node is not a client.\n");
            break;
        case DAMARIS_DATASPACE_ERROR:
            tw_error(file, line, "Damaris dataspace error for variable:  %s\n", variable);
            break;
        case DAMARIS_INIT_ERROR:
            tw_error(file, line, "Error calling damaris_init().\n");
            break;
        case DAMARIS_FINALIZE_ERROR:
            tw_error(file, line, "Error calling damaris_finalize().\n");
            break;
        case DAMARIS_INVALID_BLOCK:
            tw_error(file, line, "Damaris invalid block for variable:  %s\n", variable);
            break;
        case DAMARIS_NOT_INITIALIZED:
            tw_warning(file, line, "Damaris has not been initialized\n");
            break;
        case DAMARIS_UNDEFINED_VARIABLE:
            tw_error(file, line, "Damaris variable %s is undefined\n", variable);
            break;
        case DAMARIS_UNDEFINED_ACTION:
            tw_error(file, line, "Damaris action %s is undefined\n", variable);
            break;
        case DAMARIS_UNDEFINED_PARAMETER:
            tw_error(file, line, "Damaris parameter %s is undefined\n", variable);
            break;
        default:
            tw_error(file, line, "Damaris error unknown. %s \n", variable);
    }
}

