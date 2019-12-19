#include <ross.h>

/**
 * \brief Reset the event bitfield prior to entering the event handler
 *  post-reverse - reset the bitfield so that a potential re-running of the
 *  event is presented with a consistent bitfield state
 *  NOTE: the size checks are to better support the experimental reverse
 *  computation compiler, which can use a larger bitfield.
 *  Courtesy of John P Jenkins
 */
static inline void reset_bitfields(tw_event *revent)
{
    memset(&revent->cv, 0, sizeof(revent->cv));
}

/*
 * Get all events out of my event queue and spin them out into
 * the priority queue so they can be processed in time stamp
 * order.
 */
static void tw_sched_event_q(tw_pe * me) {
    tw_clock     start;
    tw_kp       *dest_kp;
    tw_event    *cev;
    tw_event    *nev;

    while (me->event_q.size) {
        cev = tw_eventq_pop_list(&me->event_q);

        for (; cev; cev = nev) {
            nev = cev->next;

            if(!cev->state.owner || cev->state.owner == TW_pe_free_q) {
                tw_error(TW_LOC, "no owner!");
            }
            if (cev->state.cancel_q) {
                cev->state.owner = TW_pe_anti_msg;
                cev->next = cev->prev = NULL;
                continue;
            }

            switch (cev->state.owner) {
                case TW_pe_event_q:
                    dest_kp = cev->dest_lp->kp;

                    if (TW_STIME_CMP(dest_kp->last_time, cev->recv_ts) > 0) {
                        /* cev is a straggler message which has arrived
                        * after we processed events occuring after it.
                        * We need to jump back to before cev's timestamp.
                        */
                        start = tw_clock_read();
                        tw_kp_rollback_to(dest_kp, cev->recv_ts);
                        me->stats.s_rollback += tw_clock_read() - start;
                        if (g_st_ev_trace == RB_TRACE)
                           st_collect_event_data(cev, (double)start / g_tw_clock_rate);
                    }
                    start = tw_clock_read();
                    tw_pq_enqueue(me->pq, cev);
                    me->stats.s_pq += tw_clock_read() - start;
                    break;

                default:
                    tw_error(TW_LOC, "Event in event_q, but owner %d not recognized", cev->state.owner);
            }
        }
    }
}

/*
 * OPT: need to link events into canq in reverse order so
 *      that when we rollback the 1st event, we should not
 *  need to do any further rollbacks.
 */
static void tw_sched_cancel_q(tw_pe * me) {
    tw_clock     start=0, pq_start;
    tw_event    *cev;
    tw_event    *nev;

    start = tw_clock_read();
    while (me->cancel_q) {
        cev = me->cancel_q;
        me->cancel_q = NULL;

        for (; cev; cev = nev) {
            nev = cev->cancel_next;

            if (!cev->state.cancel_q) {
                tw_error(TW_LOC, "No cancel_q bit on event in cancel_q");
            }

            if(!cev->state.owner || cev->state.owner == TW_pe_free_q) {
                tw_error(TW_LOC, "Cancelled event, no owner!");
            }

            switch (cev->state.owner) {
                case TW_pe_event_q:
                    /* This event hasn't been added to our pq yet and we
                    * have not officially received it yet either.  We'll
                    * do the actual free of this event when we receive it
                    * as we spin out the event_q chain.
                    */
                    tw_eventq_delete_any(&me->event_q, cev);

                    tw_event_free(me, cev);
                    break;

                case TW_pe_anti_msg:
                    tw_event_free(me, cev);
                    break;

                case TW_pe_pq:
                    /* Event was not cancelled directly from the event_q
                    * because the cancel message came after we popped it
                    * out of that queue but before we could process it.
                    */
                    pq_start = tw_clock_read();
                    tw_pq_delete_any(me->pq, cev);
                    me->stats.s_pq += tw_clock_read() - pq_start;
                    tw_event_free(me, cev);
                    break;

                case TW_kp_pevent_q:
                    /* The event was already processed.
                    * SECONDARY ROLLBACK
                    */
                    tw_kp_rollback_event(cev);
                    tw_event_free(me, cev);
                    break;

                default:
                    tw_error(TW_LOC, "Event in cancel_q, but owner %d not recognized", cev->state.owner);
            }
        }
    }

    me->stats.s_cancel_q += tw_clock_read() - start;
}

