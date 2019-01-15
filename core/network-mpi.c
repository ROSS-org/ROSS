#include <ross.h>
#include <mpi.h>
#include "ross.h"

MPI_Comm MPI_COMM_ROSS = MPI_COMM_WORLD;
int custom_communicator = 0;

static long id_tmp;

struct act_q
{
    const char	 *name;

    tw_event	**event_list;
    MPI_Request	 *req_list;
    int		 *idx_list;
    MPI_Status	 *status_list;
    int        *free_idx_list;//add, que of free indices
    int        *overflow_anti;

#if ROSS_MEMORY
    char		**buffers;
#endif

    unsigned int cur;
    int front;//add, front of queue
    int coda;//add, back of queue but back is already a variable somewhere
    int size_of_fr_q;//add, size of queue array
    int num_in_fr_q;//add, number of elements in queue

// Deal with filling queue, then plateauing

};

int deal_with_cur(struct act_q *q)// try this
{
    if(q->cur < (q->size_of_fr_q-1))
    {
        q->cur++;
        return 1;
    }
    else
    {
        return 1;
    }
}


int fr_q_chq(struct act_q *q, int *frontOrCoda) //free index queue; check for modulating the front or back index of que
{
    if(*frontOrCoda != q->size_of_fr_q)//don't mess with queue
    {
        return 0;// return probably not necessary
    }
    else//mess with queue
    {
        *frontOrCoda = 0;
        return 0;
    }
}

void fr_q_aq(struct act_q *q, int ele) // free index queue; add to queue
{
    q->free_idx_list[q->coda] = ele;
    q->coda++;
//    q->num_in_fr_q++; //fixed in function
    fr_q_chq(q,&q->coda);//wraps the queue array around

}

int fr_q_dq(struct act_q *q) // free index queue; dequeue
{
    int rv =q->free_idx_list[q->front];
    q->front++;
    q->num_in_fr_q--;
    fr_q_chq(q,&q->front);// wraps the queue array around

    return rv;
}
#define EVENT_TAG 1

#if ROSS_MEMORY
#define EVENT_SIZE(e) TW_MEMORY_BUFFER_SIZE
#else
#define EVENT_SIZE(e) g_tw_event_msg_sz
#endif

static struct act_q posted_sends;
static struct act_q posted_recvs;
static tw_eventq outq;

static unsigned int read_buffer = 16;
static unsigned int send_buffer = 1024;
static int world_size = 1;

static const tw_optdef mpi_opts[] = {
        TWOPT_GROUP("ROSS MPI Kernel"),
        TWOPT_UINT(
                "read-buffer",
                read_buffer,
                "network read buffer size in # of events"),
        TWOPT_UINT(
                "send-buffer",
                send_buffer,
                "network send buffer size in # of events"),
        TWOPT_END()
};

// Forward declarations of functions used in MPI network message processing
static int recv_begin(tw_pe *me);
static void recv_finish(tw_pe *me, tw_event *e, char * buffer, struct act_q* q, int id);
static void late_recv_finish(tw_pe *me, tw_event *e, char * buffer, struct act_q* q, int id);
static int send_begin(tw_pe *me);
static void send_finish(tw_pe *me, tw_event *e, char * buffer, struct act_q* q, int id);

// Start of implmentation of network processing routines/functions
void tw_comm_set(MPI_Comm comm)
{
    MPI_COMM_ROSS = comm;
    custom_communicator = 1;
}

const tw_optdef *
tw_net_init(int *argc, char ***argv)
{
    int my_rank;

    int initialized;
    MPI_Initialized(&initialized);

    if (!initialized) {
        if (MPI_Init(argc, argv) != MPI_SUCCESS)
            tw_error(TW_LOC, "MPI_Init failed.");
    }
    if (MPI_Comm_rank(MPI_COMM_ROSS, &my_rank) != MPI_SUCCESS)
        tw_error(TW_LOC, "Cannot get MPI_Comm_rank(MPI_COMM_ROSS)");

    g_tw_masternode = 0;
    g_tw_mynode = my_rank;

    return mpi_opts;
}

