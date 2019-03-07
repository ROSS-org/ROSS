#include <ross.h>
#include <sys/stat.h>
#include <instrumentation/st-stats-buffer.h>
#include <instrumentation/st-instrumentation-internal.h>
#include <instrumentation/st-sim-engine.h>
#include <instrumentation/ross-lps/analysis-lp.h>


//#define __STDC_FORMAT_MACROS 1

long g_st_current_interval = 0;
static tw_statistics last_pe_stats[3];
static tw_stat last_all_reduce_cnt = 0;

size_t calc_sim_engine_sample_size()
{
    if (g_tw_synchronization_protocol == SEQUENTIAL)
        return 0;

    size_t total_size = 0;
    if (g_st_pe_data)
        total_size += sizeof(st_pe_stats);
    if (g_st_kp_data)
        total_size += sizeof(st_kp_stats) * g_tw_nkp;
    if (g_st_lp_data)
        total_size += sizeof(st_lp_stats) * g_tw_nlp;
    return total_size;
}

// has to be calculated at each sample because may have
// differing numbers of lps per analysis LP?
size_t calc_sim_engine_sample_size_vts(tw_lp *lp)
{
    if (g_tw_synchronization_protocol == SEQUENTIAL)
        return 0;

    size_t total_size = 0;
    if (g_st_pe_data && lp->kp->id == 0)
        total_size += sizeof(st_pe_stats);
    if (g_st_kp_data)
        total_size += sizeof(st_kp_stats);
    if (g_st_lp_data)
    {
        int num_lps;
        get_sim_lp_list(lp->cur_state, &num_lps);
        total_size += sizeof(st_lp_stats) * num_lps;
    }
    return total_size;
}

/* wrapper to call gvt instrumentation functions depending on which granularity to use */
void st_collect_engine_data(tw_pe* pe, int col_type, char* buffer, size_t data_size,
        sample_metadata* sample_md, tw_lp *alp) // LP only used for VTS, ptr to analysis LP
{
    tw_clock start_time = tw_clock_read();
    char* cur_buf = buffer;
    size_t cur_size = 0;

    tw_kp* kp;
    tw_lp* lp;
    int i;
    tw_statistics s;
    bzero(&s, sizeof(s));
    tw_get_stats(pe, &s);
    int start_kp = 0, end_kp = 0;
    int num_lps = 0;
    tw_lpid *lp_list;

    switch (col_type)
    {
        case GVT_INST:
        case RT_INST:
            if (g_st_pe_data)
                sample_md->has_pe = 1;

            if (g_st_kp_data)
            {
                sample_md->has_kp = g_tw_nkp;
                start_kp = 0;
                end_kp = g_tw_nkp;
            }

            if (g_st_lp_data)
            {
                sample_md->has_lp = g_tw_nlp;
                num_lps = g_tw_nlp;
            }
            break;
        case VT_INST:
            if (g_st_pe_data && alp->kp->id == 0)
                sample_md->has_pe = 1;

            if (g_st_kp_data)
            {
                sample_md->has_kp = 1;
                start_kp = alp->kp->id;
                end_kp = start_kp + 1;
            }

            if (g_st_lp_data)
            {
                lp_list = get_sim_lp_list(alp->cur_state, &num_lps);
                sample_md->has_lp = num_lps;
            }
            break;
        default:
            break;
    }
    if (sample_md->has_pe)
    {
        st_pe_stats* pe_stats = (st_pe_stats*)cur_buf;
        cur_buf += sizeof(*pe_stats);
        cur_size += sizeof(*pe_stats);
        st_collect_engine_data_pes(pe, &s, col_type, pe_stats);
    }
    if (sample_md->has_kp)
    {
        for (i = start_kp; i < end_kp; i++)
        {
            st_kp_stats* kp_stats = (st_kp_stats*)cur_buf;
            cur_buf += sizeof(*kp_stats);
            cur_size += sizeof(*kp_stats);
            kp = tw_getkp(i);
            st_collect_engine_data_kps(pe, kp, &s, col_type, kp_stats);
        }
    }
    if (sample_md->has_lp)
    {
        for (i = 0; i < num_lps; i++)
        {
            st_lp_stats* lp_stats = (st_lp_stats*)cur_buf;
            cur_buf += sizeof(*lp_stats);
            cur_size += sizeof(*lp_stats);
            if (col_type == VT_INST)
                lp = tw_getlocal_lp(lp_list[i]);
            else
                lp = tw_getlp(i);
            st_collect_engine_data_lps(pe, lp, &s, col_type, lp_stats);
        }
    }
    pe->stats.s_stat_comp += tw_clock_read() - start_time;
}