static void tw_sched_batch(tw_pe * me) {
    /* Number of consecutive times we gave up because there were no free event buffers. */
    static int no_free_event_buffers = 0;
    static int warned_no_free_event_buffers = 0;
    const int max_alloc_fail_count = 20;

    tw_clock     start, pq_start;
    unsigned int     msg_i;

    /* Process g_tw_mblock events, or until the PQ is empty
    * (whichever comes first).
    */
    for (msg_i = g_tw_mblock; msg_i; msg_i--) {
        tw_event *cev;
        tw_lp *clp;
        tw_kp *ckp;

        /* OUT OF FREE EVENT BUFFERS.  BAD.
        * Go do fossil collect immediately.
        */
        if (me->free_q.size <= g_tw_gvt_threshold) {
            /* Suggested by Adam Crume */
            if (++no_free_event_buffers > 10) {
                if (!warned_no_free_event_buffers) {
                    fprintf(stderr, "WARNING: No free event buffers.  Try increasing memory via the --extramem option.\n");
                    warned_no_free_event_buffers = 1;
                }
                if (no_free_event_buffers >= max_alloc_fail_count) {
                    tw_error(TW_LOC, "Event allocation failed %d consecutive times.  Exiting.", max_alloc_fail_count);
                }
            }
            tw_gvt_force_update();
            break;
        }
        no_free_event_buffers = 0;

        start = tw_clock_read();
        if (!(cev = tw_pq_dequeue(me->pq))) {
            break;
        }
        me->stats.s_pq += tw_clock_read() - start;
        if(TW_STIME_CMP(cev->recv_ts, tw_pq_minimum(me->pq)) == 0) {
            me->stats.s_pe_event_ties++;
        }

        clp = cev->dest_lp;

	ckp = clp->kp;
	me->cur_event = cev;
	ckp->last_time = cev->recv_ts;

	/* Save state if no reverse computation is available */
	if (!clp->type->revent) {
	  tw_error(TW_LOC, "Reverse Computation must be implemented!");
	}

	start = tw_clock_read();
	reset_bitfields(cev);

	// if NOT A SUSPENDED LP THEN FORWARD PROC EVENTS
	if( !(clp->suspend_flag) )
	  {
        // state-save and update the LP's critical path
        unsigned int prev_cp = clp->critical_path;
        clp->critical_path = ROSS_MAX(clp->critical_path, cev->critical_path)+1;
	    (*clp->type->event)(clp->cur_state, &cev->cv,
				tw_event_data(cev), clp);
        if (g_st_ev_trace == FULL_TRACE)
            st_collect_event_data(cev, (double)tw_clock_read() / g_tw_clock_rate);
        cev->critical_path = prev_cp;
	  }
	ckp->s_nevent_processed++;
    // instrumentation
    ckp->kp_stats->s_nevent_processed++;
    clp->lp_stats->s_nevent_processed++;
	me->stats.s_event_process += tw_clock_read() - start;

	/* We ran out of events while processing this event.  We
	 * cannot continue without doing GVT and fossil collect.
	 */

	if (me->cev_abort)
	  {
	    start = tw_clock_read();
	    me->stats.s_nevent_abort++;
        // instrumentation
        ckp->kp_stats->s_nevent_abort++;
        clp->lp_stats->s_nevent_abort++;
	    me->cev_abort = 0;

	    tw_event_rollback(cev);
        pq_start = tw_clock_read();
	    tw_pq_enqueue(me->pq, cev);
        me->stats.s_pq += tw_clock_read() - pq_start;

	    cev = tw_eventq_peek(&ckp->pevent_q);
	    ckp->last_time = cev ? cev->recv_ts : me->GVT;

	    tw_gvt_force_update();

	    me->stats.s_event_abort += tw_clock_read() - start;


	    break;
	  } // END ABORT CHECK

	/* Thread current event into processed queue of kp */
        cev->state.owner = TW_kp_pevent_q;
        tw_eventq_unshift(&ckp->pevent_q, cev);

        if(g_st_rt_sampling &&
                tw_clock_read() - g_st_rt_samp_start_cycles > g_st_rt_interval)
        {
            tw_clock current_rt = tw_clock_read();
#ifdef USE_DAMARIS
            if (g_st_engine_stats == RT_STATS || g_st_engine_stats == ALL_STATS)
            {
                if (g_st_damaris_enabled)
                    st_damaris_expose_data(me, me->GVT, RT_COL);
                else
                    st_collect_engine_data(me, RT_COL);
            }
#else
            if (g_st_engine_stats == RT_STATS || g_st_engine_stats == ALL_STATS)
                st_collect_engine_data(me, RT_COL);
            if (g_st_model_stats == RT_STATS || g_st_model_stats == ALL_STATS)
                st_collect_model_data(me, ((double) current_rt) / g_tw_clock_rate, RT_STATS);
#endif
            g_st_rt_samp_start_cycles = tw_clock_read();
        }

    }
}

