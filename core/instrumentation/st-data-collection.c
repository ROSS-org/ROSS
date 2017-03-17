#include <ross.h>
#include <sys/stat.h>
#define __STDC_FORMAT_MACROS 1
#include <inttypes.h>

extern MPI_Comm MPI_COMM_ROSS;

char g_st_stats_out[MAX_LENGTH] = {0};
int g_st_stats_enabled = 0;
long g_st_current_interval = 0;
tw_clock g_st_real_time_samp = 0;
tw_clock g_st_real_samp_start_cycles = 0;
static tw_statistics last_stats = {0};
static tw_stat last_all_reduce_cnt = 0;
int g_st_disable_out = 0;
st_cycle_counters last_cycle_counters = {0};
st_event_counters last_event_counters = {0};
int g_st_granularity = 0;
tw_clock g_st_stat_write_ctr = 0;
tw_clock g_st_stat_comp_ctr = 0;
int g_st_num_gvt = 10;

static int num_gvt_vals = 10;
static int num_gvt_vals_pe = 4;
static int num_gvt_vals_kp = 2;
static int num_gvt_vals_lp = 4;
static int num_cycle_ctrs = 11;
static int num_ev_ctrs = 12;
static int num_ev_ctrs_pe = 5;
static int num_ev_ctrs_kp = 2;
static int num_ev_ctrs_lp = 4;

static const tw_optdef stats_options[] = {
    TWOPT_GROUP("ROSS Stats"),
    TWOPT_UINT("enable-gvt-stats", g_st_stats_enabled, "Collect data after each GVT; 0 no stats, 1 for stats"),
    TWOPT_UINT("num-gvt", g_st_num_gvt, "number of GVT computations between GVT-based sampling points"),
    TWOPT_ULONGLONG("real-time-samp", g_st_real_time_samp, "real time sampling interval in ms"),
    TWOPT_UINT("granularity", g_st_granularity, "collect on PE basis only, or also KP/LP basis, 0 for PE, 1 for KP/LP"),
    TWOPT_UINT("event-trace", g_st_ev_trace, "collect detailed data on all events for specified LPs; 0, no trace, 1 full trace, 2 only events causing rollbacks, 3 only committed events"),
    TWOPT_CHAR("stats-filename", g_st_stats_out, "prefix for filename(s) for stats output"),
    TWOPT_UINT("buffer-size", g_st_buffer_size, "size of buffer in bytes for stats collection"),
    TWOPT_UINT("buffer-free", g_st_buffer_free_percent, "percentage of free space left in buffer before writing out at GVT"),
    TWOPT_UINT("disable-output", g_st_disable_out, "used for perturbation analysis; buffer never dumped to file when 1"),
    TWOPT_END()
};

const tw_optdef *tw_stats_setup(void)
{
	return stats_options;
}

void st_stats_init()
{
    // Need to call after st_buffer_init()!
    int i;
    int npe = tw_nnodes();
    tw_lpid nlp_per_pe[npe];
    MPI_Gather(&g_tw_nlp, 1, MPI_UINT64_T, nlp_per_pe, 1, MPI_UINT64_T, 0, MPI_COMM_ROSS);

    if (!g_st_disable_out && g_tw_mynode == 0) {
        FILE *file;
        char filename[MAX_LENGTH];
        sprintf(filename, "%s/%s-README.txt", g_st_directory, g_st_stats_out);
        file = fopen(filename, "w");

        /* start of metadata info for binary reader */
        fprintf(file, "GRANULARITY=%d\n", g_st_granularity);
        fprintf(file, "NUM_PE=%d\n", tw_nnodes());
        fprintf(file, "NUM_KP=%lu\n", g_tw_nkp);

        fprintf(file, "LP_PER_PE=");
        for (i = 0; i < npe; i++)
        {
            if (i == npe - 1)
                fprintf(file, "%"PRIu64"\n", nlp_per_pe[i]);
            else
                fprintf(file, "%"PRIu64",", nlp_per_pe[i]);
        }

        if (g_st_stats_enabled)
        {
            if (g_st_granularity)
            {
                fprintf(file, "NUM_GVT_VALS_PE=%d\n", num_gvt_vals_pe);
                fprintf(file, "NUM_GVT_VALS_KP=%d\n", num_gvt_vals_kp);
                fprintf(file, "NUM_GVT_VALS_LP=%d\n", num_gvt_vals_lp);
            }
            else
                fprintf(file, "NUM_GVT_VALS_PE=%d\n", num_gvt_vals);
        }

        if (g_st_real_time_samp)
        {
            fprintf(file, "NUM_CYCLE_CTRS=%d\n", num_cycle_ctrs);
            if (g_st_granularity)
            {
                fprintf(file, "NUM_EV_CTRS_PE=%d\n", num_ev_ctrs_pe);
                fprintf(file, "NUM_EV_CTRS_KP=%d\n", num_ev_ctrs_kp);
                fprintf(file, "NUM_EV_CTRS_LP=%d\n", num_ev_ctrs_lp);
            }
            else
                fprintf(file, "NUM_EV_CTRS_PE=%d\n", num_ev_ctrs);
        }

        fprintf(file, "END\n\n");

        /* end of metadata info for binary reader */
        fprintf(file, "Info for ROSS run.\n\n");
#if HAVE_CTIME
        time_t raw_time;
        time(&raw_time);
        fprintf(file, "Date Created:\t%s\n", ctime(&raw_time));
#endif
        fprintf(file, "## BUILD CONFIGURATION\n\n");
#ifdef ROSS_VERSION
        fprintf(file, "ROSS Version:\t%s\n", ROSS_VERSION);
#endif
        //fprintf(file, "MODEL Version:\t%s\n", model_version);
        fprintf(file, "\n## RUN TIME SETTINGS by GROUP:\n\n");
        tw_opt_settings(file);
        fclose(file);
    }
}

