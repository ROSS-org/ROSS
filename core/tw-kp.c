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

void
tw_kp_rollback_to(tw_kp * kp, tw_stime to)
{
        tw_event       *e;

        kp->s_rb_total++;

#if VERIFY_ROLLBACK
        printf("%d %d: rb_to %f, now = %f \n",
		kp->pe->id, kp->id, to, kp->last_time);
#endif

        while(kp->pevent_q.size && kp->pevent_q.head->recv_ts >= to)
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
                tw_pq_enqueue(kp->pe->pq, e);
        }
}

void
tw_kp_rollback_event(tw_event * event)
{
        tw_event       *e = NULL;
        tw_kp          *kp;
        tw_pe          *pe;

        kp = event->dest_lp->kp;
        pe = kp->pe;

        kp->s_rb_total++;
	kp->s_rb_secondary++;

#if VERIFY_ROLLBACK
        printf("%d %d: rb_event: %f \n", pe->id, kp->id, event->recv_ts);

	if(!kp->pevent_q.size)
		tw_error(TW_LOC, "Attempting to rollback empty pevent_q!");
#endif

	e = tw_eventq_shift(&kp->pevent_q);
        while(e != event)
	{
                kp->last_time = kp->pevent_q.head->recv_ts;
		tw_event_rollback(e);
                tw_pq_enqueue(pe->pq, e);

		e = tw_eventq_shift(&kp->pevent_q);
        }

        tw_event_rollback(e);

        if (0 == kp->pevent_q.size)
                kp->last_time = kp->pe->GVT;
        else
                kp->last_time = kp->pevent_q.head->recv_ts;
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
	tw_kp *prev_kp = NULL;
	tw_kpid i;

	for (i = 0; i < g_tw_nkp; i++)
	{
		tw_kp *kp = tw_getkp(i);

		if (kp->pe != me)
			continue;

		kp->id = i;
		kp->s_e_rbs = 0;
		kp->s_rb_total = 0;
		kp->s_rb_secondary = 0;
		prev_kp = kp;
        if (g_tw_synchronization_protocol == OPTIMISTIC) {
            kp->output = init_output_messages(kp);
        }

#if ROSS_MEMORY
		kp->pmemory_q = tw_calloc(TW_LOC, "KP memory queues",
					sizeof(tw_memoryq), g_tw_memory_nqueues);
#endif
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

void
tw_kp_fossil_memoryq(tw_kp * kp, tw_fd fd)
{
#if ROSS_MEMORY
	tw_memoryq	*q;

	tw_memory      *b;
	tw_memory      *tail;

	tw_memory      *last;

	tw_stime	gvt = kp->pe->GVT;

	int             cnt;

	q = &kp->pmemory_q[fd];
	tail = q->tail;

	if(0 == q->size || tail->ts >= gvt)
		return;

	if(q->head->ts < gvt)
	{
		tw_memoryq_push_list(tw_pe_getqueue(kp->pe, fd), 
				     q->head, q->tail, q->size);

		q->head = q->tail = NULL;
		q->size = 0;

		return;
	}

	/*
	 * Start direct search.
	 */
	last = NULL;
	cnt = 0;

	b = q->head;
	while (b->ts >= gvt)
	{
		last = b;
		cnt++;

		b = b->next;
	}

	tw_memoryq_push_list(tw_pe_getqueue(kp->pe, fd), b, q->tail, q->size - cnt);

	/* fix what remains of our pmemory_q */
	q->tail = last;
	q->tail->next = NULL;
	q->size = cnt;

#if VERIFY_PE_FC_MEM
	printf("%d: FC %d buf from FD %d \n", kp->id, cnt, fd);
#endif
#endif
}

void
tw_kp_fossil_memory(tw_kp * kp)
{
	int	 i;

	for(i = 0; i < g_tw_memory_nqueues; i++)
		tw_kp_fossil_memoryq(kp, i);
}
