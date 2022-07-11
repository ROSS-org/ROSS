#include <ross.h>
#include <sys/stat.h>
#define __STDC_FORMAT_MACROS 1

long g_st_current_interval = 0;
static tw_statistics last_pe_stats[3];
static tw_stat last_all_reduce_cnt = 0;

/* wrapper to call gvt instrumentation functions depending on which granularity to use */
void st_collect_engine_data(tw_pe *pe, int col_type)
{
    tw_clock start_time = tw_clock_read();
    tw_kp *kp;
    tw_lp *lp;
    unsigned int i;
    tw_statistics s;
    bzero(&s, sizeof(s));
    tw_get_stats(pe, &s);

    sample_metadata sample_md;
#ifdef USE_RAND_TIEBREAKER
    sample_md.ts = pe->GVT_sig.recv_ts;
#else
    sample_md.ts = pe->GVT;
#endif
    sample_md.real_time = (double)tw_clock_read() / g_tw_clock_rate;

    if (g_st_pe_data)
        st_collect_engine_data_pes(pe, &sample_md, &s, col_type);
    if (g_st_kp_data)
    {
        for (i = 0; i < g_tw_nkp; i++)
        {
            kp = tw_getkp(i);
            st_collect_engine_data_kps(pe, kp, &sample_md, col_type);
        }
    }
    if (g_st_lp_data)
    {
        for (i = 0; i < g_tw_nlp; i++)
        {
            lp = tw_getlp(i);
            st_collect_engine_data_lps(lp, &sample_md, col_type);
        }
    }
    pe->stats.s_stat_comp += tw_clock_read() - start_time;
}