/* wrapper to call gvt log functions depending on which granularity to use */
void st_gvt_log(tw_pe *me, tw_stime gvt, tw_statistics *s, tw_stat all_reduce_cnt)
{
    if (g_st_granularity == 0)
        st_gvt_log_pes(me, gvt, s, all_reduce_cnt);
    else
        st_gvt_log_lps(me, gvt, s, all_reduce_cnt);

}

void st_gvt_log_pes(tw_pe *me, tw_stime gvt, tw_statistics *s, tw_stat all_reduce_cnt)
{
    //int buf_size = sizeof(tw_stat) * 11 + sizeof(tw_node) + sizeof(tw_stime) + sizeof(double) + sizeof(long long) *2;
    int buf_size = sizeof(unsigned int) * num_gvt_vals + sizeof(unsigned short) + sizeof(float) * 3  + sizeof(int);
    int index = 0;
    char buffer[buf_size];
    //tw_stat tmp;
    unsigned int tmp;
    int tmp2;
    float eff;
    unsigned short nodeid;
    tw_clock current_rt = (double)tw_clock_read() / g_tw_clock_rate;

    nodeid = (unsigned short) g_tw_mynode;
    memcpy(&buffer[index], &nodeid, sizeof(nodeid));
    index += sizeof(nodeid);

    eff = (float)gvt;
    memcpy(&buffer[index], &eff, sizeof(eff));
    index += sizeof(eff);

    eff = (float) current_rt;
    memcpy(&buffer[index], &eff, sizeof(eff));
    index += sizeof(eff);

    tmp = (unsigned int)(all_reduce_cnt-last_all_reduce_cnt);
    memcpy(&buffer[index], &tmp, sizeof(tmp));
    index += sizeof(tmp);

    tmp = (unsigned int)( s->s_nevent_processed-last_stats.s_nevent_processed);
    memcpy(&buffer[index], &tmp, sizeof(tmp));
    index += sizeof(tmp);

    tmp = (unsigned int)(s->s_nevent_abort-last_stats.s_nevent_abort);
    memcpy(&buffer[index], &tmp, sizeof(tmp));
    index += sizeof(tmp);

    tmp = (unsigned int)(s->s_e_rbs-last_stats.s_e_rbs);
    memcpy(&buffer[index], &tmp, sizeof(tmp));
    index += sizeof(tmp);

    tmp = (unsigned int)(s->s_pe_event_ties-last_stats.s_pe_event_ties);
    memcpy(&buffer[index], &tmp, sizeof(tmp));
    index += sizeof(tmp);

    tmp = (unsigned int)( s->s_rb_total-last_stats.s_rb_total);
    memcpy(&buffer[index], &tmp, sizeof(tmp));
    index += sizeof(tmp);

    tmp = (unsigned int)(s->s_rb_secondary-last_stats.s_rb_secondary);
    memcpy(&buffer[index], &tmp, sizeof(tmp));
    index += sizeof(tmp);

    tmp = (unsigned int)(s->s_fc_attempts-last_stats.s_fc_attempts);
    memcpy(&buffer[index], &tmp, sizeof(tmp));
    index += sizeof(tmp);

    tmp = (unsigned int)(s->s_nsend_network-last_stats.s_nsend_network);
    memcpy(&buffer[index], &tmp, sizeof(tmp));
    index += sizeof(tmp);

    tmp = (unsigned int)(s->s_nread_network-last_stats.s_nread_network);
    memcpy(&buffer[index], &tmp, sizeof(tmp));
    index += sizeof(tmp);

    // next stat not guaranteed to always be non-decreasing
    // can't use tw_stat (unsigned long long) for negative number
    tmp2 = (int)s->s_nsend_net_remote-(int)last_stats.s_nsend_net_remote;
    memcpy(&buffer[index], &tmp2, sizeof(tmp2));
    index += sizeof(tmp2);

    eff = (float) 100.0 * (1.0 - ((float) (s->s_e_rbs-last_stats.s_e_rbs)/(float) (s->s_net_events-last_stats.s_net_events)));
    memcpy(&buffer[index], &eff, sizeof(float));
    index += sizeof(float);

    if (index != buf_size)
        tw_error(TW_LOC, "size of data being pushed to buffer is incorrect!\n");

    st_buffer_push(g_st_buffer_gvt, &buffer[0], buf_size);
    memcpy(&last_stats, s, sizeof(tw_statistics));
    last_all_reduce_cnt = all_reduce_cnt;
}

