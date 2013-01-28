#ifndef _QHOLD_H
#define _QHOLD_H

#include <ross.h>

// Message type for qhold
typedef struct {
    unsigned long int msgValue;
    
    // Sub-struct for reversible computation operations
    struct {
        unsigned long int oldStateValue;
        int rngLoopCount;
        tw_stime lastvtime;
    } RC;
    
} q_message;

// State information for qhold
typedef struct {
    tw_stime lastvtime;
    unsigned long int stateValue;
} q_state;

// Global variables used by both main and driver
extern unsigned long int nLPs;
extern tw_lptype qhold_lps[];
extern unsigned int nlp_per_pe;
extern unsigned long int remoteThreshold;

extern unsigned int population;
extern unsigned long int randomSeedVariation;

extern unsigned long int globalHash;
extern unsigned long int globalEvents;
extern unsigned long int globalEventsScheduled;
extern unsigned long int globalTies;
extern unsigned long int globalZeroDelays;

// Reduced versions of the above counters
extern unsigned long int globalHashR;
extern unsigned long int globalEventsR;
extern unsigned long int globalEventsScheduledR;
extern unsigned long int globalTiesR;
extern unsigned long int globalZeroDelaysR;

extern unsigned int remoteFractNumerator;
extern unsigned int remoteFractDenominator;

extern double percent_remote;
extern unsigned optimistic_memory;

#endif /* _QHOLD_H */