void st_collect_engine_data_pes(tw_pe *pe, sample_metadata *sample_md, tw_statistics *s, int col_type)
{
    st_pe_stats pe_stats;
    int buf_size = sizeof(*sample_md) + sizeof(pe_stats);
    char buffer[buf_size];
    tw_stat all_reduce_cnt = st_get_allreduce_count();

    // sample_md time stamps were set in the calling function
    sample_md->flag = PE_TYPE;
    sample_md->sample_sz = sizeof(pe_stats);

    pe_stats.peid = (unsigned int) g_tw_mynode;
    pe_stats.s_nevent_processed = (unsigned int)( s->s_nevent_processed-last_pe_stats[col_type].s_nevent_processed);
    pe_stats.s_nevent_abort = (unsigned int)(s->s_nevent_abort-last_pe_stats[col_type].s_nevent_abort);
    pe_stats.s_e_rbs = (unsigned int)(s->s_e_rbs-last_pe_stats[col_type].s_e_rbs);
    pe_stats.s_rb_total = (unsigned int)( s->s_rb_total-last_pe_stats[col_type].s_rb_total);
    pe_stats.s_rb_secondary = (unsigned int)(s->s_rb_secondary-last_pe_stats[col_type].s_rb_secondary);
    pe_stats.s_fc_attempts = (unsigned int)(s->s_fc_attempts-last_pe_stats[col_type].s_fc_attempts);
    pe_stats.s_pq_qsize = tw_pq_get_size(pe->pq);
    pe_stats.s_nsend_network = (unsigned int)(s->s_nsend_network-last_pe_stats[col_type].s_nsend_network);
    pe_stats.s_nread_network = (unsigned int)(s->s_nread_network-last_pe_stats[col_type].s_nread_network);
    pe_stats.s_pe_event_ties = (unsigned int)(s->s_pe_event_ties-last_pe_stats[col_type].s_pe_event_ties);
    pe_stats.s_ngvts = (unsigned int)(g_tw_gvt_done - last_pe_stats[col_type].s_ngvts);
    pe_stats.all_reduce_count = (unsigned int)(all_reduce_cnt-last_all_reduce_cnt);

    // I think it's possible for net_events to be negative over some interval of simulation time
    // e.g., if in the current interval we've happened to process more rollback events than forward events
    // for now, just report efficiency as 0 in this case?
    int net_events = pe_stats.s_nevent_processed - pe_stats.s_e_rbs;
    if (net_events > 0)
        pe_stats.efficiency = (float) 100.0 * (1.0 - ((float) pe_stats.s_e_rbs / (float) net_events));
    else
        pe_stats.efficiency = 0;

    // TODO set a starting clock rate and subtract that from the counters?
    // because PEs on different nodes will probably have different starting points for cycle counters
    pe_stats.s_net_read = (float)(pe->stats.s_net_read - last_pe_stats[col_type].s_net_read) / g_tw_clock_rate;
    pe_stats.s_net_other = (float)(pe->stats.s_net_other - last_pe_stats[col_type].s_net_other) / g_tw_clock_rate;
    pe_stats.s_gvt = (float)(pe->stats.s_gvt - last_pe_stats[col_type].s_gvt) / g_tw_clock_rate;
    pe_stats.s_fossil_collect = (float)(pe->stats.s_fossil_collect - last_pe_stats[col_type].s_fossil_collect) / g_tw_clock_rate;
    pe_stats.s_event_abort = (float)(pe->stats.s_event_abort - last_pe_stats[col_type].s_event_abort) / g_tw_clock_rate;
    pe_stats.s_event_process = (float)(pe->stats.s_event_process - last_pe_stats[col_type].s_event_process) / g_tw_clock_rate;
    pe_stats.s_pq = (float)(pe->stats.s_pq - last_pe_stats[col_type].s_pq) / g_tw_clock_rate;
    pe_stats.s_rollback = (float)(pe->stats.s_rollback - last_pe_stats[col_type].s_rollback) / g_tw_clock_rate;
    pe_stats.s_cancel_q = (float)(pe->stats.s_cancel_q - last_pe_stats[col_type].s_cancel_q) / g_tw_clock_rate;
    pe_stats.s_avl = (float)(pe->stats.s_avl - last_pe_stats[col_type].s_avl) / g_tw_clock_rate;
    pe_stats.s_buddy = (float)(pe->stats.s_buddy - last_pe_stats[col_type].s_buddy) / g_tw_clock_rate;
    pe_stats.s_lz4 = (float)(pe->stats.s_lz4 - last_pe_stats[col_type].s_lz4) / g_tw_clock_rate;

    memcpy(&buffer[0], sample_md, sizeof(*sample_md));
    memcpy(&buffer[sizeof(*sample_md)], &pe_stats, sizeof(pe_stats));
    st_buffer_push(col_type, &buffer[0], buf_size);

    memcpy(&last_pe_stats[col_type], s, sizeof(tw_statistics));
    last_all_reduce_cnt = all_reduce_cnt;
}