void st_gvt_log_lps(tw_pe *me, tw_stime gvt, tw_statistics *s, tw_stat all_reduce_cnt)
{
    //int buf_size = sizeof(tw_node) + sizeof(tw_stime) + sizeof(tw_stat) * 4 + sizeof(double) + sizeof(long long) +
    //    sizeof(tw_stat) * 2 * g_tw_nkp + sizeof(tw_stat) * 4 * g_tw_nlp + sizeof(long long) * g_tw_nlp;
    int buf_size = sizeof(unsigned short) + sizeof(float) * 2 + sizeof(unsigned int) * num_gvt_vals_pe + sizeof(int) + sizeof(float) +
        sizeof(unsigned int) * num_gvt_vals_kp * g_tw_nkp + sizeof(unsigned int) * num_gvt_vals_lp * g_tw_nlp + sizeof(int) * g_tw_nlp;
    int index = 0, i;
    char buffer[buf_size];
    unsigned int tmp;
    int tmp2;
    float eff;
    tw_kp *kp;
    tw_lp *lp;
    unsigned short nodeid;
    tw_clock current_rt = (double)tw_clock_read() / g_tw_clock_rate;

    /* PE granularity */
    nodeid = (unsigned short) g_tw_mynode;
    memcpy(&buffer[index], &nodeid, sizeof(nodeid));
    index += sizeof(nodeid);

    eff = (float)gvt;
    memcpy(&buffer[index], &eff, sizeof(float));
    index += sizeof(float);

    eff = (float) current_rt;
    memcpy(&buffer[index], &eff, sizeof(eff));
    index += sizeof(eff);

    tmp = (unsigned int)(all_reduce_cnt-last_all_reduce_cnt);
    memcpy(&buffer[index], &tmp, sizeof(unsigned int));
    index += sizeof(unsigned int);

    tmp = (unsigned int)(s->s_nevent_abort-last_stats.s_nevent_abort);
    memcpy(&buffer[index], &tmp, sizeof(unsigned int));
    index += sizeof(unsigned int);

    tmp = (unsigned int)(s->s_pe_event_ties-last_stats.s_pe_event_ties);
    memcpy(&buffer[index], &tmp, sizeof(unsigned int));
    index += sizeof(unsigned int);

    tmp = (unsigned int)(s->s_fc_attempts-last_stats.s_fc_attempts);
    memcpy(&buffer[index], &tmp, sizeof(unsigned int));
    index += sizeof(unsigned int);

    tmp2 = (int)((long long)s->s_net_events-(long long)last_stats.s_net_events);
    memcpy(&buffer[index], &tmp2, sizeof(int));
    index += sizeof(int);

    eff = (float)100.0 * (1.0 - ((float) (s->s_e_rbs-last_stats.s_e_rbs)/(float) (s->s_net_events-last_stats.s_net_events)));
    memcpy(&buffer[index], &eff, sizeof(float));
    index += sizeof(float);

    /* KP granularity */
    for (i = 0; i < g_tw_nkp; i++)
    {
        kp = tw_getkp(i);

        tmp = (unsigned int)(kp->s_rb_total - kp->last_s_rb_total_gvt);
        memcpy(&buffer[index], &tmp, sizeof(unsigned int));
        index += sizeof(unsigned int);
        kp->last_s_rb_total_gvt = kp->s_rb_total;

        tmp = (unsigned int)(kp->s_rb_secondary - kp->last_s_rb_secondary_gvt);
        memcpy(&buffer[index], &tmp, sizeof(unsigned int));
        index += sizeof(unsigned int);
        kp->last_s_rb_secondary_gvt = kp->s_rb_secondary;
    }

    /* LP granularity */
    for (i=0; i < g_tw_nlp; i++)
    {
        lp = tw_getlp(i);

        tmp = (unsigned int)(lp->event_counters->s_nevent_processed - lp->prev_event_counters_gvt->s_nevent_processed);
        memcpy(&buffer[index], &tmp, sizeof(unsigned int));
        index += sizeof(unsigned int);
        lp->prev_event_counters_gvt->s_nevent_processed = lp->event_counters->s_nevent_processed;

        tmp = (unsigned int)(lp->event_counters->s_e_rbs - lp->prev_event_counters_gvt->s_e_rbs);
        memcpy(&buffer[index], &tmp, sizeof(unsigned int));
        index += sizeof(unsigned int);
        lp->prev_event_counters_gvt->s_e_rbs = lp->event_counters->s_e_rbs;

        tmp = (unsigned int)(lp->event_counters->s_nsend_network - lp->prev_event_counters_gvt->s_nsend_network);
        memcpy(&buffer[index], &tmp, sizeof(unsigned int));
        index += sizeof(unsigned int);
        lp->prev_event_counters_gvt->s_nsend_network = lp->event_counters->s_nsend_network;

        tmp = (unsigned int)(lp->event_counters->s_nread_network - lp->prev_event_counters_gvt->s_nread_network);
        memcpy(&buffer[index], &tmp, sizeof(unsigned int));
        index += sizeof(unsigned int);
        lp->prev_event_counters_gvt->s_nread_network = lp->event_counters->s_nread_network;

        // next stat not guaranteed to always be non-decreasing
        // can't use tw_stat (unsigned long long) for negative number
        tmp2 = (int)lp->event_counters->s_nsend_net_remote - (int)lp->prev_event_counters_gvt->s_nsend_net_remote;
        memcpy(&buffer[index], &tmp2, sizeof(int));
        index += sizeof(int);
        lp->prev_event_counters_gvt->s_nsend_net_remote = lp->event_counters->s_nsend_net_remote;
    }

    if (index != buf_size)
        tw_error(TW_LOC, "size of data being pushed to buffer is incorrect!\n");

    st_buffer_push(g_st_buffer_gvt, &buffer[0], buf_size);
    memcpy(&last_stats, s, sizeof(tw_statistics));
    last_all_reduce_cnt = all_reduce_cnt;
}