static void
init_q(struct act_q *q, const char *name)
{
    unsigned int n;
#if ROSS_MEMORY
    unsigned int i;
#endif

    if(q == &posted_sends)
        n = send_buffer;
    else
        n = read_buffer;

    q->name = name;
    q->event_list = (tw_event **) tw_calloc(TW_LOC, name, sizeof(*q->event_list), n);
    q->req_list = (MPI_Request *) tw_calloc(TW_LOC, name, sizeof(*q->req_list), n);
    q->idx_list = (int *) tw_calloc(TW_LOC, name, sizeof(*q->idx_list), n);
    q->status_list = (MPI_Status *) tw_calloc(TW_LOC, name, sizeof(*q->status_list), n);
    q->free_idx_list = (int *) tw_calloc(TW_LOC, name, sizeof(*q->idx_list), n+1);// queue, n+1 is meant to prevent a full queue
    q->overflow_anti = (int *) tw_calloc(TW_LOC, name, sizeof(*q->idx_list), n+1);// queue, n+1 is meant to prevent a full queue
    q->front = 0;// front of queue
    q->coda  = 0;// end of queue
    q->size_of_fr_q=n+1;// for wraparound
    q->num_in_fr_q= 0;// number of elements in queue
    int i = 0;
    while(i<n) // initializes the queue
    {
        fr_q_aq( q , i) ;
        i++;
    }

    q->overflow_anti[0]=1;
    q->num_in_fr_q = n;


#if ROSS_MEMORY
    q->buffers = tw_calloc(TW_LOC, name, sizeof(*q->buffers), n);

  for(i = 0; i < n; i++)
    q->buffers[i] = tw_calloc(TW_LOC, "", TW_MEMORY_BUFFER_SIZE, 1);
#endif
}

tw_node * tw_net_onnode(tw_peid gid) {
    id_tmp = gid;
    return &id_tmp;
}

unsigned int
tw_nnodes(void)
{
    return world_size;
}

void
tw_net_start(void)
{
    if (MPI_Comm_size(MPI_COMM_ROSS, &world_size) != MPI_SUCCESS)
        tw_error(TW_LOC, "Cannot get MPI_Comm_size(MPI_COMM_ROSS)");

    if( g_tw_mynode == 0)
    {
        printf("tw_net_start: Found world size to be %d \n", world_size );
    }

    // Check after tw_nnodes is defined
    if(tw_nnodes() == 1 && g_tw_npe == 1) {
        // force the setting of SEQUENTIAL protocol
        if (g_tw_synchronization_protocol == NO_SYNCH) {
            g_tw_synchronization_protocol = SEQUENTIAL;
        } else if(g_tw_synchronization_protocol == CONSERVATIVE || g_tw_synchronization_protocol == OPTIMISTIC) {
            g_tw_synchronization_protocol = SEQUENTIAL;
            fprintf(stderr, "Warning: Defaulting to Sequential Simulation, not enought PEs defined.\n");
        }
    }

    tw_pe_create(1);
    tw_pe_init(0, g_tw_mynode);

    //If we're in (some variation of) optimistic mode, we need this hash
    if (g_tw_synchronization_protocol == OPTIMISTIC ||
        g_tw_synchronization_protocol == OPTIMISTIC_DEBUG ||
        g_tw_synchronization_protocol == OPTIMISTIC_REALTIME) {
        g_tw_pe[0]->hash_t = tw_hash_create();
    } else {
        g_tw_pe[0]->hash_t = NULL;
    }

    if (send_buffer < 1)
        tw_error(TW_LOC, "network send buffer must be >= 1");
    if (read_buffer < 1)
        tw_error(TW_LOC, "network read buffer must be >= 1");

    init_q(&posted_sends, "MPI send queue");
    init_q(&posted_recvs, "MPI recv queue");

    g_tw_net_device_size = read_buffer;

    // pre-post all the Irecv operations
    recv_begin( g_tw_pe[0] );
}

void
tw_net_abort(void)
{
    MPI_Abort(MPI_COMM_ROSS, 1);
    exit(1);
}

void
tw_net_stop(void)
{
    if (!custom_communicator) {
        if (MPI_Finalize() != MPI_SUCCESS)
            tw_error(TW_LOC, "Failed to finalize MPI");
    }
}