static void tw_sched_batch_realtime(tw_pe * me) {
    /* Number of consecutive times we gave up because there were no free event buffers. */
    static int no_free_event_buffers = 0;
    static int warned_no_free_event_buffers = 0;
    const int max_alloc_fail_count = 20;

    tw_clock     start, pq_start;
    unsigned int     msg_i;

    /* Process g_tw_mblock events, or until the PQ is empty
    * (whichever comes first).
    */
    for (msg_i = g_tw_mblock; msg_i; msg_i--) {
        tw_event *cev;
        tw_lp *clp;
        tw_kp *ckp;

        /* OUT OF FREE EVENT BUFFERS.  BAD.
        * Go do fossil collect immediately.
        */
        if (me->free_q.size <= g_tw_gvt_threshold) {
            /* Suggested by Adam Crume */
            if (++no_free_event_buffers > 10) {
                if (!warned_no_free_event_buffers) {
                    fprintf(stderr, "WARNING: No free event buffers.  Try increasing memory via the --extramem option.\n");
                    warned_no_free_event_buffers = 1;
                }
                if (no_free_event_buffers >= max_alloc_fail_count) {
                    tw_error(TW_LOC, "Event allocation failed %d consecutive times.  Exiting.", max_alloc_fail_count);
                }
            }
            tw_gvt_force_update_realtime();
            break;
        }
        no_free_event_buffers = 0;

        start = tw_clock_read();
        if (!(cev = tw_pq_dequeue(me->pq))) {
          break; // leave the batch function
        }
        me->stats.s_pq += tw_clock_read() - start;
        if(TW_STIME_CMP(cev->recv_ts, tw_pq_minimum(me->pq)) == 0) {
            me->stats.s_pe_event_ties++;
        }

        clp = cev->dest_lp;

	ckp = clp->kp;
	me->cur_event = cev;
	ckp->last_time = cev->recv_ts;

	/* Save state if no reverse computation is available */
	if (!clp->type->revent) {
	  tw_error(TW_LOC, "Reverse Computation must be implemented!");
	}

	start = tw_clock_read();

	reset_bitfields(cev);

	// if NOT A SUSPENDED LP THEN FORWARD PROC EVENTS
	if( !(clp->suspend_flag) )
	  {
        // state-save and update the LP's critical path
        unsigned int prev_cp = clp->critical_path;
        clp->critical_path = ROSS_MAX(clp->critical_path, cev->critical_path)+1;
	    (*clp->type->event)(clp->cur_state, &cev->cv,
				tw_event_data(cev), clp);
        if (g_st_ev_trace == FULL_TRACE)
            st_collect_event_data(cev, (double)tw_clock_read() / g_tw_clock_rate);
        cev->critical_path = prev_cp;
	  }
	ckp->s_nevent_processed++;
    // instrumentation
    ckp->kp_stats->s_nevent_processed++;
    clp->lp_stats->s_nevent_processed++;
	me->stats.s_event_process += tw_clock_read() - start;

	/* We ran out of events while processing this event.  We
	 * cannot continue without doing GVT and fossil collect.
	 */

	if (me->cev_abort)
	  {
	    start = tw_clock_read();
	    me->stats.s_nevent_abort++;
        // instrumentation
        ckp->kp_stats->s_nevent_abort++;
        clp->lp_stats->s_nevent_abort++;
	    me->cev_abort = 0;

	    tw_event_rollback(cev);
        pq_start = tw_clock_read();
	    tw_pq_enqueue(me->pq, cev);
        me->stats.s_pq += tw_clock_read() - pq_start;

	    cev = tw_eventq_peek(&ckp->pevent_q);
	    ckp->last_time = cev ? cev->recv_ts : me->GVT;

	    tw_gvt_force_update_realtime();

	    me->stats.s_event_abort += tw_clock_read() - start;

	    break; // leave the batch function
	  } // END ABORT CHECK

	/* Thread current event into processed queue of kp */
        cev->state.owner = TW_kp_pevent_q;
        tw_eventq_unshift(&ckp->pevent_q, cev);

	/* Check if realtime GVT time interval has expired */
	if( tw_clock_read() - g_tw_gvt_interval_start_cycles > g_tw_gvt_realtime_interval)
	  {
	    tw_gvt_force_update_realtime();
	    break; // leave the batch function
	  }

        if(g_st_rt_sampling &&
                tw_clock_read() - g_st_rt_samp_start_cycles > g_st_rt_interval)
        {
            tw_clock current_rt = tw_clock_read();
            if (g_st_engine_stats == RT_STATS || g_st_engine_stats == ALL_STATS)
                st_collect_engine_data(me, RT_COL);
            if (g_st_model_stats == RT_STATS || g_st_model_stats == ALL_STATS)
                st_collect_model_data(me, ((double)current_rt) / g_tw_clock_rate, RT_STATS);

            g_st_rt_samp_start_cycles = tw_clock_read();
        }
    }
}

