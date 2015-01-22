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
extern void tw_gvt_step2(tw_pe *);

/*
 * Provide a mechanism to force a GVT computation outside of the 
 * GVT interval (optional)
 */
extern void tw_gvt_force_update(tw_pe *);

/* Set the PE GVT value */
extern int tw_gvt_set(tw_pe * pe, tw_stime LVT);

/* Returns true if GVT in progress, false otherwise */
static inline int  tw_gvt_inprogress(tw_pe * pe);

/* Statistics collection and printing function */
extern void tw_gvt_stats(FILE * F);
#endif