void
tw_net_barrier(tw_pe * pe)
{
    if (MPI_Barrier(MPI_COMM_ROSS) != MPI_SUCCESS)
        tw_error(TW_LOC, "Failed to wait for MPI_Barrier");
}

tw_stime
tw_net_minimum(tw_pe *me)
{
    tw_stime m = DBL_MAX;
    tw_event *e;
    int i;

    e = outq.head;
    while (e) {
        if (m > e->recv_ts)
            m = e->recv_ts;
        e = e->next;
    }

    for (i = 0; i < posted_sends.cur; i++) { //fix this line (?)
        e = posted_sends.event_list[i];
        if(e == NULL)
        {}
        else if(m > e->recv_ts)
            m = e->recv_ts;
        else
        {}
    }

    return m;
}

static int
test_q(
        struct act_q * q,
        tw_pe *me,
        void (*finish)(tw_pe *, tw_event *, char *, struct act_q*, int id))
{
    int ready, i, n, indicator;
    q->overflow_anti[0]=1; //
    indicator = 1;

#if ROSS_MEMORY
    char *tmp;
#endif


    if( q->num_in_fr_q == ((q->size_of_fr_q)-1) )
        return 0;

    if (MPI_Testsome(
            q->cur,
            q->req_list,
            &ready,
            q->idx_list,
            q->status_list) != MPI_SUCCESS) {
        tw_error(
                TW_LOC,
                "MPI_testsome failed with %u items in %s",
                q->cur,
                q->name);
    }


    if (1 > ready)
        return 0;

    for (i = 0; i < ready; i++) {

        tw_event *e;

        n = q->idx_list[i];
        e = q->event_list[n];
        fr_q_aq(q, n);//add n onto queue

#if ROSS_MEMORY
        finish(me, e, q->buffers[n], q, n);
#else
        finish(me, e, NULL, q, n);
#endif
        if (indicator != q->overflow_anti[0])
        {
            indicator= q->overflow_anti[0];
        }
        else
        {
            q->event_list[n] = NULL;
        }


    }

    i = 1;
    while (i < q->overflow_anti[0])
    {

        tw_event *e;
        n = q->overflow_anti[i];
        e = q->event_list[n];
        q->event_list[n] = NULL;

        late_recv_finish(me, e, NULL, q, n);
        //might need an augmented version for ROSS_MEMORY?

        i++;

    }

    q->num_in_fr_q+=ready;

    return 1;
}

static int
recv_begin(tw_pe *me)
{
    MPI_Status status;

    tw_event	*e = NULL;

    int flag = 0;
    int changed = 0;

    while (0 < posted_recvs.num_in_fr_q)//fix these lines
    {


        if(!(e = tw_event_grab(me)))
        {
            if(tw_gvt_inprogress(me))
                tw_error(TW_LOC, "Out of events in GVT! Consider increasing --extramem");
            return changed;
        }

        int id = fr_q_dq(&posted_recvs);

#if ROSS_MEMORY
        if( MPI_Irecv(posted_recvs.buffers[id],
		   EVENT_SIZE(e),
		   MPI_BYTE,
		   MPI_ANY_SOURCE,
		   EVENT_TAG,
		   MPI_COMM_ROSS,
		   &posted_recvs.req_list[id]) != MPI_SUCCESS)
#else
        if( MPI_Irecv(e,
                      (int)EVENT_SIZE(e),
                      MPI_BYTE,
                      MPI_ANY_SOURCE,
                      EVENT_TAG,
                      MPI_COMM_ROSS,
                      &posted_recvs.req_list[id]) != MPI_SUCCESS)
#endif
        {
            tw_event_free(me, e);
            return changed;
        }

        posted_recvs.event_list[id] = e;
        deal_with_cur(&posted_recvs);
        changed = 1;
    }

    return changed;
}


