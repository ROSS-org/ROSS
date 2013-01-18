#include "qhold.h"

unsigned nlp_per_pe;
unsigned long int nLPs;
unsigned population;
unsigned long int remoteThreshold;
unsigned long int randomSeedVariation;

unsigned long int globalHash = 0;
unsigned long int globalEvents = 0;
unsigned long int globalEventsScheduled = 0;
unsigned long int globalTies = 0;
unsigned long int globalZeroDelays = 0;

// Reduced versions of the above counters
unsigned long int globalHashR = 0;
unsigned long int globalEventsR = 0;
unsigned long int globalEventsScheduledR = 0;
unsigned long int globalTiesR = 0;
unsigned long int globalZeroDelaysR = 0;

// Init function
// - called once for each LP
// ! LP can only send messages to itself during init !
void qhold_init(q_state *s, tw_lp *lp)
{
    int i;
    //unsigned long int seed = 0;
    
    /* initialize all of the PHOLD parameters */
	// initialize self;
    s->lastvtime = 0;
    s->stateValue = 0;
    s->zeroDelays = 0;
    s->ties = 0;
    s->events = 0;
    s->eventsScheduled = 0;
	// initialize nLPs;
    nLPs = tw_nnodes() * nlp_per_pe;
    // population is set by command-line parameters
	// population = popSize;
	// lookahead = lookAheadDelay;
    
	/* Seed initialization done mod 2**64, i.e. overflows ignored */
	// seed = ((unsigned long int)0xA174652BC0F983DE)) XOR (((unsigned long int)( lp->gid + 1000001 ))**10) xor randomSeeedVariation;
    // initialize stateValue = randomUnsignedLongInt(seed);
    s->stateValue = tw_rand_integer(lp->rng, 0, ULONG_MAX);
    globalHash += s->stateValue;
    
	/* create initial population of events, randomly distributed among the LPs */
    
	for (i = 0; i < population; i++) {
        tw_stime ts;
        tw_lpid dest;
        q_message *newData;
        tw_event *newEvent;
        int nextEventDelay;
        
		/* Calculate next event time */
		nextEventDelay = tw_rand_integer(lp->rng, 0, UINT_MAX);  	// 32-bit
        
		/* Calculate next event destination */
		//dest = randomUnsignedLongInt(seed) mod nLPs; 		// 64-bit; send to all destinations uniformly
        dest = lp->gid;
        
		/* Schedule initial event with lookAhead */
        ts = nextEventDelay + g_tw_lookahead;
        
        newEvent = tw_event_new(dest, ts, lp);
        newData = tw_event_data(newEvent);
        newData->msgValue = s->stateValue;
        
        tw_event_send(newEvent);
        s->eventsScheduled++;
        globalEventsScheduled++;
		//scheduleEvent(time + lookAhead + nextEventDelay, dest, stateValue); //; must be no overflow in the calculation of time
		
	}
}

// Forward event handler
void qhold_event(q_state *s, tw_bf *bf, q_message *msg, tw_lp *lp)
{
    tw_stime ts;
    tw_lpid dest;
    q_message *newData;
    tw_event *newEvent;
    int nextEventDelay;
    unsigned long int random;
    
    // Zero out all of our bitfields and counters
    *(int *)bf = 0;
    msg->RC.oldStateValue = 0;
    msg->RC.rngLoopCount = 0;
    msg->RC.lastvtime = 0;
    
    s->events++;
    globalEvents++;
    
    if (tw_now(lp) == s->lastvtime) {
        bf->c0 = 1;
        s->ties++;
        globalTies++;
    }
    
    // 1 rng
    random = tw_rand_integer(lp->rng, 0, ULONG_MAX);
    msg->RC.oldStateValue = s->stateValue;
    s->stateValue = (s->stateValue + msg->msgValue) ^ random;
    globalHash += s->stateValue;
    
    /* Calculate next event time */
    // 2 rng
	nextEventDelay = tw_rand_integer(lp->rng, 0, UINT_MAX);  // 32-bit
	if ( nextEventDelay == 0 ) {
        bf->c1 = 1;
        // if much more or less often that 1 in 2**32, we have a bad RNG
        s->zeroDelays++;
        globalZeroDelays++;
    }
        
    /* Calculate next event destination */
    // 3 rng
	if ( tw_rand_integer(lp->rng, 0, ULONG_MAX) < remoteThreshold ) {
        bf->c2 = 1;
		/* remote destination, uniformly distributed but excluding self */
        while (1) {
            msg->RC.rngLoopCount++;
            // 4 a, b, c... rng
            dest = tw_rand_integer(lp->rng, 0, nLPs);
            if (dest == lp->gid) {
                // We don't want to send to ourselves so loop through again
                continue;
            }
            else {
                break;
            }
        }
    }
    else {
        // Send to ourselves instead
        dest = lp->gid;
    }
    
    ts = nextEventDelay + g_tw_lookahead;
    
    newEvent = tw_event_new(dest, ts, lp);
    newData = tw_event_data(newEvent);
    newData->msgValue = s->stateValue;
    
    tw_event_send(newEvent);
    s->eventsScheduled++;
    globalEventsScheduled++;
    
    msg->RC.lastvtime = s->lastvtime;
    s->lastvtime = tw_now(lp);
}

