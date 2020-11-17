#ifndef INC_network_mpi_h
#define INC_network_mpi_h

typedef long tw_node;

extern MPI_Comm MPI_COMM_ROSS;

/**
 * @brief Initalize the network library and parse options.
 *
 * argc and argv are pointers to the original command line; the
 * network library may edit these before the option parser sees
 * them allowing for network implementation specific argument
 * handling to occur.
 *
 * It's possible for a model to init MPI itself, as this
 * function will first check if MPI is already initialized before
 * attempting to call MPI_Init().
 *
 * This function also sets the global variables
 * g_tw_masternode and g_tw_mynode.
 *
 * @param[in] argc Pointer to command line arg count
 * @param[in] argv Pointer to command line args
 * @return tw_optdef array to be included in overall process
 * command line argument display and parsing; NULL may be returned
 * to indicate the implementation has no options it wants included.
 */
const tw_optdef *tw_net_init(int *argc, char ***argv);

/**
 * @brief Setup the MPI_COMM_ROSS communicator to use instead of MPI_COMM_WORLD.
 *
 * This function should be called before tw_net_init.
 * @param[in] comm Custom MPI communicator for setting MPI_COMM_ROSS
 */
void tw_comm_set(MPI_Comm comm);

/**
 * @brief Starts the network library after option parsing.
 *
 * Makes calls to initialize the PE (g_tw_pe), create the hash/AVL tree
 * (for optimistic modes), and queues for posted sends/recvs.
 * Also pre-posts MPI Irecvs operations.
 */
void tw_net_start(void);

/**
 * @brief Stops the network library after simulation end.
 *
 * Checks to see if custom communicator was used. If not, finalizes MPI.
 * Otherwise, the application is expected to finalize MPI itself.
 */
void tw_net_stop(void);

/** Aborts the entire simulation when a grave error is found. */
void tw_net_abort(void) NORETURN;

/**
 * @brief starts service_queues() to poll network
 *
 * @param[in] me pointer to the PE
 */
extern void tw_net_read(tw_pe *);

/**
 * @brief Adds the event to the outgoing queue of events to be sent,
 * polls for finished sends, and attempts to start sends from outq.
 *
 * @param[in] e remote event to be sent
 */
extern void tw_net_send(tw_event *);

/**
 * @brief Cancel the given remote event by either removing from the outq
 * or sending an antimessage, depending on the status of the original positive send.
 *
 * @param[in] e remote event to be canceled
 */
extern void tw_net_cancel(tw_event *);

/** Obtain the total number of PEs executing the simulation.
 *
 * @return number of ROSS PEs/MPI world size
 */
extern unsigned tw_nnodes(void);

/** Block until all nodes call the barrier. */
extern void tw_net_barrier(void);

/**
 * @brief Obtain the lowest timestamp inside the network buffers.
 *
 * @return minimum timestamp for this PE's network buffers
 */
extern tw_stime tw_net_minimum(void);

#ifdef USE_RAND_TIEBREAKER
/**
 * @brief Obtain the event signature for the lowest ordered event inside the network buffers.
 *
 * @return minimum event signature for this PE's network buffers
 */
extern tw_event_sig tw_net_minimum_sig(void);
#endif

/**
 * @brief Function to reduce all the statistics for output.
 * @attention Notice that the MPI_Reduce "count" parameter is greater than one.
 * We are reducing on multiple variables *simultaneously* so if you change
 * this function or the struct tw_statistics, you must update the other.
 **/
extern tw_statistics *tw_net_statistics(tw_pe *, tw_statistics *);

#endif
