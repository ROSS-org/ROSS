#ifndef INC_ross_gvt_h
#define INC_ross_gvt_h

/*
 * Initialize the GVT library and parse options.
 */

/* setup the GVT library (config cmd line args, etc) */
extern const tw_optdef *tw_gvt_setup(void);

/* start the GVT library (init vars, etc) */
extern void tw_gvt_start(void);

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
extern void (*g_tw_gvt_hook) (tw_pe * pe);
/* Force call of arbitrary GVT function `g_tw_gvt_hook` even on Sequential mode.
 * This function should only be called before any event is processed, or after a GVT step, but
 * it's behaviour is undefined if called anywhere else, specially during an event call. */
void tw_trigger_gvt_hook_at(tw_stime time);
#ifdef USE_RAND_TIEBREAKER
void tw_trigger_gvt_hook_at_event_sig(tw_event_sig time);
#endif

// Holds one timestamp at which to trigger the arbitrary function
struct trigger_gvt_hook {
#ifdef USE_RAND_TIEBREAKER
    tw_event_sig sig_at;
#else
    tw_stime at;
#endif
    // the GVT hook can be enabled or disabled (meaning, it will be executed at
    // some GVT state or it won't). To handle all passible cases, we use an
    // enumeration.
    enum {
        GVT_HOOK_disabled = 0, // Hook won't be executed
        GVT_HOOK_enabled,  // Hook will be executed
        GVT_HOOK_triggered,  // Hook is or has been executed, but it hasn't yet been enabled to be triggered again (it should be changed to `GVT_HOOK_enabled` to be executed again)
    } active;
};

extern struct trigger_gvt_hook g_tw_trigger_gvt_hook;

#endif
