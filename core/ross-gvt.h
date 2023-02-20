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
extern void (*g_tw_gvt_arbitrary_fun) (tw_pe * pe, tw_event_sig gvt);
/* Force call of arbitrary GVT function `g_tw_gvt_arbitrary_fun` even when on Sequential mode.
 * This function should only be called before any event is process, or after a GVT step, but
 * it's behaviour is undefined if called anywhere else, specially during an event call. */
void tw_trigger_arbitrary_fun_at(tw_event_sig time);

// Holds one timestamp at which to trigger the arbitrary function
struct trigger_arbitrary_fun {
    tw_event_sig sig_at;
    //tw_stime at;
    enum {
        ARBITRARY_FUN_disabled = 0,
        ARBITRARY_FUN_enabled, // If the function is called, it will change its state to triggered
        ARBITRARY_FUN_triggered,  // If the flag is in the triggered state, it will be set to disabled once the function is called. If in the function call, the flag is changed to enable, it won't be reset
    } active;
};

extern struct trigger_arbitrary_fun g_tw_trigger_arbitrary_fun;

#endif