static void
late_recv_finish(tw_pe *me, tw_event *e, char * buffer, struct act_q * q, int id )
{
    tw_pe		*dest_pe;
    tw_clock       start;

#if ROSS_MEMORY
    tw_memory	*memory;
  tw_memory	*last;
  tw_fd		 mem_fd;

  size_t		 mem_size;

  unsigned	 position = 0;

  memcpy(e, buffer, g_tw_event_msg_sz);
  position += g_tw_event_msg_sz;
#endif

    dest_pe = e->dest_lp->pe;
    if(e->send_pe > tw_nnodes()-1)
        tw_error(TW_LOC, "bad sendpe_id: %d", e->send_pe);

    e->cancel_next = NULL;
    e->caused_by_me = NULL;
    e->cause_next = NULL;


    if(e->recv_ts < me->GVT)
        tw_error(TW_LOC, "%d: Received straggler from %d: %lf (%d)",
                 me->id,  e->send_pe, e->recv_ts, e->state.cancel_q);

    if(tw_gvt_inprogress(me))
        me->trans_msg_ts = ROSS_MIN(me->trans_msg_ts, e->recv_ts);

    // if cancel event, retrieve and flush
    // else, store in hash table

    tw_event *cancel = tw_hash_remove(me->hash_t, e, e->send_pe);
    g_tw_pe[0]->avl_tree_size++;

    // NOTE: it is possible to cancel the event we
        // are currently processing at this PE since this
        // MPI module lets me read cancel events during
        // event sends over the network.

    cancel->state.cancel_q = 1;
    cancel->state.remote = 0;
    cancel->cancel_next = dest_pe->cancel_q;
    dest_pe->cancel_q = cancel;
    tw_event_free(me, e);

    return;

}

static void
recv_finish(tw_pe *me, tw_event *e, char * buffer, struct act_q * q, int id )
{
    tw_pe		*dest_pe;
    tw_clock       start;

#if ROSS_MEMORY
    tw_memory	*memory;
  tw_memory	*last;
  tw_fd		 mem_fd;

  size_t		 mem_size;

  unsigned	 position = 0;

  memcpy(e, buffer, g_tw_event_msg_sz);
  position += g_tw_event_msg_sz;
#endif
    me->stats.s_nread_network++;
    me->s_nwhite_recv++;

    e->dest_lp = tw_getlocal_lp((tw_lpid) e->dest_lp);//->gid);// check here
    dest_pe = e->dest_lp->pe;// check here


    // instrumentation
    e->dest_lp->kp->kp_stats->s_nread_network++;
    e->dest_lp->lp_stats->s_nread_network++;

    if(e->send_pe > tw_nnodes()-1)
        tw_error(TW_LOC, "bad sendpe_id: %d", e->send_pe);


    if(e->recv_ts < me->GVT)
        tw_error(TW_LOC, "%d: Received straggler from %d: %lf (%d)",
                 me->id,  e->send_pe, e->recv_ts, e->state.cancel_q);

    if(tw_gvt_inprogress(me))
        me->trans_msg_ts = ROSS_MIN(me->trans_msg_ts, e->recv_ts);

    // if cancel event, retrieve and flush
    // else, store in hash table

    if(e->state.cancel_q)
    {

        tw_event *cancel = tw_hash_remove(me->hash_t, e, e->send_pe);

        // NOTE: it is possible to cancel the event we
        // are currently processing at this PE since this
        // MPI module lets me read cancel events during
        // event sends over the network.

        if(cancel!=NULL)
        {

            e->cancel_next = NULL;
            e->caused_by_me = NULL;
            e->cause_next = NULL;
            cancel->state.cancel_q = 1;
            cancel->state.remote = 0;


            cancel->cancel_next = dest_pe->cancel_q;
            dest_pe->cancel_q = cancel;
            tw_event_free(me, e);
        }
        else
        {
            q->overflow_anti[q->overflow_anti[0]] = id; //add id stuff later
            q->overflow_anti[0]++;


        }

        return;
    }

    e->cancel_next = NULL;
    e->caused_by_me = NULL;
    e->cause_next = NULL;

    if (g_tw_synchronization_protocol == OPTIMISTIC ||
        g_tw_synchronization_protocol == OPTIMISTIC_DEBUG ||
        g_tw_synchronization_protocol == OPTIMISTIC_REALTIME ) {
        tw_hash_insert(me->hash_t, e, e->send_pe);
        e->state.remote = 1;
    }

#if ROSS_MEMORY
    mem_size = (size_t) e->memory;
  mem_fd = (tw_fd) e->prev;

  last = NULL;
  while(mem_size)
    {
      memory = tw_memory_alloc(e->dest_lp, mem_fd);

      if(last)
	last->next = memory;
      else
	e->memory = memory;

      memcpy(memory, &buffer[position], mem_size);
      position += mem_size;

      memory->fd = mem_fd;
      memory->nrefs = 1;

      mem_size = (size_t) memory->next;
      mem_fd = memory->fd;

      last = memory;
    }
#endif

    /* NOTE: the final check in the if conditional below was added to make sure
     * that we do not execute the fast case unless the cancellation queue is
     * empty on the destination PE.  Otherwise we need to invoke the normal
     * scheduling routines to make sure that a forward event doesn't bypass a
     * cancellation event with an earlier timestamp.  This is helpful for
     * stateful models that produce incorrect results when presented with
     * duplicate messages with no rollback between them.
     */
    if(me == dest_pe && e->dest_lp->kp->last_time <= e->recv_ts && !dest_pe->cancel_q) {
        /* Fast case, we are sending to our own PE and
         * there is no rollback caused by this send.
         */
        start = tw_clock_read();
        tw_pq_enqueue(dest_pe->pq, e);
        dest_pe->stats.s_pq += tw_clock_read() - start;
        return;
    }

    if (me->node == dest_pe->node) {
        /* Slower, but still local send, so put into top
         * of dest_pe->event_q.
         */
        e->state.owner = TW_pe_event_q;
        tw_eventq_push(&dest_pe->event_q, e);
        return;
    }

    /* Never should happen; MPI should have gotten the
     * message to the correct node without needing us
     * to redirect the message there for it.  This is
     * probably a serious bug with the event headers
     * not being formatted right.
     */
    tw_error(
            TW_LOC,
            "Event recived by PE %u but meant for PE %u",
            me->id,
            dest_pe->id);
}