void st_collect_data(tw_pe *pe, tw_stime current_rt)
{
    tw_clock start_cycle_time = tw_clock_read();
    int index = 0;
    int total_size = sizeof(unsigned short) + sizeof(float)*2;
    int data_size = g_tw_nkp * sizeof(float);
    char data_gvt[data_size];
    unsigned short id = (unsigned short)pe->id;
    float ts;

    st_collect_time_ahead_GVT(pe, &data_gvt[0]);
    total_size += data_size;

    data_size = num_cycle_ctrs * sizeof(float);
    char data_cycles[data_size];
    st_collect_cycle_counters(pe, &data_cycles[0]);
    total_size += data_size;

    if (g_st_granularity == 0)
        data_size = sizeof(st_event_counters);
    else
        data_size = sizeof(unsigned int) * num_ev_ctrs_pe + sizeof(unsigned int) * num_ev_ctrs_kp * g_tw_nkp + (sizeof(unsigned int) * num_ev_ctrs_lp + sizeof(int)) * g_tw_nlp;

    char data_events[data_size];
    if (g_st_granularity == 0)
        st_collect_event_counters_pes(pe, &data_events[0]);
    else
        st_collect_event_counters_lps(pe, &data_events[0]);
    total_size += data_size;

    char final_data[total_size];
    memcpy(&final_data[index], &id, sizeof(id));
    index += sizeof(id);
    ts = (float) current_rt;
    memcpy(&final_data[index], &ts, sizeof(ts));
    index += sizeof(ts);
    ts = (float)pe->GVT;
    memcpy(&final_data[index], &ts, sizeof(ts));
    index += sizeof(ts);
    memcpy(&final_data[index], &data_gvt[0], sizeof(data_gvt));
    index += sizeof(data_gvt);
    memcpy(&final_data[index], &data_cycles[0], sizeof(data_cycles));
    index += sizeof(data_cycles);
    memcpy(&final_data[index], &data_events[0], sizeof(data_events));
    index += sizeof(data_events);
    st_buffer_push(g_st_buffer_rt, final_data, total_size);
    g_st_stat_comp_ctr += tw_clock_read() - start_cycle_time;
}

