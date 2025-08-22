#ifndef INC_ross_gvt_h
#define INC_ross_gvt_h

#include "ross-types.h"
#include "tw-opts.h"
#include "stdbool.h"

/*
 * Initialize the GVT library and parse options.
 */

/* setup the GVT library (config cmd line args, etc) */
extern const tw_optdef *tw_gvt_setup(void);

/* start the GVT library (init vars, etc) */
extern void tw_gvt_start(void);
extern void tw_gvt_finish(void);

/*
 * GVT computation is broken into two stages:
 * stage 1: determine if GVT computation should be started
 * stage 2: compute GVT
 */
extern void tw_gvt_step1(tw_pe *);
extern void tw_gvt_step1_realtime(tw_pe *);
extern void tw_gvt_step2(tw_pe *);

/*
 * Provide a mechanism to force a GVT computation outside of the
 * GVT interval (optional)
 */
extern void tw_gvt_force_update(void);
extern void tw_gvt_force_update_realtime(void);

/* Set the PE GVT value */
extern int tw_gvt_set(tw_pe * pe, tw_stime LVT);

/* Returns true if GVT in progress, false otherwise */
static inline int  tw_gvt_inprogress(tw_pe * pe);

/* Statistics collection and printing function */
extern void tw_gvt_stats(FILE * F);

/* Function to be injected/executed at every GVT. The function receives the current PE and a boolean indicating if the simulation has no more events to process in the queue */
extern void (*g_tw_gvt_hook) (tw_pe * pe, bool past_end_time);
/* Trigger `g_tw_gvt_hook` at a specific time (it even works in Sequential
 * mode). This function should only be called before tw_run or inside
 * g_tw_gvt_hook. It's behaviour is undefined if called anywhere else,
 * specially during event processing. */
void tw_trigger_gvt_hook_at(tw_stime time);
#ifdef USE_RAND_TIEBREAKER
void tw_trigger_gvt_hook_at_event_sig(tw_event_sig time);
#endif
/* Trigger GVT hook every N GVT comptutations. Like `tw_trigger_gvt_hook_at`,
 * this function has to be called at before `tw_run` or inside the GVT hook
 * function. Calling this function will disable `tw_trigger_gvt_hook_at`
 */
void tw_trigger_gvt_hook_every(int num_gvt_calls);
/* Calling this function will enable the user to trigger the GVT hook
 * `tw_trigger_gvt_hook_now` as an event is processed. Like
 * `tw_trigger_gvt_hook_at`, this function has to be called at the start of the
 * simulation or inside g_tw_gvt_hook
 */
void tw_trigger_gvt_hook_when_model_calls(void);
/* It will pause the simulation and force a gvt hook call. Basically, this does
 * something similar to doing `tw_trigger_gvt_hook_at(now)` on all PEs with
 * `now * == tw_now(lp of the caller)`. Ofc, this function should be called only
 * when processing an event, thus it is complementary to
 * `tw_trigger_gvt_hook_at`. This function will only work if
 * `tw_trigger_gvt_hook_when_model_calls` have been called before on all PEs
 */
void tw_trigger_gvt_hook_now(tw_lp *);
// Reversing (removing) LP call to trigger GVT hook
void tw_trigger_gvt_hook_now_rev(tw_lp *);

enum GVT_HOOK_STATUS {
    GVT_HOOK_STATUS_disabled = 0, // The gvt hook will not be called
    GVT_HOOK_STATUS_timestamp,    // GVT called: At a specific time stamp (`tw_trigger_gvt_hook_at` has to be called every single time gvt hook is called)
    GVT_HOOK_STATUS_every_n_gvt,  // GVT called: Every N gvt operations (is set once, no need to call `tw_trigger_gvt_hook_every` again within gvt hook)
    GVT_HOOK_STATUS_model_call,   // GVT called: Whenever the user calls `tw_trigger_gvt_hook_now` (within the event being processed)
};

// Holds one timestamp at which to trigger the arbitrary function
struct gvt_hook_trigger {
    enum GVT_HOOK_STATUS status;
    // GVT_HOOK_TYPE_timestamp and GVT_HOOK_STATUS_model_call
    struct {
#ifdef USE_RAND_TIEBREAKER
    tw_event_sig sig_at;
#else
    tw_stime at;
#endif
    };
    // GVT_HOOK_TYPE_every_n_gvt
    struct {
        int starting_at;
        int nums;
    } every_n_gvt;
};

extern struct gvt_hook_trigger g_tw_gvt_hook_trigger;

#endif