void st_collect_engine_data_kps(tw_pe *pe, tw_kp *kp, sample_metadata *sample_md, int col_type)
{
    st_kp_stats kp_stats;
    int buf_size = sizeof(*sample_md) + sizeof(kp_stats);
    char buffer[buf_size];
    int index = 0;

    // sample_md time stamps were set in the calling function
    sample_md->flag = KP_TYPE;
    sample_md->sample_sz = sizeof(kp_stats);

    kp_stats.peid = (unsigned int) g_tw_mynode;

    kp_stats.kpid = kp->id;

    kp_stats.s_nevent_processed = (unsigned int)(kp->kp_stats->s_nevent_processed - kp->last_stats[col_type]->s_nevent_processed);
    kp_stats.s_nevent_abort = (unsigned int)(kp->kp_stats->s_nevent_abort - kp->last_stats[col_type]->s_nevent_abort);
    kp_stats.s_e_rbs = (unsigned int)(kp->kp_stats->s_e_rbs - kp->last_stats[col_type]->s_e_rbs);
    kp_stats.s_rb_total = (unsigned int)(kp->kp_stats->s_rb_total - kp->last_stats[col_type]->s_rb_total);
    kp_stats.s_rb_secondary = (unsigned int)(kp->kp_stats->s_rb_secondary - kp->last_stats[col_type]->s_rb_secondary);
    kp_stats.s_nsend_network = (unsigned int)(kp->kp_stats->s_nsend_network - kp->last_stats[col_type]->s_nsend_network);
    kp_stats.s_nread_network = (unsigned int)(kp->kp_stats->s_nread_network - kp->last_stats[col_type]->s_nread_network);
#ifdef USE_RAND_TIEBREAKER
    kp_stats.time_ahead_gvt = (float)(TW_STIME_DBL(kp->last_sig.recv_ts) - TW_STIME_DBL(pe->GVT_sig.recv_ts));
#else
    kp_stats.time_ahead_gvt = (float)(TW_STIME_DBL(kp->last_time) - TW_STIME_DBL(pe->GVT));
#endif

    int net_events = kp_stats.s_nevent_processed - kp_stats.s_e_rbs;
    if (net_events > 0)
        kp_stats.efficiency = (float) 100.0 * (1.0 - ((float) kp_stats.s_e_rbs / (float) net_events));
    else
        kp_stats.efficiency = 0;

    memcpy(kp->last_stats[col_type], kp->kp_stats, sizeof(st_kp_stats));

    memcpy(&buffer[index], sample_md, sizeof(*sample_md));
    index += sizeof(*sample_md);
    memcpy(&buffer[index], &kp_stats, sizeof(kp_stats));
    index += sizeof(kp_stats);

    if (index != buf_size)
        tw_error(TW_LOC, "size of data being pushed to buffer is incorrect!\n");

    st_buffer_push(col_type, &buffer[0], buf_size);
}

void st_collect_engine_data_lps(tw_lp *lp, sample_metadata *sample_md, int col_type)
{
    st_lp_stats lp_stats;
    int buf_size = sizeof(*sample_md) + sizeof(lp_stats);
    char buffer[buf_size];
    int index = 0;

    // sample_md time stamps were set in the calling function
    sample_md->flag = LP_TYPE;
    sample_md->sample_sz = sizeof(lp_stats);

    lp_stats.peid = (unsigned int) g_tw_mynode;

    lp_stats.kpid = lp->kp->id;
    lp_stats.lpid = lp->gid;

    lp_stats.s_nevent_processed = (unsigned int)(lp->lp_stats->s_nevent_processed - lp->last_stats[col_type]->s_nevent_processed);
    lp_stats.s_nevent_abort = (unsigned int)(lp->lp_stats->s_nevent_abort - lp->last_stats[col_type]->s_nevent_abort);
    lp_stats.s_e_rbs = (unsigned int)(lp->lp_stats->s_e_rbs - lp->last_stats[col_type]->s_e_rbs);
    lp_stats.s_nsend_network = (unsigned int)(lp->lp_stats->s_nsend_network - lp->last_stats[col_type]->s_nsend_network);
    lp_stats.s_nread_network = (unsigned int)(lp->lp_stats->s_nread_network - lp->last_stats[col_type]->s_nread_network);

    int net_events = lp_stats.s_nevent_processed - lp_stats.s_e_rbs;
    if (net_events > 0)
        lp_stats.efficiency = (float) 100.0 * (1.0 - ((float) lp_stats.s_e_rbs / (float) net_events));
    else
        lp_stats.efficiency = 0;

    memcpy(lp->last_stats[col_type], lp->lp_stats, sizeof(st_lp_stats));

    memcpy(&buffer[index], sample_md, sizeof(*sample_md));
    index += sizeof(*sample_md);
    memcpy(&buffer[index], &lp_stats, sizeof(lp_stats));
    index += sizeof(lp_stats);

    if (index != buf_size)
        tw_error(TW_LOC, "size of data being pushed to buffer is incorrect!\n");

    st_buffer_push(col_type, &buffer[0], buf_size);
}