void tw_sched_init(tw_pe * me) {
    /* First Stage Init */
    (*me->type.pre_lp_init)(me);
    tw_init_kps(me);
    tw_init_lps(me);
    (*me->type.post_lp_init)(me);

    tw_net_barrier();

    /* Second Stage Init -- all LPs are created and have proper mappings */
    tw_pre_run_lps(me);
    tw_net_barrier();

#ifdef USE_RIO
    tw_clock start = tw_clock_read();
    io_load_events(me);
    me->stats.s_rio_load += (tw_clock_read() - start);
    tw_net_barrier();
#endif

    /*
    * Recv all of the startup events out of the network before
    * starting simulation.. at this point, all LPs are done with init.
    */
    if (tw_nnodes() > 1) {
        tw_net_read(me);
        tw_net_barrier();
        tw_clock_init(me);
    }

    /* This lets the signal handler know that we have started
    * the scheduler loop, and to print out the stats before
    * finishing if someone should type CTRL-c
    */
    g_tw_sim_started = 1;
}

/*************************************************************************/
/* Primary Schedulers -- In order: Sequential, Conservative, Optimistic  */
/*************************************************************************/

void tw_scheduler_sequential(tw_pe * me) {
    tw_stime gvt = TW_STIME_CRT(0.0);

    if(tw_nnodes() > 1) {
        tw_error(TW_LOC, "Sequential Scheduler used for world size greater than 1.");
    }

    tw_event *cev;

    printf("*** START SEQUENTIAL SIMULATION ***\n\n");

    tw_wall_now(&me->start_time);
    me->stats.s_total = tw_clock_read();

    while ((cev = tw_pq_dequeue(me->pq))) {
        tw_lp *clp = cev->dest_lp;
        tw_kp *ckp = clp->kp;

        me->cur_event = cev;
        ckp->last_time = cev->recv_ts;

        if(TW_STIME_CMP(cev->recv_ts, tw_pq_minimum(me->pq)) == 0) {
            me->stats.s_pe_event_ties++;
        }

        gvt = cev->recv_ts;
        if(TW_STIME_DBL(gvt)/g_tw_ts_end > percent_complete && (g_tw_mynode == g_tw_masternode)) {
            gvt_print(gvt);
        }

        reset_bitfields(cev);
        clp->critical_path = ROSS_MAX(clp->critical_path, cev->critical_path)+1;
        (*clp->type->event)(clp->cur_state, &cev->cv, tw_event_data(cev), clp);
        if (g_st_ev_trace == FULL_TRACE)
            st_collect_event_data(cev, tw_clock_read() / g_tw_clock_rate);
        if (*clp->type->commit) {
            (*clp->type->commit)(clp->cur_state, &cev->cv, tw_event_data(cev), clp);
        }

        if (me->cev_abort){
            tw_error(TW_LOC, "insufficient event memory");
        }

        ckp->s_nevent_processed++;
        // instrumentation
        ckp->kp_stats->s_nevent_processed++;
        clp->lp_stats->s_nevent_processed++;
        tw_event_free(me, cev);

        if(g_st_rt_sampling &&
                tw_clock_read() - g_st_rt_samp_start_cycles > g_st_rt_interval)
        {
            tw_clock current_rt = tw_clock_read();
            if (g_st_model_stats == RT_STATS || g_st_model_stats == ALL_STATS)
                st_collect_model_data(me, ((double)current_rt) / g_tw_clock_rate, RT_STATS);

            g_st_rt_samp_start_cycles = tw_clock_read();
        }
    }
    tw_wall_now(&me->end_time);
    me->stats.s_total = tw_clock_read() - me->stats.s_total;

    printf("*** END SIMULATION ***\n\n");

    tw_stats(me);

    (*me->type.final)(me);
}