// Reverse event handler
void qhold_event_reverse(q_state *s, tw_bf *bf, q_message *msg, tw_lp *lp)
{
    int i;
    
    s->lastvtime = msg->RC.lastvtime;
    
    globalEventsScheduled--;
    s->eventsScheduled--;
    
    if (bf->c2) {
        // We have at least one RNG call
        for (i = 0; i < msg->RC.rngLoopCount; i++) {
            // 4 a, b, c...
            tw_rand_reverse_unif(lp->rng);
        }
    }
    
    // 3
    tw_rand_reverse_unif(lp->rng);
    
    if (bf->c1) {
        globalZeroDelays--;
        s->zeroDelays--;
    }
    
    // 2
    tw_rand_reverse_unif(lp->rng);
    
    globalHash -= s->stateValue;
    s->stateValue = msg->RC.oldStateValue;
    
    // 1
    tw_rand_reverse_unif(lp->rng);
    
    if (bf->c0) {
        globalTies--;
        s->ties--;
    }
    
    globalEvents--;
    s->events--;
}

// Report any final statistics for this LP
void qhold_final(q_state *s, tw_lp *lp)
{
    if (g_tw_synchronization_protocol != SEQUENTIAL) {
        printf("globalEventsScheduled: %ld @ rank %d\n", globalEventsScheduled, g_tw_mynode);
        if (MPI_SUCCESS != MPI_Reduce(&globalHashR, &globalHash, 1, MPI_UNSIGNED_LONG, MPI_SUM, 0, MPI_COMM_WORLD))
            printf("FOO 1\n");
        if (MPI_SUCCESS != MPI_Reduce(&globalEventsR, &globalEvents, 1, MPI_UNSIGNED_LONG, MPI_SUM, 0, MPI_COMM_WORLD))
            printf("FOO 2\n");
        if (MPI_SUCCESS != MPI_Reduce(&globalEventsScheduledR, &globalEventsScheduled, 1, MPI_UNSIGNED_LONG, MPI_SUM, 0, MPI_COMM_WORLD))
            printf("FOO 3\n");
        if (MPI_SUCCESS != MPI_Reduce(&globalTiesR, &globalTies, 1, MPI_UNSIGNED_LONG, MPI_SUM, 0, MPI_COMM_WORLD))
            printf("FOO 4\n");
        if (MPI_SUCCESS != MPI_Reduce(&globalZeroDelaysR, &globalZeroDelays, 1, MPI_UNSIGNED_LONG, MPI_SUM, 0, MPI_COMM_WORLD))
            printf("FOO 5\n");
        printf("globalEventsScheduledR: %ld @ rank %d\n", globalEventsScheduledR, g_tw_mynode);
    }
    else {
        globalHashR = globalHash;
        globalEventsR = globalEvents;
        globalEventsScheduledR = globalEventsScheduled;
        globalTiesR = globalTies;
        globalZeroDelaysR = globalZeroDelays;
    }
    
    if (tw_ismaster()) {
        
        if (globalTiesR > 0) {
            printf("*** Warning: globalTies > 0\n");
        }
        if (globalZeroDelaysR > 0) {
            printf("*** Warning: globalZeroDelays > 0\n");
        }
        if (globalEventsR + nLPs * population != globalEventsScheduledR) {
            printf("*** ERROR: globalEvents does not correspond to globalEventsScheduled\n");
            printf("globalEventsR + nLPs * population (%ld, %ld, %d) = %ld\n", globalEventsR, nLPs, population,
                   globalEventsR + nLPs * population);
            printf("globalEventsScheduledR: %ld", globalEventsScheduledR);
        }
    }
}

// Given a gid, return the PE (or node) id
tw_peid qhold_map(tw_lpid gid)
{
    return (tw_peid) gid / g_tw_nlp;
}

// This defines the fuctions used by the LPs
// Multiple sets can be defined (for multiple LP types)
tw_lptype qhold_lps[] = {
    {
        (init_f) qhold_init,
        (event_f) qhold_event,
        (revent_f) qhold_event_reverse,
        (final_f) qhold_final,
        (map_f) qhold_map,
        sizeof(q_state)
    },
    { 0 },
};