static int
send_begin(tw_pe *me)
{
    int changed = 0;

    while (0 < posted_sends.num_in_fr_q)//fixed these line (hopefully)
    {
        tw_event *e = tw_eventq_peek(&outq);//next event?
        tw_node	*dest_node = NULL;
        // posted_sends.cur; //fixed this line

#if ROSS_MEMORY
            tw_event *tmp_prev = NULL;

      tw_lp *tmp_lp = NULL;

      tw_memory *memory = NULL;
      tw_memory *m = NULL;

      char *buffer = NULL;

      size_t mem_size = 0;

      unsigned position = 0;
#endif

        if (!e)
            break;

        if(e == me->abort_event)
            tw_error(TW_LOC, "Sending abort event!");

        int id =  fr_q_dq(&posted_sends);// fixed, grabs from front of queue, moves front up one element
        dest_node = tw_net_onnode((*e->src_lp->type->map)
                                          ((tw_lpid) e->dest_lp));

        //if(!e->state.cancel_q)
        //e->event_id = (tw_eventid) ++me->seq_num;

        e->send_pe = (tw_peid) g_tw_mynode;
        e->send_lp = e->src_lp->gid;

#if ROSS_MEMORY
        // pack pointers
      tmp_prev = e->prev;
      tmp_lp = e->src_lp;

      // delete when working
      e->src_lp = NULL;

      memory = NULL;
      if(e->memory)
	{
	  memory = e->memory;
	  e->memory = (tw_memory *) tw_memory_getsize(me, memory->fd);
	  e->prev = (tw_event *) memory->fd;
	  mem_size = (size_t) e->memory;
	}

      buffer = posted_sends.buffers[id];
      memcpy(&buffer[position], e, g_tw_event_msg_sz);
      position += g_tw_event_msg_sz;

      // restore pointers
      e->prev = tmp_prev;
      e->src_lp = tmp_lp;

      m = NULL;
      while(memory)
	{
	  m = memory->next;

	  if(m)
	    {
	      memory->next = (tw_memory *)
		tw_memory_getsize(me, m->fd);
	      memory->fd = m->fd;
	    }

	  if(position + mem_size > TW_MEMORY_BUFFER_SIZE)
	    tw_error(TW_LOC, "Out of buffer space!");

	  memcpy(&buffer[position], memory, mem_size);
	  position += mem_size;

	  memory->nrefs--;
	  tw_memory_unshift(e->src_lp, memory, memory->fd);

	  if(NULL != (memory = m))
	    mem_size = tw_memory_getsize(me, memory->fd);
	}

      e->memory = NULL;

      if (MPI_Isend(buffer,
		    EVENT_SIZE(e),
		    MPI_BYTE,
		    *dest_node,
		    EVENT_TAG,
		    MPI_COMM_ROSS,
		    &posted_sends.req_list[id]) != MPI_SUCCESS) {
	return changed;
      }
#else
        if (MPI_Isend(e,
                      (int)EVENT_SIZE(e),
                      MPI_BYTE,
                      (int)*dest_node,
                      EVENT_TAG,
                      MPI_COMM_ROSS,
                      &posted_sends.req_list[id]) != MPI_SUCCESS) {
            return changed;
        }
#endif

        tw_eventq_pop(&outq);
        e->state.owner = e->state.cancel_q
                         ? TW_net_acancel
                         : TW_net_asend;

        posted_sends.event_list[id] = e;
        deal_with_cur(&posted_sends);

        // fixed in fr_q_dq //posted_sends.cur++;//fix this line
        me->s_nwhite_sent++;

        changed = 1;
    }
    return changed;
}