void st_collect_time_ahead_GVT(tw_pe *pe, char *data)
{
    tw_kp *kp;
    tw_lp *lp;
    float time;
    int i, index = 0;

    for(i = 0; i < g_tw_nkp; i++)
    {
        kp = tw_getkp(i);
        if (pe->GVT <= g_tw_ts_end)
            time = (float)(kp->last_time - pe->GVT);
        else
            time = -1.0; // so we can filter out
        memcpy(&data[index], &time, sizeof(time));
        index += sizeof(time);
    }
}

void st_collect_cycle_counters(tw_pe *pe, char *data)
{
    int index = 0;
    float tmp;

    tmp = (float)(pe->stats.s_net_read - last_cycle_counters.s_net_read) / g_tw_clock_rate;
    memcpy(&data[index], &tmp, sizeof(float));
    index += sizeof(float);

    tmp = (float)(pe->stats.s_gvt - last_cycle_counters.s_gvt) / g_tw_clock_rate;
    memcpy(&data[index], &tmp, sizeof(float));
    index += sizeof(float);

    tmp = (float)(pe->stats.s_fossil_collect - last_cycle_counters.s_fossil_collect) / g_tw_clock_rate;
    memcpy(&data[index], &tmp, sizeof(float));
    index += sizeof(float);

    tmp = (float)(pe->stats.s_event_abort - last_cycle_counters.s_event_abort) / g_tw_clock_rate;
    memcpy(&data[index], &tmp, sizeof(float));
    index += sizeof(float);

    tmp = (float)(pe->stats.s_event_process - last_cycle_counters.s_event_process) / g_tw_clock_rate;
    memcpy(&data[index], &tmp, sizeof(float));
    index += sizeof(float);

    tmp = (float)(pe->stats.s_pq - last_cycle_counters.s_pq) / g_tw_clock_rate;
    memcpy(&data[index], &tmp, sizeof(float));
    index += sizeof(float);

    tmp = (float)(pe->stats.s_rollback - last_cycle_counters.s_rollback) / g_tw_clock_rate;
    memcpy(&data[index], &tmp, sizeof(float));
    index += sizeof(float);

    tmp = (float)(pe->stats.s_cancel_q - last_cycle_counters.s_cancel_q) / g_tw_clock_rate;
    memcpy(&data[index], &tmp, sizeof(float));
    index += sizeof(float);

    tmp = (float)(pe->stats.s_avl - last_cycle_counters.s_avl) / g_tw_clock_rate;
    memcpy(&data[index], &tmp, sizeof(float));
    index += sizeof(float);

    tmp = (float)(pe->stats.s_buddy - last_cycle_counters.s_buddy) / g_tw_clock_rate;
    memcpy(&data[index], &tmp, sizeof(float));
    index += sizeof(float);

    tmp = (float)(pe->stats.s_lz4 - last_cycle_counters.s_lz4) / g_tw_clock_rate;
    memcpy(&data[index], &tmp, sizeof(float));

    last_cycle_counters.s_net_read = pe->stats.s_net_read;
    last_cycle_counters.s_gvt = pe->stats.s_gvt;
    last_cycle_counters.s_fossil_collect = pe->stats.s_fossil_collect;
    last_cycle_counters.s_event_abort = pe->stats.s_event_abort;
    last_cycle_counters.s_event_process = pe->stats.s_event_process;
    last_cycle_counters.s_pq = pe->stats.s_pq;
    last_cycle_counters.s_rollback = pe->stats.s_rollback;
    last_cycle_counters.s_cancel_q = pe->stats.s_cancel_q;
    last_cycle_counters.s_avl = pe->stats.s_avl;
    last_cycle_counters.s_buddy = pe->stats.s_buddy;
    last_cycle_counters.s_lz4 = pe->stats.s_lz4;
}

