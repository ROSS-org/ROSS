#include "qhold_fp.h"

unsigned nlp_per_pe = 16;
unsigned long int nLPs;
unsigned population = 16;
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

double percent_remote = 0.1;
unsigned optimistic_memory = 1024;

// numerator and denominator of the fraction of events that should be sent away
// from this LP; we represent the fraction this way to avoid floating point arithmetic
unsigned int remoteFractNumerator = 1;
unsigned int remoteFractDenominator = 1;

double mean = 1.0;

// Init function
// - called once for each LP
// ! LP can only send messages to itself during init !
void qhold_init(q_state *s, tw_lp *lp)
{
    unsigned i;
    //unsigned long int seed = 0;
    
    /* initialize all of the PHOLD parameters */
	// initialize self;
    s->lastvtime = 0;
    s->stateValue = 0;
	// initialize nLPs;
    nLPs = tw_nnodes() * nlp_per_pe;
    // population is set by command-line parameters
	// population = popSize;
	// lookahead = lookAheadDelay;
    
	/* Seed initialization done mod 2**64, i.e. overflows ignored */
	// seed = ((unsigned long int)0xA174652BC0F983DE)) XOR (((unsigned long int)( lp->gid + 1000001 ))**10) xor randomSeedVariation;
    // initialize stateValue = randomUnsignedLongInt(seed);
    s->stateValue = tw_rand_ulong(lp->rng, 0, ULONG_MAX-1);
    globalHash += s->stateValue;
    
	/* create initial population of events, randomly distributed among the LPs */
    
	for (i = 0; i < population; i++) {
        tw_stime ts;
        tw_lpid dest;
        q_message *newData;
        tw_event *newEvent;
        //unsigned nextEventDelay;
        tw_stime nextEventDelay;
        
		/* Calculate next event time */
		//nextEventDelay = tw_rand_ulong(lp->rng, 0, UINT_MAX-1);  	// 32-bit
        nextEventDelay = tw_rand_exponential(lp->rng, mean);
        
		/* Calculate next event destination */
		//dest = randomUnsignedLongInt(seed) mod nLPs; 		// 64-bit; send to all destinations uniformly
        dest = lp->gid;
        
		/* Schedule initial event with lookAhead */
        ts = nextEventDelay + g_tw_lookahead;
        
        newEvent = tw_event_new(dest, ts, lp);
        newData = tw_event_data(newEvent);
        newData->msgValue = s->stateValue;
        
        tw_event_send(newEvent);
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
    //unsigned nextEventDelay;
    tw_stime nextEventDelay;
    unsigned long int random;
    
    // Zero out all of our bitfields and counters
    *(unsigned *)bf = 0;
    msg->RC.oldStateValue = 0;
    msg->RC.rngLoopCount = 0;
    msg->RC.lastvtime = 0;
    
    globalEvents++;
    
    if (tw_now(lp) == s->lastvtime) {
        bf->c0 = 1;
        globalTies++;
    }
    
    // 1 rng
    random = tw_rand_ulong(lp->rng, 0, ULONG_MAX-1);
    //printf("random is %lu\n", random);
    //printf("ULONG_MAX is %lu\n", ULONG_MAX-1);
    msg->RC.oldStateValue = s->stateValue;
    s->stateValue = (s->stateValue + msg->msgValue) ^ random;
    //printf("s->stateValue is %lu\n", s->stateValue);
    globalHash += s->stateValue;
    
    /* Calculate next event time */
    // 2 rng
	//nextEventDelay = tw_rand_ulong(lp->rng, 0, UINT_MAX-1);  // 32-bit
    nextEventDelay = tw_rand_exponential(lp->rng, mean);
	if ( nextEventDelay == 0 ) {
        bf->c1 = 1;
        // if much more or less often that 1 in 2**32, we have a bad RNG
        globalZeroDelays++;
    }
        
    /* Calculate next event destination */
    // 3 rng
	if ( tw_rand_ulong(lp->rng, 0, ULONG_MAX-1) < remoteThreshold ) {
        bf->c2 = 1;
		/* remote destination, uniformly distributed but excluding self */
        while (1) {
            msg->RC.rngLoopCount++;
            // 4 a, b, c... rng
            dest = tw_rand_ulong(lp->rng, 0, nLPs-1);
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
    globalEventsScheduled++;
    
    msg->RC.lastvtime = s->lastvtime;
    s->lastvtime = tw_now(lp);
}

// Reverse event handler
void qhold_event_reverse(q_state *s, tw_bf *bf, q_message *msg, tw_lp *lp)
{
    unsigned i;
    
    s->lastvtime = msg->RC.lastvtime;
    
    globalEventsScheduled--;
    
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
    }
    
    // 2
    tw_rand_reverse_unif(lp->rng);
    
    globalHash -= s->stateValue;
    s->stateValue = msg->RC.oldStateValue;
    
    // 1
    tw_rand_reverse_unif(lp->rng);
    
    if (bf->c0) {
        globalTies--;
    }
    
    globalEvents--;
}

// Report any final statistics for this LP
void qhold_final(q_state *s, tw_lp *lp)
{
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
        (pre_run_f) NULL,
        (event_f) qhold_event,
        (revent_f) qhold_event_reverse,
        (final_f) qhold_final,
        (map_f) qhold_map,
        sizeof(q_state)
    },
    { 0 },
};