static void
send_finish(tw_pe *me, tw_event *e, char * buffer, struct act_q * q, int id)
{
    me->stats.s_nsend_network++;
    // instrumentation
    e->src_lp->kp->kp_stats->s_nsend_network++;
    e->src_lp->lp_stats->s_nsend_network++;

    if (e->state.owner == TW_net_asend) {
        if (e->state.cancel_asend) {
            /* Event was cancelled during transmission.  We must
             * send another message to pass the cancel flag to
             * the other node.
             */
            e->state.cancel_asend = 0;
            e->state.cancel_q = 1;
            tw_eventq_push(&outq, e);
        } else {
            /* Event finished transmission and was not cancelled.
             * Add to our sent event queue so we can retain the
             * event in case we need to cancel it later.  Note it
             * is currently in remote format and must be converted
             * back to local format for fossil collection.
             */
            e->state.owner = TW_pe_sevent_q;
            if( g_tw_synchronization_protocol == CONSERVATIVE )
                tw_event_free(me, e);
        }

        return;
    }

    if (e->state.owner == TW_net_acancel) {
        /* We just finished sending the cancellation message
         * for this event.  We need to free the buffer and
         * make it available for reuse.
         */
        tw_event_free(me, e);
        return;
    }

    /* Never should happen, not unless we somehow broke this
     * module's other functions related to sending an event.
     */

    tw_error(
            TW_LOC,
            "Don't know how to finish send of owner=%u, cancel_q=%d",
            e->state.owner,
            e->state.cancel_q);

}

static void
service_queues(tw_pe *me)
{
    int changed;
    do {
        changed  = test_q(&posted_recvs, me, recv_finish);
        changed |= test_q(&posted_sends, me, send_finish);
        changed |= recv_begin(me);
        changed |= send_begin(me);
    } while (changed);
}

/*
 * NOTE: Chris believes that this network layer is too aggressive at
 * reading events out of the network.. so we are modifying the algorithm
 * to only send events when tw_net_send it called, and only read events
 * when tw_net_read is called.
 */
void
tw_net_read(tw_pe *me)
{
    service_queues(me);
}

void
tw_net_send(tw_event *e)
{
    tw_pe * me = e->src_lp->pe;
    int changed = 0;

    e->state.remote = 0;
    e->state.owner = TW_net_outq;
    tw_eventq_unshift(&outq, e);

    do
    {
        changed = test_q(&posted_sends, me, send_finish);
        changed |= send_begin(me);
    } while (changed);
}

