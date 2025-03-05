#ifndef INC_check_revent_state_check
#define INC_check_revent_state_check

#include <stdbool.h>

typedef void (*save_checkpoint_state_f) (void * into, void const * from);
typedef void (*clean_checkpoint_state_f) (void * state);
typedef bool (*check_states_f) (void * current_state, void const * before_state);
typedef void (*print_lpstate_f) (FILE *, char const * prefix, void * state);
typedef void (*print_checkpoint_state_f) (FILE *, char const * prefix, void * state);
typedef void (*print_event_f) (FILE *, char const * prefix, void * state);

/*
 * Interface to implement in order to get tighter control over the
 * SEQUENTIAL_ROLLBACK_CHECK synchronization option.
 *
 * SEQUENTIAL_ROLLBACK_CHECK allows to run check if all reversible
 * computations have been properly implemented. By default, there is
 * no need to use this interface in order to run SEQUENTIAL_ROLLBACK_CHECK.
 *
 * If save_lp is not implemented, then the LP struct will be save into the
 * LP checkpoint.
 *
 * Often, it is best to start by implementing print_lp.
 *
 * Only the `lptype` is mandatory, everything else can be NULL or zero.
 */
typedef struct crv_checkpointer {
    tw_lptype * lptype;
    size_t sz_storage; // Size of the LP checkpoint to save (can be different from actual LP size)
    save_checkpoint_state_f save_lp; // Copies LP state into LP checkpoint
    clean_checkpoint_state_f clean_lp; // Cleans LP checkpoint (do not call free). Only to be used if there is something to clean as a result of saving checkpoint earlier
    check_states_f check_lps; // Checks if the current LP state is the same as the checkpoint state
    print_lpstate_f print_lp; // Prints the state of the LP in a human readable way
    print_checkpoint_state_f print_checkpoint; // Prints the state of the LP checkpoint in a human readable way
    print_event_f print_event; // Prints the contents of the message the LP processes
} crv_checkpointer;


// Adding LP checkpointer
void crv_add_custom_state_checkpoint(crv_checkpointer *);


/*
 * Internal struct, not to be modified by model developer.
 */
typedef struct crv_lpstate_checkpoint_internal {
    void * state;
    tw_rng_stream rng;
    tw_rng_stream core_rng;
} crv_lpstate_checkpoint_internal;

size_t crv_init_checkpoints(void);
void crv_copy_lpstate(crv_lpstate_checkpoint_internal * into, tw_lp const * clp);
void crv_clean_lpstate(crv_lpstate_checkpoint_internal * state, tw_lp const * clp);
void crv_check_lpstates(
         tw_lp * clp,
         tw_event * cev,
         crv_lpstate_checkpoint_internal const * before_state,
         char const * before_msg,
         char const * after_msg
);

#endif