void st_collect_engine_data_pes(tw_pe *pe, tw_statistics *s, int col_type, st_pe_stats *pe_stats)
{
    tw_stat all_reduce_cnt = st_get_allreduce_count();

    pe_stats->peid = (unsigned int) g_tw_mynode;
    pe_stats->s_nevent_processed = (unsigned int)(s->s_nevent_processed-last_pe_stats[col_type].s_nevent_processed);
    pe_stats->s_nevent_abort = (unsigned int)(s->s_nevent_abort-last_pe_stats[col_type].s_nevent_abort);
    pe_stats->s_e_rbs = (unsigned int)(s->s_e_rbs-last_pe_stats[col_type].s_e_rbs);
    pe_stats->s_rb_total = (unsigned int)( s->s_rb_total-last_pe_stats[col_type].s_rb_total);
    pe_stats->s_rb_secondary = (unsigned int)(s->s_rb_secondary-last_pe_stats[col_type].s_rb_secondary);
    pe_stats->s_rb_primary = pe_stats->s_rb_total - pe_stats->s_rb_secondary;
    pe_stats->s_fc_attempts = (unsigned int)(s->s_fc_attempts-last_pe_stats[col_type].s_fc_attempts);
    pe_stats->s_pq_qsize = tw_pq_get_size(pe->pq);
    pe_stats->s_nsend_network = (unsigned int)(s->s_nsend_network-last_pe_stats[col_type].s_nsend_network);
    pe_stats->s_nread_network = (unsigned int)(s->s_nread_network-last_pe_stats[col_type].s_nread_network);
    pe_stats->s_pe_event_ties = (unsigned int)(s->s_pe_event_ties-last_pe_stats[col_type].s_pe_event_ties);
    pe_stats->s_ngvts = (unsigned int)(g_tw_gvt_done - last_pe_stats[col_type].s_ngvts);
    pe_stats->all_reduce_count = (unsigned int)(all_reduce_cnt-last_all_reduce_cnt);


    // TODO set a starting clock rate and subtract that from the counters?
    // because PEs on different nodes will probably have different starting points for cycle counters
    pe_stats->s_net_read = (float)(pe->stats.s_net_read - last_pe_stats[col_type].s_net_read) / g_tw_clock_rate;
    pe_stats->s_net_other = (float)(pe->stats.s_net_other - last_pe_stats[col_type].s_net_other) / g_tw_clock_rate;
    pe_stats->s_gvt = (float)(pe->stats.s_gvt - last_pe_stats[col_type].s_gvt) / g_tw_clock_rate;
    pe_stats->s_fossil_collect = (float)(pe->stats.s_fossil_collect - last_pe_stats[col_type].s_fossil_collect) / g_tw_clock_rate;
    pe_stats->s_event_abort = (float)(pe->stats.s_event_abort - last_pe_stats[col_type].s_event_abort) / g_tw_clock_rate;
    pe_stats->s_event_process = (float)(pe->stats.s_event_process - last_pe_stats[col_type].s_event_process) / g_tw_clock_rate;
    pe_stats->s_pq = (float)(pe->stats.s_pq - last_pe_stats[col_type].s_pq) / g_tw_clock_rate;
    pe_stats->s_rollback = (float)(pe->stats.s_rollback - last_pe_stats[col_type].s_rollback) / g_tw_clock_rate;
    pe_stats->s_cancel_q = (float)(pe->stats.s_cancel_q - last_pe_stats[col_type].s_cancel_q) / g_tw_clock_rate;
    pe_stats->s_avl = (float)(pe->stats.s_avl - last_pe_stats[col_type].s_avl) / g_tw_clock_rate;
    pe_stats->s_buddy = (float)(pe->stats.s_buddy - last_pe_stats[col_type].s_buddy) / g_tw_clock_rate;
    pe_stats->s_lz4 = (float)(pe->stats.s_lz4 - last_pe_stats[col_type].s_lz4) / g_tw_clock_rate;

    memcpy(&last_pe_stats[col_type], s, sizeof(tw_statistics));
    last_all_reduce_cnt = all_reduce_cnt;
}

void st_collect_engine_data_kps(tw_pe *pe, tw_kp *kp, tw_statistics *s, int col_type, st_kp_stats* kp_stats)
{
    kp_stats->kpid = (unsigned int)kp->id;

    kp_stats->s_nevent_processed = (unsigned int)(kp->kp_stats->s_nevent_processed - kp->last_stats[col_type]->s_nevent_processed);
    kp_stats->s_nevent_abort = (unsigned int)(kp->kp_stats->s_nevent_abort - kp->last_stats[col_type]->s_nevent_abort);
    kp_stats->s_e_rbs = (unsigned int)(kp->kp_stats->s_e_rbs - kp->last_stats[col_type]->s_e_rbs);
    kp_stats->s_rb_total = (unsigned int)(kp->kp_stats->s_rb_total - kp->last_stats[col_type]->s_rb_total);
    kp_stats->s_rb_secondary = (unsigned int)(kp->kp_stats->s_rb_secondary - kp->last_stats[col_type]->s_rb_secondary);
    kp_stats->s_rb_primary = kp_stats->s_rb_total - kp_stats->s_rb_secondary;
    kp_stats->s_nsend_network = (unsigned int)(kp->kp_stats->s_nsend_network - kp->last_stats[col_type]->s_nsend_network);
    kp_stats->s_nread_network = (unsigned int)(kp->kp_stats->s_nread_network - kp->last_stats[col_type]->s_nread_network);
    kp_stats->time_ahead_gvt = (float)(kp->last_time - pe->GVT);

    memcpy(kp->last_stats[col_type], kp->kp_stats, sizeof(st_kp_stats));
}

void st_collect_engine_data_lps(tw_pe* pe, tw_lp* lp, tw_statistics* s, int col_type, st_lp_stats* lp_stats)
{
    lp_stats->lpid = (unsigned int)lp->gid;
    lp_stats->kpid = (unsigned int)lp->kp->id;

    lp_stats->s_nevent_processed = (unsigned int)(lp->lp_stats->s_nevent_processed - lp->last_stats[col_type]->s_nevent_processed);
    lp_stats->s_nevent_abort = (unsigned int)(lp->lp_stats->s_nevent_abort - lp->last_stats[col_type]->s_nevent_abort);
    lp_stats->s_e_rbs = (unsigned int)(lp->lp_stats->s_e_rbs - lp->last_stats[col_type]->s_e_rbs);
    lp_stats->s_nsend_network = (unsigned int)(lp->lp_stats->s_nsend_network - lp->last_stats[col_type]->s_nsend_network);
    lp_stats->s_nread_network = (unsigned int)(lp->lp_stats->s_nread_network - lp->last_stats[col_type]->s_nread_network);

    memcpy(lp->last_stats[col_type], lp->lp_stats, sizeof(st_lp_stats));
}