void tw_scheduler_conservative(tw_pe * me) {
    tw_clock start;
    unsigned int msg_i;

    if (g_tw_mynode == g_tw_masternode) {
        printf("*** START PARALLEL CONSERVATIVE SIMULATION ***\n\n");
    }

    tw_wall_now(&me->start_time);
    me->stats.s_total = tw_clock_read();

    for (;;){
        if (tw_nnodes() > 1){
            start = tw_clock_read();
            tw_net_read(me);
            me->stats.s_net_read += tw_clock_read() - start;
        }

        tw_gvt_step1(me);
        tw_sched_event_q(me);
        tw_gvt_step2(me);

        if (TW_STIME_DBL(me->GVT) > g_tw_ts_end) {
            break;
        }

        // put "batch" loop directly here
        /* Process g_tw_mblock events, or until the PQ is empty
        * (whichever comes first).
        */
        for (msg_i = g_tw_mblock; msg_i; msg_i--) {
            tw_event *cev;
            tw_lp *clp;
            tw_kp *ckp;

            /* OUT OF FREE EVENT BUFFERS.  BAD.
            * Go do fossil collect immediately.
            */
            if (me->free_q.size <= g_tw_gvt_threshold) {
                tw_gvt_force_update();
                break;
            }

            if(TW_STIME_DBL(tw_pq_minimum(me->pq)) >= TW_STIME_DBL(me->GVT) + g_tw_lookahead) {
                break;
            }

            start = tw_clock_read();
            if (!(cev = tw_pq_dequeue(me->pq))) {
                break;
            }
            me->stats.s_pq += tw_clock_read() - start;
            if(TW_STIME_CMP(cev->recv_ts, tw_pq_minimum(me->pq)) == 0) {
                me->stats.s_pe_event_ties++;
            }

            clp = cev->dest_lp;
            ckp = clp->kp;
            me->cur_event = cev;
            if( TW_STIME_CMP(ckp->last_time, cev->recv_ts) > 0 ){
                tw_error(TW_LOC, "Found KP last time %lf > current event time %lf for LP %d, PE %lu"
                        "src LP %lu, src PE %lu",
                ckp->last_time, cev->recv_ts, clp->gid, clp->pe->id,
                cev->send_lp, cev->send_pe);
            }
            ckp->last_time = cev->recv_ts;

            start = tw_clock_read();
            reset_bitfields(cev);
            clp->critical_path = ROSS_MAX(clp->critical_path, cev->critical_path)+1;
            (*clp->type->event)(clp->cur_state, &cev->cv, tw_event_data(cev), clp);
            if (g_st_ev_trace == FULL_TRACE)
                st_collect_event_data(cev, (double)tw_clock_read() / g_tw_clock_rate);
            if (*clp->type->commit) {
                (*clp->type->commit)(clp->cur_state, &cev->cv, tw_event_data(cev), clp);
            }

            ckp->s_nevent_processed++;
            // instrumentation
            ckp->kp_stats->s_nevent_processed++;
            clp->lp_stats->s_nevent_processed++;
            me->stats.s_event_process += tw_clock_read() - start;

            if (me->cev_abort) {
                tw_error(TW_LOC, "insufficient event memory");
            }

            tw_event_free(me, cev);

            if(g_st_rt_sampling &&
                    tw_clock_read() - g_st_rt_samp_start_cycles > g_st_rt_interval)
            {
                tw_clock current_rt = tw_clock_read();
                if (g_st_engine_stats == RT_STATS || g_st_engine_stats == ALL_STATS)
                    st_collect_engine_data(me, RT_COL);
                if (g_st_model_stats == RT_STATS || g_st_model_stats == ALL_STATS)
                    st_collect_model_data(me, ((double)current_rt) / g_tw_clock_rate, RT_STATS);

                g_st_rt_samp_start_cycles = tw_clock_read();
            }
        }
    }

    tw_wall_now(&me->end_time);
    me->stats.s_total = tw_clock_read() - me->stats.s_total;

    if (g_tw_mynode == g_tw_masternode) {
        printf("*** END SIMULATION ***\n\n");
    }

    tw_net_barrier();

    // call the model PE finalize function
    (*me->type.final)(me);

    st_inst_finalize(me);

    tw_stats(me);
}

