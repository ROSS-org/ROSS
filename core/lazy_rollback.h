#ifndef LAZY_ROLLBACK_H
#define LAZY_ROLLBACK_H

// send any events from the queue that are before this time
void lazy_rollback_catchup_to(tw_pe *pe, tw_stime timestamp);

// find and annihilate or send a match
int lazy_q_annihilate (tw_pe *pe, tw_event *e);

// add events to the lazy_q
void lazy_q_insert(tw_pe *pe, tw_event *e);

#endif
