#include <ross.h>

void
tw_kp_onpe(tw_kpid id, tw_pe * pe)
{
	if(id >= g_tw_nkp)
		tw_error(TW_LOC, "ID %d exceeded MAX KPs", id);

	if(g_tw_kp[id])
		tw_error(TW_LOC, "KP already allocated: %lld\n", id);

	g_tw_kp[id] = (tw_kp *) tw_calloc(TW_LOC, "Local KP", sizeof(tw_kp), 1);

	g_tw_kp[id]->id = id;
	g_tw_kp[id]->pe = pe;

#ifdef ROSS_QUEUE_kp_splay
	g_tw_kp[id]->pq = tw_eventpq_create();
#endif
}


#ifdef USE_RAND_TIEBREAKER
void
tw_kp_rollback_to_sig(tw_kp * kp, tw_event_sig to_sig)
{
    tw_event    *e;
    tw_clock pq_start;

    kp->s_rb_total++;
    kp->kp_stats->s_rb_total++;

    while (kp->pevent_q.size && tw_event_sig_compare(kp->pevent_q.head->sig, to_sig) >= 0)
    {
        e = tw_eventq_shift(&kp->pevent_q);

        // rollback first
        tw_event_rollback(e);

        // reset kp pointers
        if (kp->pevent_q.size == 0)
        {
            // kp->last_time = kp->pe->GVT;
            kp->last_sig = kp->pe->GVT_sig;
        } else
        {
            // kp->last_time = kp->pevent_q.head->recv_ts;
            kp->last_sig = kp->pevent_q.head->sig;
        }

        // place event back into priority queue
        pq_start = tw_clock_read();
        tw_pq_enqueue(kp->pe->pq, e);
        kp->pe->stats.s_pq += tw_clock_read() - pq_start;
    }
}
#else
void
tw_kp_rollback_to(tw_kp * kp, tw_stime to)
{
        tw_event       *e;
        tw_clock pq_start;

        kp->s_rb_total++;
        // instrumentation
        kp->kp_stats->s_rb_total++;

#if VERIFY_ROLLBACK
        printf("%d %d: rb_to %f, now = %f \n",
               kp->pe->id, kp->id, TW_STIME_DBL(to), TW_STIME_DBL(kp->last_time));
#endif

        while(kp->pevent_q.size && TW_STIME_CMP(kp->pevent_q.head->recv_ts, to) >= 0)
        {
                e = tw_eventq_shift(&kp->pevent_q);

                /*
                 * rollback first
                 */
                tw_event_rollback(e);

                /*
                 * reset kp pointers
                 */
                if (kp->pevent_q.size == 0)
                {
                        kp->last_time = kp->pe->GVT;
                } else
                {
                        kp->last_time = kp->pevent_q.head->recv_ts;
                }

                /*
                 * place event back into priority queue
                 */
                pq_start = tw_clock_read();
                tw_pq_enqueue(kp->pe->pq, e);
                kp->pe->stats.s_pq += tw_clock_read() - pq_start;
        }
}
#endif

void
tw_kp_rollback_event(tw_event * event)
{
    tw_event       *e = NULL;
    tw_kp          *kp;
    tw_pe          *pe;
    tw_clock pq_start;

    kp = event->dest_lp->kp;
    pe = kp->pe;

    kp->s_rb_total++;
	kp->s_rb_secondary++;
    // instrumentation
    kp->kp_stats->s_rb_total++;
	kp->kp_stats->s_rb_secondary++;

#if VERIFY_ROLLBACK
        printf("%d %d: rb_event: %f \n", pe->id, kp->id, event->recv_ts);

	if(!kp->pevent_q.size)
		tw_error(TW_LOC, "Attempting to rollback empty pevent_q!");
#endif

	e = tw_eventq_shift(&kp->pevent_q);
        while(e != event)
	{
#ifdef USE_RAND_TIEBREAKER
                kp->last_sig = kp->pevent_q.head->sig;
#else
                kp->last_time = kp->pevent_q.head->recv_ts;
#endif
		tw_event_rollback(e);
                pq_start = tw_clock_read();
                tw_pq_enqueue(pe->pq, e);
                pe->stats.s_pq += tw_clock_read() - pq_start;

		e = tw_eventq_shift(&kp->pevent_q);
        }

        tw_event_rollback(e);

#ifdef USE_RAND_TIEBREAKER
        if (0 == kp->pevent_q.size)
                kp->last_sig = kp->pe->GVT_sig;
        else
                kp->last_sig = kp->pevent_q.head->sig;
#else
        if (0 == kp->pevent_q.size)
                kp->last_time = kp->pe->GVT;
        else
                kp->last_time = kp->pevent_q.head->recv_ts;
#endif
}

#ifndef NUM_OUT_MESG
#define NUM_OUT_MESG 2000
#endif
static tw_out*
init_output_messages(tw_kp *kp)
{
    int i;

    tw_out *ret = (tw_out *) tw_calloc(TW_LOC, "tw_out", sizeof(struct tw_out), NUM_OUT_MESG);

    for (i = 0; i < NUM_OUT_MESG - 1; i++) {
        ret[i].next = &ret[i + 1];
        ret[i].owner = kp;
    }
    ret[i].next = NULL;
    ret[i].owner = kp;

    return ret;
}

void
tw_init_kps(tw_pe * me)
{
	tw_kpid i;
    int j;

	for (i = 0; i < g_tw_nkp; i++)
	{
		tw_kp *kp = tw_getkp(i);

		if (kp->pe != me)
			continue;

		kp->id = i;
        kp->s_nevent_processed = 0;
		kp->s_e_rbs = 0;
		kp->s_rb_total = 0;
		kp->s_rb_secondary = 0;
        if (g_tw_synchronization_protocol == OPTIMISTIC ||
	    g_tw_synchronization_protocol == OPTIMISTIC_DEBUG ||
	    g_tw_synchronization_protocol == OPTIMISTIC_REALTIME) {
            kp->output = init_output_messages(kp);
        }

        // instrumentation setup
        kp->kp_stats = (st_kp_stats*) tw_calloc(TW_LOC, "KP instrumentation", sizeof(st_kp_stats), 1);
        for (j = 0; j < 3; j++)
            kp->last_stats[j] = (st_kp_stats*) tw_calloc(TW_LOC, "KP instrumentation", sizeof(st_kp_stats), 1);
	}
}

tw_out *
tw_kp_grab_output_buffer(tw_kp *kp)
{
    if (kp->output) {
        tw_out *ret = kp->output;
        kp->output = kp->output->next;
        ret->next = 0;
        return ret;
    }

    return NULL;
}

void
tw_kp_put_back_output_buffer(tw_out *out)
{
    tw_kp *kp = out->owner;

    if (kp->output) {
        out->next = kp->output;
        kp->output = out;
    }
    else {
        kp->output = out;
        kp->output->next = NULL;
    }
}
