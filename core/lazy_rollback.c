#include <ross.h>

int lazy_q_annihilate (tw_pe *pe, tw_event *e) {
    tw_clock net_start;
    int annihilation_achieved = 0;
    tw_pe *send_pe = e->src_lp->pe;

    tw_event *cev = tw_eventq_peek_head(&pe->lazy_q);
    while (cev) {

        if (TW_STIME_CMP(cev->send_ts, e->send_ts) == 0) {
            // assuming unique tuple (send lp, dest lp, send time, recv time)
            // assuming the queue is in sent time order

            if (cev->send_lp == e->send_lp &&
                cev->dest_lp == e->dest_lp &&
                TW_STIME_CMP(cev->recv_ts, e->recv_ts) == 0) {

                // candidate found, remove from lazy_q
                tw_eventq_delete_any(&pe->lazy_q, cev);

                if (memcmp(&cev->cv, &e->cv, sizeof(tw_bf)) == 0 &&
                    memcmp(&cev->delta_size, &e->delta_size, (sizeof(size_t))) == 0 && // todo check delta_buddy?
                    cev->critical_path == e->critical_path &&
                    memcmp(tw_event_data(cev), tw_event_data(e), g_tw_msg_sz) == 0) {

                    // optimization achieved
                    annihilation_achieved = 1;
                    tw_event_free(send_pe, cev);
                } else {
                    // optimization not achieved, must send cev as antimessage
                    send_pe->stats.s_nsend_net_remote++;
                    cev->state.owner = TW_net_asend;
                    net_start = tw_clock_read();
                    tw_net_send(cev);
                    send_pe->stats.s_net_other += tw_clock_read() - net_start;
                }

                break;
            }
        } else {
            // todo: can we assume that head of the lazy q should be >= current event?
            break;
        }
        cev = cev->next;
    }
    return annihilation_achieved;
}

void lazy_rollback_catchup_to(tw_pe *pe, tw_stime timestamp) {
    tw_clock net_start;

    tw_event *cev = tw_eventq_peek_head(&pe->lazy_q);

    while (cev) {
        if (TW_STIME_CMP(cev->send_ts, timestamp) < 0) {
            tw_event *e = cev;
            cev = cev->next;

            tw_eventq_delete_any(&pe->lazy_q, e);

            pe->stats.s_nsend_net_remote++;
            e->state.owner = TW_net_asend;
            net_start = tw_clock_read();
            tw_net_send(e);
            pe->stats.s_net_other += tw_clock_read() - net_start;
        } else {
            break;
        }
    }

    return;
}

void lazy_q_insert(tw_pe *pe, tw_event *e) {
    pe->stats.s_n_lazy_events++;
    e->state.owner = TW_pe_lazy_q;
    tw_eventq_insert_send_ts(&pe->lazy_q, e);
    return;
}