void tw_scheduler_optimistic(tw_pe * me) {
    tw_clock start;

    if (g_tw_mynode == g_tw_masternode) {
        printf("*** START PARALLEL OPTIMISTIC SIMULATION WITH SUSPEND LP FEATURE ***\n\n");
    }

    tw_wall_now(&me->start_time);
    me->stats.s_total = tw_clock_read();

    for (;;) {
        if (tw_nnodes() > 1) {
            start = tw_clock_read();
            tw_net_read(me);
            me->stats.s_net_read += tw_clock_read() - start;
        }

        tw_gvt_step1(me);
        tw_sched_event_q(me);
        tw_sched_cancel_q(me);
        tw_gvt_step2(me);

        if (TW_STIME_DBL(me->GVT) > g_tw_ts_end) {
            break;
        }

        tw_sched_batch(me);
    }

    tw_wall_now(&me->end_time);
    me->stats.s_total = tw_clock_read() - me->stats.s_total;

    tw_net_barrier();

    if (g_tw_mynode == g_tw_masternode) {
        printf("*** END SIMULATION ***\n\n");
    }

    // call the model PE finalize function
    (*me->type.final)(me);

    st_inst_finalize(me);

    tw_stats(me);
}

void tw_scheduler_optimistic_realtime(tw_pe * me) {
    tw_clock start;

    g_tw_gvt_realtime_interval = g_tw_gvt_interval * g_tw_clock_rate / 1000;

    if (g_tw_mynode == g_tw_masternode) {
        printf("*** START PARALLEL OPTIMISTIC SIMULATION WITH SUSPEND LP FEATURE AND REAL TIME GVT ***\n\n");
    }

    tw_wall_now(&me->start_time);
    me->stats.s_total = tw_clock_read();

    // init the realtime GVT
    g_tw_gvt_interval_start_cycles = tw_clock_read();

    for (;;) {
        if (tw_nnodes() > 1) {
            start = tw_clock_read();
            tw_net_read(me);
            me->stats.s_net_read += tw_clock_read() - start;
        }

        tw_gvt_step1_realtime(me);
        tw_sched_event_q(me);
        tw_sched_cancel_q(me);
        tw_gvt_step2(me); // use regular step2 at this point

        if (TW_STIME_DBL(me->GVT) > g_tw_ts_end) {
            break;
        }

        tw_sched_batch_realtime(me);
    }

    tw_wall_now(&me->end_time);
    me->stats.s_total = tw_clock_read() - me->stats.s_total;

    tw_net_barrier();

    if (g_tw_mynode == g_tw_masternode) {
        printf("*** END SIMULATION ***\n\n");
    }

    // call the model PE finalize function
    (*me->type.final)(me);

    st_inst_finalize(me);

    tw_stats(me);
}

