#ifndef _QHOLD_H
#define _QHOLD_H

#include <ross.h>

typedef struct {
    unsigned long int msgValue;
    struct {
        unsigned long int oldStateValue;
        int rngLoopCount;
        tw_stime lastvtime;
    } RC;
} q_message;

typedef struct {
    tw_stime lastvtime;
    unsigned long int stateValue;
    unsigned long int zeroDelays;
    unsigned long int ties;
    unsigned long int events;
    unsigned long int eventsScheduled;
} q_state;

// Global variables used by both main and driver
extern tw_lptype qhold_lps[];
extern unsigned long remoteThreshold;
extern unsigned long nLPs;
extern unsigned int nlp_per_pe;
extern unsigned int population;
extern unsigned long int randomSeedVariation;

#endif /* _QHOLD_H */
