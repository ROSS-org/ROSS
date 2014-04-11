#ifndef INC_ross_network_h
#define INC_ross_network_h

/* Initalize the network library and parse options.
 *
 * argc and argv are pointers to the original command line; the
 * network library may edit these before the option parser sees
 * them allowing for network implementation specific argument
 * handling to occur.
 *
 * returned tw_optdef array will be included in the overall process
 * command line argument display and parsing; NULL may be returned
 * to indicate the implementation has no options it wants included.
 */
const tw_optdef *tw_net_init(int *argc, char ***argv);

/* Starts the network library after option parsing. */
void tw_net_start(void);

/* Stops the network library after simulation end. */
void tw_net_stop(void);

/* Aborts the entire simulation when a grave error is found. */
void tw_net_abort(void) NORETURN;

/* TODO: Old function definitions not yet replaced/redefined. */
extern void tw_net_read(tw_pe *);
extern void tw_net_send(tw_event *);
extern void tw_net_cancel(tw_event *);

/* Determine the identification of the node a pe is running on. */
tw_node * tw_net_onnode(tw_peid gid);

/* Obtain the total number of nodes executing the simulation. */
extern unsigned tw_nnodes(void);

/* Block until all nodes call the barrier. */
extern void tw_net_barrier(tw_pe * pe);

/* Obtain the lowest timestamp inside the network buffers. */
extern tw_stime tw_net_minimum(tw_pe *);

/* Send / receive tw_statistics objects.  */
extern tw_statistics *tw_net_statistics(tw_pe *, tw_statistics *);

/* Communicate LVT / GVT values to compute node network.  */
extern void tw_net_gvt_compute(tw_pe *, tw_stime *);

/* Provide a mechanism to send the PE LVT value to remote processors */
extern void tw_net_send_lvt(tw_pe *, tw_stime);

#endif