double g_tw_rollback_time = 0.000000001;

void tw_scheduler_optimistic_debug(tw_pe * me) {
    tw_event *cev=NULL;

    if(tw_nnodes() > 1) {
        tw_error(TW_LOC, "Sequential Scheduler used for world size greater than 1.");
    }

    printf("/***************************************************************************/\n");
    printf("/***** WARNING: Starting Optimistic Debug Scheduler!! **********************/\n");
    printf("This schedule assumes the following: \n");
    printf(" 1) One 1 Processor/Core is used.\n");
    printf(" 2) One 1 KP is used.\n");
    printf("    NOTE: use the --nkp=1 argument to the simulation to ensure that\n");
    printf("          it only uses 1 KP.\n");
    printf(" 3) Events ARE NEVER RECLAIMED (LP Commit Functions are not called).\n");
    printf(" 4) Executes til out of memory (16 events left) and \n    injects rollback to first before primodal init event.\n");
    printf(" 5) g_tw_rollback_time = %13.12lf \n", g_tw_rollback_time);
    printf("/***************************************************************************/\n");

    if( g_tw_nkp > 1 ) {
        tw_error(TW_LOC, "Number of KPs is greater than 1.");
    }

    tw_wall_now(&me->start_time);

    while ((cev = tw_pq_dequeue(me->pq))) {
        tw_lp *clp = cev->dest_lp;
        tw_kp *ckp = clp->kp;

        me->cur_event = cev;
        ckp->last_time = cev->recv_ts;

        /* don't update GVT */
        reset_bitfields(cev);

        // state-save and update the LP's critical path
        unsigned int prev_cp = clp->critical_path;
        clp->critical_path = ROSS_MAX(clp->critical_path, cev->critical_path)+1;
        (*clp->type->event)(clp->cur_state, &cev->cv, tw_event_data(cev), clp);
        cev->critical_path = prev_cp;

        ckp->s_nevent_processed++;

        /* Thread current event into processed queue of kp */
        cev->state.owner = TW_kp_pevent_q;
        tw_eventq_unshift(&ckp->pevent_q, cev);

        /* stop when we have 1024 events left */
        if ( me->free_q.size <= 1024) {
            break;
        }
    }

    // If we've run out of free events or events to process (maybe we're past end time?)
    // Perform all the rollbacks!
    printf("/******************* Starting Rollback Phase ******************************/\n");
    tw_kp_rollback_to( g_tw_kp[0], TW_STIME_CRT(g_tw_rollback_time) );
    printf("/******************* Completed Rollback Phase ******************************/\n");

    tw_wall_now(&me->end_time);

    printf("*** END SIMULATION ***\n\n");

    tw_stats(me);

    (*me->type.final)(me);
}