void
tw_net_cancel(tw_event *e)
{
    tw_pe *src_pe = e->src_lp->pe;

    switch (e->state.owner) {
        case TW_net_outq:
            /* Cancelled before we could transmit it.  Do not
             * transmit the event and instead just release the
             * buffer back into our own free list.
             */
            tw_eventq_delete_any(&outq, e);
            tw_event_free(src_pe, e);

            return;

            break;

        case TW_net_asend:
            /* Too late.  We've already let MPI start to send
             * this event over the network.  We can't pull it
             * back now without sending another message to do
             * the cancel.
             *
             * Setting the cancel_q flag will signal us to do
             * another message send once the current send of
             * this message is completed.
             */
            e->state.cancel_asend = 1;
            break;

        case TW_pe_sevent_q:
            /* Way late; the event was already sent and is in
             * our sent event queue.  Mark it as a cancel and
             * place it at the front of the outq.
             */
            e->state.cancel_q = 1;
            tw_eventq_unshift(&outq, e);
            break;

        default:
            /* Huh?  Where did you come from?  Why are we being
             * told about you?  We did not send you so we cannot
             * cancel you!
             */
            tw_error(
                    TW_LOC,
                    "Don't know how to cancel event owned by %u",
                    e->state.owner);
    }

    service_queues(src_pe);
}

/**
 * tw_net_statistics
 * @brief Function to output the statistics
 * @attention Notice that the MPI_Reduce "count" parameter is greater than one.
 * We are reducing on multiple variables *simultaneously* so if you change
 * this function or the struct tw_statistics, you must update the other.
 **/
tw_statistics	*
tw_net_statistics(tw_pe * me, tw_statistics * s)
{
    if(MPI_Reduce(&(s->s_max_run_time),
                  &me->stats.s_max_run_time,
                  1,
                  MPI_DOUBLE,
                  MPI_MAX,
                  (int)g_tw_masternode,
                  MPI_COMM_ROSS) != MPI_SUCCESS)
        tw_error(TW_LOC, "Unable to reduce statistics!");

    if(MPI_Reduce(&(s->s_net_events),
                  &me->stats.s_net_events,
                  17,
                  MPI_UNSIGNED_LONG_LONG,
                  MPI_SUM,
                  (int)g_tw_masternode,
                  MPI_COMM_ROSS) != MPI_SUCCESS)
        tw_error(TW_LOC, "Unable to reduce statistics!");

    if(MPI_Reduce(&s->s_min_detected_offset,
                  &me->stats.s_min_detected_offset,
                  1,
                  MPI_DOUBLE,
                  MPI_MIN,
                  (int)g_tw_masternode,
                  MPI_COMM_ROSS) != MPI_SUCCESS)
        tw_error(TW_LOC, "Unable to reduce statistics!");

    if(MPI_Reduce(&(s->s_total),
                  &me->stats.s_total,
                  16,
                  MPI_UNSIGNED_LONG_LONG,
                  MPI_MAX,
                  (int)g_tw_masternode,
                  MPI_COMM_ROSS) != MPI_SUCCESS)
        tw_error(TW_LOC, "Unable to reduce statistics!");

    if (MPI_Reduce(&s->s_events_past_end,
                   &me->stats.s_events_past_end,
                   3,
                   MPI_UNSIGNED_LONG_LONG,
                   MPI_SUM,
                   (int)g_tw_masternode,
                   MPI_COMM_ROSS) != MPI_SUCCESS)
        tw_error(TW_LOC, "Unable to reduce statistics!");

#ifdef USE_RIO
    if (MPI_Reduce(&s->s_rio_load,
            &me->stats.s_rio_load,
            1,
            MPI_UNSIGNED_LONG_LONG,
            MPI_MAX,
            (int)g_tw_masternode,
            MPI_COMM_ROSS) != MPI_SUCCESS)
        tw_error(TW_LOC, "Unable to reduce statistics!");
    if (MPI_Reduce(&s->s_rio_lp_init,
            &me->stats.s_rio_lp_init,
            1,
            MPI_UNSIGNED_LONG_LONG,
            MPI_MAX,
            (int)g_tw_masternode,
            MPI_COMM_ROSS) != MPI_SUCCESS)
        tw_error(TW_LOC, "Unable to reduce statistics!");
#endif

    return &me->stats;
}