void st_collect_event_counters_pes(tw_pe *pe, char *data)
{
    int i, index = 0;
    tw_kp *kp;
    tw_stat tmp;

    tmp = (unsigned int)(pe->stats.s_nevent_abort - last_event_counters.s_nevent_abort);
    memcpy(&data[index], &tmp, sizeof(unsigned int));
    index += sizeof(unsigned int);

    tmp = (unsigned int)tw_pq_get_size(pe->pq);
    memcpy(&data[index], &tmp, sizeof(unsigned int));
    index += sizeof(unsigned int);

    tmp = (unsigned int)(pe->stats.s_nsend_net_remote - last_event_counters.s_nsend_net_remote);
    memcpy(&data[index], &tmp, sizeof(unsigned int));
    index += sizeof(unsigned int);

    tmp = (unsigned int)(pe->stats.s_nsend_network - last_event_counters.s_nsend_network);
    memcpy(&data[index], &tmp, sizeof(unsigned int));
    index += sizeof(unsigned int);

    tmp = (unsigned int)(pe->stats.s_nread_network - last_event_counters.s_nread_network);
    memcpy(&data[index], &tmp, sizeof(unsigned int));
    index += sizeof(unsigned int);

    tmp = (unsigned int)(pe->stats.s_pe_event_ties - last_event_counters.s_pe_event_ties);
    memcpy(&data[index], &tmp, sizeof(unsigned int));
    index += sizeof(unsigned int);

    tmp = (unsigned int)(g_tw_fossil_attempts - last_event_counters.s_fc_attempts);
    memcpy(&data[index], &tmp, sizeof(unsigned int));
    index += sizeof(unsigned int);

    tmp = (unsigned int)(g_tw_gvt_done - last_event_counters.s_ngvts);
    memcpy(&data[index], &tmp, sizeof(unsigned int));
    index += sizeof(unsigned int);

    unsigned int t[4] = {0};
    for(i = 0; i < g_tw_nkp; i++)
    {
        kp = tw_getkp(i);
        t[0] += kp->s_nevent_processed;
        t[1] += kp->s_e_rbs;
        t[2] += kp->s_rb_total;
        t[3] += kp->s_rb_secondary;
    }

    tmp = t[0] - (unsigned int)last_event_counters.s_nevent_processed;
    memcpy(&data[index], &tmp, sizeof(unsigned int));
    index += sizeof(unsigned int);
    tmp = t[1] - (unsigned int)last_event_counters.s_e_rbs;
    memcpy(&data[index], &tmp, sizeof(unsigned int));
    index += sizeof(unsigned int);
    tmp = t[2] - (unsigned int)last_event_counters.s_rb_total;
    memcpy(&data[index], &tmp, sizeof(unsigned int));
    index += sizeof(unsigned int);
    tmp = t[3] - (unsigned int)last_event_counters.s_rb_secondary;
    memcpy(&data[index], &tmp, sizeof(unsigned int));
    index += sizeof(unsigned int);

    last_event_counters.s_nevent_abort = pe->stats.s_nevent_abort;
    last_event_counters.s_fc_attempts = g_tw_fossil_attempts;
    last_event_counters.s_pq_qsize = tw_pq_get_size(pe->pq);
    last_event_counters.s_nsend_network = pe->stats.s_nsend_network;
    last_event_counters.s_nread_network = pe->stats.s_nread_network;
    last_event_counters.s_nsend_net_remote = pe->stats.s_nsend_net_remote;
    last_event_counters.s_pe_event_ties = pe->stats.s_pe_event_ties;
    last_event_counters.s_ngvts = g_tw_gvt_done;
    last_event_counters.s_nevent_processed = t[0];
    last_event_counters.s_e_rbs = t[1];
    last_event_counters.s_rb_total = t[2];
    last_event_counters.s_rb_secondary = t[3];
}

