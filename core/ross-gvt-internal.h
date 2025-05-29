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

/* Function to be injected/executed at every GVT */
//static void (* const g_tw_gvt_hook) (tw_pe * pe) = NULL;
extern void (*g_tw_gvt_hook) (tw_pe * pe);
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

enum GVT_HOOK_TYPE {
    GVT_HOOK_TYPE_timestamp = 0,
    GVT_HOOK_TYPE_every_n_gvt,  // Hook will be executed
};

// Holds one timestamp at which to trigger the arbitrary function
struct trigger_gvt_hook {
    enum GVT_HOOK_TYPE trigger_type;
    union {
        // GVT_HOOK_TYPE_timestamp
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
    bool enabled;
};

extern struct trigger_gvt_hook g_tw_trigger_gvt_hook;

#endif