void st_collect_event_counters_lps(tw_pe *pe, char *data)
{
    int i, index = 0;
    tw_kp *kp;
    tw_lp *lp;
    unsigned int tmp;
    int tmp2;

    /* PE granularity */
    tmp = (unsigned int)(pe->stats.s_nevent_abort - last_event_counters.s_nevent_abort);
    memcpy(&data[index], &tmp, sizeof(unsigned int));
    index += sizeof(unsigned int);
    last_event_counters.s_nevent_abort = pe->stats.s_nevent_abort;

    tmp = (unsigned int)tw_pq_get_size(pe->pq);
    memcpy(&data[index], &tmp, sizeof(unsigned int));
    index += sizeof(unsigned int);
    last_event_counters.s_pq_qsize = tw_pq_get_size(pe->pq);

    tmp = (unsigned int)(pe->stats.s_pe_event_ties - last_event_counters.s_pe_event_ties);
    memcpy(&data[index], &tmp, sizeof(unsigned int));
    index += sizeof(unsigned int);
    last_event_counters.s_pe_event_ties = pe->stats.s_pe_event_ties;

    tmp = (unsigned int)(g_tw_fossil_attempts - last_event_counters.s_fc_attempts);
    memcpy(&data[index], &tmp, sizeof(unsigned int));
    index += sizeof(unsigned int);
    last_event_counters.s_fc_attempts = g_tw_fossil_attempts;

    tmp = (unsigned int)(g_tw_gvt_done - last_event_counters.s_ngvts);
    memcpy(&data[index], &tmp, sizeof(unsigned int));
    index += sizeof(unsigned int);
    last_event_counters.s_ngvts = g_tw_gvt_done;

    /* KP granularity */
    for(i = 0; i < g_tw_nkp; i++)
    {
        kp = tw_getkp(i);

        tmp = (unsigned int)(kp->s_rb_total - kp->last_s_rb_total_rt);
        memcpy(&data[index], &tmp, sizeof(unsigned int));
        index += sizeof(unsigned int);
        kp->last_s_rb_total_rt = kp->s_rb_total;

        tmp = (unsigned int)(kp->s_rb_secondary - kp->last_s_rb_secondary_rt);
        memcpy(&data[index], &tmp, sizeof(unsigned int));
        index += sizeof(unsigned int);
        kp->last_s_rb_secondary_rt = kp->s_rb_secondary;
    }


    /* LP granularity */
    for (i=0; i < g_tw_nlp; i++)
    {
        lp = tw_getlp(i);

        tmp = (unsigned int)(lp->event_counters->s_nevent_processed - lp->prev_event_counters_rt->s_nevent_processed);
        memcpy(&data[index], &tmp, sizeof(unsigned int));
        index += sizeof(unsigned int);
        lp->prev_event_counters_rt->s_nevent_processed = lp->event_counters->s_nevent_processed;

        tmp = (unsigned int)(lp->event_counters->s_e_rbs - lp->prev_event_counters_rt->s_e_rbs);
        memcpy(&data[index], &tmp, sizeof(unsigned int));
        index += sizeof(unsigned int);
        lp->prev_event_counters_rt->s_e_rbs = lp->event_counters->s_e_rbs;

        tmp = (unsigned int)(lp->event_counters->s_nsend_network - lp->prev_event_counters_rt->s_nsend_network);
        memcpy(&data[index], &tmp, sizeof(unsigned int));
        index += sizeof(unsigned int);
        lp->prev_event_counters_rt->s_nsend_network = lp->event_counters->s_nsend_network;

        tmp = (unsigned int)(lp->event_counters->s_nread_network - lp->prev_event_counters_rt->s_nread_network);
        memcpy(&data[index], &tmp, sizeof(unsigned int));
        index += sizeof(unsigned int);
        lp->prev_event_counters_rt->s_nread_network = lp->event_counters->s_nread_network;

        // next stat not guaranteed to always be non-decreasing
        // can't use unsigned int (unsigned long long) for negative number
        tmp2 = (int)lp->event_counters->s_nsend_net_remote - (int)lp->prev_event_counters_rt->s_nsend_net_remote;
        memcpy(&data[index], &tmp2, sizeof(int));
        index += sizeof(int);
        lp->prev_event_counters_rt->s_nsend_net_remote = lp->event_counters->s_nsend_net_remote;
    }
}


