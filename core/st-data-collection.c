#include <ross.h>
#include <sys/stat.h>
#include <inttypes.h>

char g_st_stats_out[128] = {0};
int g_st_stats_enabled = 0;
long g_st_time_interval = 0;
long g_st_current_interval = 0;
tw_clock g_st_real_time_samp = 0;
tw_clock g_st_real_samp_start_cycles = 0;
int g_st_pe_per_file = 1;
int g_st_my_file_id = 0;
tw_stat_list *g_st_stat_head = NULL;
tw_stat_list *g_st_stat_tail = NULL;
MPI_File gvt_file;
MPI_File interval_file;
static tw_statistics last_stats = {0};
static tw_stat last_all_reduce_cnt = 0;
int g_st_disable_out = 0;
st_cycle_counters last_cycle_counters = {0};
st_event_counters last_event_counters = {0};
st_mem_usage last_mem_usage = {0};
int g_st_granularity = 0;


static const tw_optdef stats_options[] = {
    TWOPT_GROUP("ROSS Stats"),
    TWOPT_UINT("enable-gvt-stats", g_st_stats_enabled, "Collect data after each GVT; 0 no stats, 1 for stats"),
    TWOPT_UINT("time-interval", g_st_time_interval, "collect stats for specified sim time interval"),
    TWOPT_ULONGLONG("real-time-samp", g_st_real_time_samp, "real time sampling interval in ms"),
    TWOPT_UINT("granularity", g_st_granularity, "collect on PE basis only, or also KP/LP basis, 0 for PE, 1 for KP/LP"),
    TWOPT_UINT("event-collect", g_st_ev_collect, "collect detailed data on all events for specified LPs; 1 for collection"),
    TWOPT_UINT("event-rb-collect", g_st_ev_rb_collect, "collect detailed data on events that cause rollbacks; 1 for collection"),
    TWOPT_CHAR("stats-filename", g_st_stats_out, "prefix for filename(s) for stats output"),
    TWOPT_UINT("pe-per-file", g_st_pe_per_file, "how many PEs to output per file"),
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
    int i;
    int npe = tw_nnodes();
    tw_lpid nlp_per_pe[npe];
    MPI_Gather(&g_tw_nlp, 1, MPI_UINT64_T, nlp_per_pe, 1, MPI_UINT64_T, 0, MPI_COMM_WORLD);
    // Need to call after st_buffer_init()!
    if (!g_st_disable_out && g_tw_mynode == 0) {
        FILE *file;
        char filename[128];
        sprintf(filename, "%s/%s-README.txt", g_st_directory, g_st_stats_out);
        file = fopen(filename, "w");

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
        fprintf(file, "LPs per PE\n");
        for (i = 0; i < npe; i++)
        {
            if (i == npe - 1)
                fprintf(file, "%"PRIu64"\n", nlp_per_pe[i]);
            else
                fprintf(file, "%"PRIu64",", nlp_per_pe[i]);
        }
        fclose(file);
    }
}

/** write header line to stats output files */
/*
 *void tw_gvt_stats_file_setup(tw_peid id)
 *{
 *    int max_files_directory = 100;
 *    char directory_path[128];
 *    char filename[256];
 *    if (g_st_stats_out[0])
 *    {
 *        sprintf(directory_path, "%s-gvt-%d", g_st_stats_out, g_st_my_file_id/max_files_directory);
 *        mkdir(directory_path, S_IRUSR | S_IWUSR | S_IXUSR);
 *        sprintf(filename, "%s/%s-%d-gvt.txt", directory_path, g_st_stats_out, g_st_my_file_id);
 *    }
 *    else
 *    {
 *        sprintf(directory_path, "ross-gvt-%d", g_st_my_file_id/max_files_directory);
 *        mkdir(directory_path, S_IRUSR | S_IWUSR | S_IXUSR);
 *        sprintf( filename, "%s/ross-gvt-stats-%d.txt", directory_path, g_st_my_file_id);
 *    }
 *
 *    MPI_File_open(stats_comm, filename, MPI_MODE_CREATE | MPI_MODE_WRONLY, MPI_INFO_NULL, &gvt_file);
 *
 *    char buffer[1024];
 *    sprintf(buffer, "PE,GVT,Total All Reduce Calls,"
 *        "total events processed,events aborted,events rolled back,event ties detected in PE queues,"
 *        "efficiency,total remote network events processed,"
 *        "total rollbacks,primary rollbacks,secondary roll backs,fossil collect attempts,"
 *        "net events processed,remote sends,remote recvs\n");
 *    MPI_File_write(gvt_file, buffer, strlen(buffer), MPI_CHAR, MPI_STATUS_IGNORE);
 *}
 */

/** write header line to stats output files */
void tw_interval_stats_file_setup(tw_peid id)
{
/*    tw_stat forward_events;
    tw_stat reverse_events;
    tw_stat num_gvts;
    tw_stat all_reduce_calls;
    tw_stat events_aborted;
    tw_stat pe_event_ties;

    tw_stat nevents_remote;
    tw_stat nsend_network;
    tw_stat nread_network;

    tw_stat events_rbs;
    tw_stat rb_primary;
    tw_stat rb_secondary;
    tw_stat fc_attempts;
    */
    int max_files_directory = 100;
    char directory_path[128];
    char filename[256];
    if (g_st_stats_out[0])
    {
        sprintf(directory_path, "%s-interval-%d", g_st_stats_out, g_st_my_file_id/max_files_directory);
        mkdir(directory_path, S_IRUSR | S_IWUSR | S_IXUSR);
        sprintf(filename, "%s/%s-%d-interval.txt", directory_path, g_st_stats_out, g_st_my_file_id);
    }
    else
    {
        sprintf(directory_path, "ross-interval-%d", g_st_my_file_id/max_files_directory);
        mkdir(directory_path, S_IRUSR | S_IWUSR | S_IXUSR);
        sprintf( filename, "%s/ross-interval-stats-%d.txt", directory_path, g_st_my_file_id);
    }

    MPI_File_open(stats_comm, filename, MPI_MODE_CREATE | MPI_MODE_WRONLY, MPI_INFO_NULL, &interval_file);

    char buffer[1024];
	sprintf(buffer, "PE,interval,forward events,reverse events,number of GVT comps,all reduce calls,"
        "events aborted,event ties detected in PE queues,remote events,network sends,network recvs,"
        "events rolled back,primary rollbacks,secondary roll backs,fossil collect attempts\n");
    MPI_File_write(interval_file, buffer, strlen(buffer), MPI_CHAR, MPI_STATUS_IGNORE);
}

/* wrapper to call gvt log functions depending on which granularity to use */
void tw_gvt_log(FILE * f, tw_pe *me, tw_stime gvt, tw_statistics *s, tw_stat all_reduce_cnt)
{
    if (g_st_granularity == 0)
        st_gvt_log_pes(f, me, gvt, s, all_reduce_cnt);
    else
        st_gvt_log_lps(f, me, gvt, s, all_reduce_cnt);

}

void st_gvt_log_pes(FILE * f, tw_pe *me, tw_stime gvt, tw_statistics *s, tw_stat all_reduce_cnt)
{
	// GVT,Forced GVT, Total GVT Computations, Total All Reduce Calls, Average Reduction / GVT
    // total events processed, events aborted, events rolled back, event ties detected in PE queues
    // efficiency, total remote network events processed, percent remote events
    // total rollbacks, primary rollbacks, secondary roll backs, fossil collect attempts
    // net events processed, remote sends, remote recvs
    tw_clock start_cycle_time = tw_clock_read();
    int buf_size = sizeof(tw_stat) * 11 + sizeof(tw_node) + sizeof(tw_stime) + sizeof(double) + sizeof(long long) *2;
    int index = 0;
    char buffer[buf_size];
    tw_stat tmp;
    long long tmp2;
    double eff;

    memcpy(&buffer[index], &g_tw_mynode, sizeof(tw_node));
    index += sizeof(tw_node);

    memcpy(&buffer[index], &gvt, sizeof(tw_stime));
    index += sizeof(tw_stime);

    tmp = all_reduce_cnt-last_all_reduce_cnt;
    memcpy(&buffer[index], &tmp, sizeof(tw_stat));
    index += sizeof(tw_stat);

    tmp = s->s_nevent_processed-last_stats.s_nevent_processed;
    memcpy(&buffer[index], &tmp, sizeof(tw_stat));
    index += sizeof(tw_stat);

    tmp = s->s_nevent_abort-last_stats.s_nevent_abort;
    memcpy(&buffer[index], &tmp, sizeof(tw_stat));
    index += sizeof(tw_stat);

    tmp = s->s_e_rbs-last_stats.s_e_rbs;
    memcpy(&buffer[index], &tmp, sizeof(tw_stat));
    index += sizeof(tw_stat);

    tmp = s->s_pe_event_ties-last_stats.s_pe_event_ties;
    memcpy(&buffer[index], &tmp, sizeof(tw_stat));
    index += sizeof(tw_stat);

    tmp = s->s_rb_total-last_stats.s_rb_total;
    memcpy(&buffer[index], &tmp, sizeof(tw_stat));
    index += sizeof(tw_stat);

    tmp = s->s_rb_primary-last_stats.s_rb_primary;
    memcpy(&buffer[index], &tmp, sizeof(tw_stat));
    index += sizeof(tw_stat);

    tmp = s->s_rb_secondary-last_stats.s_rb_secondary;
    memcpy(&buffer[index], &tmp, sizeof(tw_stat));
    index += sizeof(tw_stat);

    tmp = s->s_fc_attempts-last_stats.s_fc_attempts;
    memcpy(&buffer[index], &tmp, sizeof(tw_stat));
    index += sizeof(tw_stat);

    tmp = s->s_nsend_network-last_stats.s_nsend_network;
    memcpy(&buffer[index], &tmp, sizeof(tw_stat));
    index += sizeof(tw_stat);

    tmp = s->s_nread_network-last_stats.s_nread_network;
    memcpy(&buffer[index], &tmp, sizeof(tw_stat));
    index += sizeof(tw_stat);

    // next two stats not guaranteed to always be non-decreasing
    // can't use tw_stat (unsigned long long) for negative number
    tmp2 = (long long)s->s_nsend_net_remote-(long long)last_stats.s_nsend_net_remote;
    memcpy(&buffer[index], &tmp2, sizeof(long long));
    index += sizeof(long long);

    tmp2 = (long long)s->s_net_events-(long long)last_stats.s_net_events;
    memcpy(&buffer[index], &tmp2, sizeof(long long));
    index += sizeof(long long);

    eff = 100.0 * (1.0 - ((double) (s->s_e_rbs-last_stats.s_e_rbs)/(double) (s->s_net_events-last_stats.s_net_events)));
    memcpy(&buffer[index], &eff, sizeof(double));
    index += sizeof(double);

    if (index != buf_size)
        printf("WARNING: size of data being pushed to buffer is incorrect!");

    st_buffer_push(g_st_buffer_gvt, &buffer[0], buf_size);
    //stat_comp_cycle_counter += tw_clock_read() - start_cycle_time;
    //start_cycle_time = tw_clock_read();
    //MPI_File_write(gvt_file, buffer, strlen(buffer), MPI_CHAR, MPI_STATUS_IGNORE);
    //stat_write_cycle_counter += tw_clock_read() - start_cycle_time;
    //start_cycle_time = tw_clock_read();
    memcpy(&last_stats, s, sizeof(tw_statistics));
    last_all_reduce_cnt = all_reduce_cnt;
    stat_comp_cycle_counter += tw_clock_read() - start_cycle_time;
}

void st_gvt_log_lps(FILE * f, tw_pe *me, tw_stime gvt, tw_statistics *s, tw_stat all_reduce_cnt)
{
	// GVT,Forced GVT, Total GVT Computations, Total All Reduce Calls, Average Reduction / GVT
    // total events processed, events aborted, events rolled back, event ties detected in PE queues
    // efficiency, total remote network events processed, percent remote events
    // total rollbacks, primary rollbacks, secondary roll backs, fossil collect attempts
    // net events processed, remote sends, remote recvs
    tw_clock start_cycle_time = tw_clock_read();
    int buf_size = sizeof(tw_node) + sizeof(tw_stime) + sizeof(tw_stat) * 4 + sizeof(double) + sizeof(long long) +
        sizeof(tw_stat) * 2 * g_tw_nkp + sizeof(tw_stat) * 4 * g_tw_nlp + sizeof(long long) * g_tw_nlp;
    int index = 0, i;
    char buffer[buf_size];
    tw_stat tmp;
    long long tmp2;
    double eff;
    tw_kp *kp;
    tw_lp *lp;

    /* PE granularity */
    memcpy(&buffer[index], &g_tw_mynode, sizeof(tw_node));
    index += sizeof(tw_node);

    memcpy(&buffer[index], &gvt, sizeof(tw_stime));
    index += sizeof(tw_stime);

    tmp = all_reduce_cnt-last_all_reduce_cnt;
    memcpy(&buffer[index], &tmp, sizeof(tw_stat));
    index += sizeof(tw_stat);

    tmp = s->s_nevent_abort-last_stats.s_nevent_abort;
    memcpy(&buffer[index], &tmp, sizeof(tw_stat));
    index += sizeof(tw_stat);

    tmp = s->s_pe_event_ties-last_stats.s_pe_event_ties;
    memcpy(&buffer[index], &tmp, sizeof(tw_stat));
    index += sizeof(tw_stat);

    tmp = s->s_fc_attempts-last_stats.s_fc_attempts;
    memcpy(&buffer[index], &tmp, sizeof(tw_stat));
    index += sizeof(tw_stat);

    // next two stats not guaranteed to always be non-decreasing
    // can't use tw_stat (unsigned long long) for negative number
    tmp2 = (long long)s->s_net_events-(long long)last_stats.s_net_events;
    memcpy(&buffer[index], &tmp2, sizeof(long long));
    index += sizeof(long long);

    eff = 100.0 * (1.0 - ((double) (s->s_e_rbs-last_stats.s_e_rbs)/(double) (s->s_net_events-last_stats.s_net_events)));
    memcpy(&buffer[index], &eff, sizeof(double));
    index += sizeof(double);

    /* KP granularity */
    for (i = 0; i < g_tw_nkp; i++)
    {
        kp = tw_getkp(i);

        tmp = kp->s_rb_total - kp->last_s_rb_total_gvt;
        memcpy(&buffer[index], &tmp, sizeof(tw_stat));
        index += sizeof(tw_stat);
        kp->last_s_rb_total_gvt = kp->s_rb_total;

        tmp = kp->s_rb_secondary - kp->last_s_rb_secondary_gvt;
        memcpy(&buffer[index], &tmp, sizeof(tw_stat));
        index += sizeof(tw_stat);
        kp->last_s_rb_secondary_gvt = kp->s_rb_secondary;
    }

    /* LP granularity */
    for (i=0; i < g_tw_nlp; i++)
    {
        lp = tw_getlp(i);

        tmp = lp->event_counters->s_nevent_processed - lp->prev_event_counters_gvt->s_nevent_processed;
        memcpy(&buffer[index], &tmp, sizeof(tw_stat));
        index += sizeof(tw_stat);
        lp->prev_event_counters_gvt->s_nevent_processed = lp->event_counters->s_nevent_processed;

        tmp = lp->event_counters->s_e_rbs - lp->prev_event_counters_gvt->s_e_rbs;
        memcpy(&buffer[index], &tmp, sizeof(tw_stat));
        index += sizeof(tw_stat);
        lp->prev_event_counters_gvt->s_e_rbs = lp->event_counters->s_e_rbs;

        tmp = lp->event_counters->s_nsend_network - lp->prev_event_counters_gvt->s_nsend_network;
        memcpy(&buffer[index], &tmp, sizeof(tw_stat));
        index += sizeof(tw_stat);
        lp->prev_event_counters_gvt->s_nsend_network = lp->event_counters->s_nsend_network;

        tmp = lp->event_counters->s_nread_network - lp->prev_event_counters_gvt->s_nread_network;
        memcpy(&buffer[index], &tmp, sizeof(tw_stat));
        index += sizeof(tw_stat);
        lp->prev_event_counters_gvt->s_nread_network = lp->event_counters->s_nread_network;

    }

    for (i=0; i < g_tw_nlp; i++)
    {
        lp = tw_getlp(i);
        // next stat not guaranteed to always be non-decreasing
        // can't use tw_stat (unsigned long long) for negative number
        tmp2 = (long long)lp->event_counters->s_nsend_net_remote - (long long)lp->prev_event_counters_gvt->s_nsend_net_remote;
        memcpy(&buffer[index], &tmp2, sizeof(long long));
        index += sizeof(long long);
        lp->prev_event_counters_gvt->s_nsend_net_remote = lp->event_counters->s_nsend_net_remote;
    }

    if (index != buf_size)
        printf("WARNING: size of data being pushed to buffer is incorrect!");

    st_buffer_push(g_st_buffer_gvt, &buffer[0], buf_size);
    //stat_comp_cycle_counter += tw_clock_read() - start_cycle_time;
    //start_cycle_time = tw_clock_read();
    //MPI_File_write(gvt_file, buffer, strlen(buffer), MPI_CHAR, MPI_STATUS_IGNORE);
    //stat_write_cycle_counter += tw_clock_read() - start_cycle_time;
    //start_cycle_time = tw_clock_read();
    memcpy(&last_stats, s, sizeof(tw_statistics));
    last_all_reduce_cnt = all_reduce_cnt;
    stat_comp_cycle_counter += tw_clock_read() - start_cycle_time;
}

void st_collect_data(tw_pe *pe, tw_stime current_rt)
{
    int index = 0;
    int total_size = sizeof(tw_peid) + sizeof(tw_stime)*2;
    int data_size = (g_tw_nkp/* + g_tw_nlp*/) * sizeof(tw_stime);
    char data_gvt[data_size];

    st_collect_time_ahead_GVT(pe, &data_gvt[0]);
    total_size += data_size;

    data_size = 11 * sizeof(tw_clock);
    char data_cycles[data_size];
    st_collect_cycle_counters(pe, &data_cycles[0]);
    total_size += data_size;

    if (g_st_granularity == 0)
        data_size = sizeof(st_event_counters);
    else
        data_size = sizeof(tw_stat) * 5 + sizeof(tw_stat) * 2 * g_tw_nkp + (sizeof(tw_stat) * 4 + sizeof(long long)) * g_tw_nlp;

    char data_events[data_size];
    if (g_st_granularity == 0)
        st_collect_event_counters_pes(pe, &data_events[0]);
    else
        st_collect_event_counters_lps(pe, &data_events[0]);
    total_size += data_size;

    data_size = sizeof(size_t) * 2;
    char data_mem[data_size];
    st_collect_memory_usage(&data_mem[0]);
    total_size += data_size;

    char final_data[total_size];
    memcpy(&final_data[index], &pe->id, sizeof(tw_peid));
    index += sizeof(tw_peid);
    memcpy(&final_data[index], &current_rt, sizeof(tw_stime));
    index += sizeof(tw_stime);
    memcpy(&final_data[index], &pe->GVT, sizeof(tw_stime));
    index += sizeof(tw_stime);
    memcpy(&final_data[index], &data_gvt[0], sizeof(data_gvt));
    index += sizeof(data_gvt);
    memcpy(&final_data[index], &data_cycles[0], sizeof(data_cycles));
    index += sizeof(data_cycles);
    memcpy(&final_data[index], &data_events[0], sizeof(data_events));
    index += sizeof(data_events);
    memcpy(&final_data[index], &data_mem[0], sizeof(data_mem));
    index += sizeof(data_mem);
    st_buffer_push(g_st_buffer_rt, final_data, total_size);
}

void st_collect_time_ahead_GVT(tw_pe *pe, char *data)
{
    tw_kp *kp;
    tw_lp *lp;
    tw_stime time;
    int i, index = 0;

    for(i = 0; i < g_tw_nkp; i++)
    {
        kp = tw_getkp(i);
        if (pe->GVT <= g_tw_ts_end)
            time = kp->last_time - pe->GVT;
        else
            time = -1.0; // so we can filter out
        memcpy(&data[index], &time, sizeof(tw_stime));
        index += sizeof(tw_stime);
    }
    /*for(i = 0; i < g_tw_nlp; i++)
    {
        lp = tw_getlp(i);
        time = lp->last_time - pe->GVT;
        memcpy(&data[index], &time, sizeof(tw_stime));
        index += sizeof(tw_stime);

    }*/
}

void st_collect_cycle_counters(tw_pe *pe, char *data)
{
    // TODO divide by clock rate before moving to buffer?
    int index = 0;
    tw_clock tmp;

    tmp = pe->stats.s_net_read - last_cycle_counters.s_net_read;
    memcpy(&data[index], &tmp, sizeof(tw_clock));
    index += sizeof(tw_clock);

    tmp = pe->stats.s_gvt - last_cycle_counters.s_gvt;
    memcpy(&data[index], &tmp, sizeof(tw_clock));
    index += sizeof(tw_clock);

    tmp = pe->stats.s_fossil_collect - last_cycle_counters.s_fossil_collect;
    memcpy(&data[index], &tmp, sizeof(tw_clock));
    index += sizeof(tw_clock);

    tmp = pe->stats.s_event_abort - last_cycle_counters.s_event_abort;
    memcpy(&data[index], &tmp, sizeof(tw_clock));
    index += sizeof(tw_clock);

    tmp = pe->stats.s_event_process - last_cycle_counters.s_event_process;
    memcpy(&data[index], &tmp, sizeof(tw_clock));
    index += sizeof(tw_clock);

    tmp = pe->stats.s_pq - last_cycle_counters.s_pq;
    memcpy(&data[index], &tmp, sizeof(tw_clock));
    index += sizeof(tw_clock);

    tmp = pe->stats.s_rollback - last_cycle_counters.s_rollback;
    memcpy(&data[index], &tmp, sizeof(tw_clock));
    index += sizeof(tw_clock);

    tmp = pe->stats.s_cancel_q - last_cycle_counters.s_cancel_q;
    memcpy(&data[index], &tmp, sizeof(tw_clock));
    index += sizeof(tw_clock);

    tmp = pe->stats.s_avl - last_cycle_counters.s_avl;
    memcpy(&data[index], &tmp, sizeof(tw_clock));
    index += sizeof(tw_clock);

    tmp = pe->stats.s_buddy - last_cycle_counters.s_buddy;
    memcpy(&data[index], &tmp, sizeof(tw_clock));
    index += sizeof(tw_clock);

    tmp = pe->stats.s_lz4 - last_cycle_counters.s_lz4;
    memcpy(&data[index], &tmp, sizeof(tw_clock));

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

    tmp = pe->stats.s_nevent_abort - last_event_counters.s_nevent_abort;
    memcpy(&data[index], &tmp, sizeof(tw_stat));
    index += sizeof(tw_stat);

    tmp = tw_pq_get_size(pe->pq);// - last_event_counters.s_pq_qsize;
    memcpy(&data[index], &tmp, sizeof(tw_stat));
    index += sizeof(tw_stat);

    tmp = pe->stats.s_nsend_net_remote - last_event_counters.s_nsend_net_remote;
    memcpy(&data[index], &tmp, sizeof(tw_stat));
    index += sizeof(tw_stat);

    tmp = pe->stats.s_nsend_loc_remote - last_event_counters.s_nsend_loc_remote;
    memcpy(&data[index], &tmp, sizeof(tw_stat));
    index += sizeof(tw_stat);

    tmp = pe->stats.s_nsend_network - last_event_counters.s_nsend_network;
    memcpy(&data[index], &tmp, sizeof(tw_stat));
    index += sizeof(tw_stat);

    tmp = pe->stats.s_nread_network - last_event_counters.s_nread_network;
    memcpy(&data[index], &tmp, sizeof(tw_stat));
    index += sizeof(tw_stat);

    tmp = pe->stats.s_nsend_remote_rb - last_event_counters.s_nsend_remote_rb;
    memcpy(&data[index], &tmp, sizeof(tw_stat));
    index += sizeof(tw_stat);

    tmp = pe->stats.s_pe_event_ties - last_event_counters.s_pe_event_ties;
    memcpy(&data[index], &tmp, sizeof(tw_stat));
    index += sizeof(tw_stat);

    tmp = g_tw_fossil_attempts - last_event_counters.s_fc_attempts;
    memcpy(&data[index], &tmp, sizeof(tw_stat));
    index += sizeof(tw_stat);

    tmp = g_tw_gvt_done - last_event_counters.s_ngvts;
    memcpy(&data[index], &tmp, sizeof(tw_stat));
    index += sizeof(tw_stat);

    tw_stat t[4] = {0};
    for(i = 0; i < g_tw_nkp; i++)
    {
        kp = tw_getkp(i);
        t[0] += kp->s_nevent_processed;
        t[1] += kp->s_e_rbs;
        t[2] += kp->s_rb_total;
        t[3] += kp->s_rb_secondary;
    }

    tmp = t[0] - last_event_counters.s_nevent_processed;
    memcpy(&data[index], &tmp, sizeof(tw_stat));
    index += sizeof(tw_stat);
    tmp = t[1] - last_event_counters.s_e_rbs;
    memcpy(&data[index], &tmp, sizeof(tw_stat));
    index += sizeof(tw_stat);
    tmp = t[2] - last_event_counters.s_rb_total;
    memcpy(&data[index], &tmp, sizeof(tw_stat));
    index += sizeof(tw_stat);
    tmp = t[3] - last_event_counters.s_rb_secondary;
    memcpy(&data[index], &tmp, sizeof(tw_stat));
    index += sizeof(tw_stat);

	tmp = t[0] - t[1] - last_event_counters.s_net_events;
    memcpy(&data[index], &tmp, sizeof(tw_stat));
    index += sizeof(tw_stat);
	tmp = t[2] - t[3] - last_event_counters.s_rb_primary;
    memcpy(&data[index], &tmp, sizeof(tw_stat));
    index += sizeof(tw_stat);

    last_event_counters.s_nevent_abort = pe->stats.s_nevent_abort;
    last_event_counters.s_fc_attempts = g_tw_fossil_attempts;
    last_event_counters.s_pq_qsize = tw_pq_get_size(pe->pq);
    last_event_counters.s_nsend_network = pe->stats.s_nsend_network;
    last_event_counters.s_nread_network = pe->stats.s_nread_network;
    last_event_counters.s_nsend_remote_rb = pe->stats.s_nsend_remote_rb;
    last_event_counters.s_nsend_loc_remote = pe->stats.s_nsend_loc_remote;
    last_event_counters.s_nsend_net_remote = pe->stats.s_nsend_net_remote;
    last_event_counters.s_pe_event_ties = pe->stats.s_pe_event_ties;
    last_event_counters.s_ngvts = g_tw_gvt_done;
    last_event_counters.s_nevent_processed = t[0];
    last_event_counters.s_e_rbs = t[1];
    last_event_counters.s_rb_total = t[2];
    last_event_counters.s_rb_secondary = t[3];
    last_event_counters.s_net_events = t[0] - t[1];
    last_event_counters.s_rb_primary = t[2] - t[3];
}

void st_collect_event_counters_lps(tw_pe *pe, char *data)
{
    int i, index = 0;
    tw_kp *kp;
    tw_lp *lp;
    tw_stat tmp;
    long long tmp2;

    /* PE granularity */
    tmp = pe->stats.s_nevent_abort - last_event_counters.s_nevent_abort;
    memcpy(&data[index], &tmp, sizeof(tw_stat));
    index += sizeof(tw_stat);
    last_event_counters.s_nevent_abort = pe->stats.s_nevent_abort;

    tmp = tw_pq_get_size(pe->pq);// - last_event_counters.s_pq_qsize;
    memcpy(&data[index], &tmp, sizeof(tw_stat));
    index += sizeof(tw_stat);
    last_event_counters.s_pq_qsize = tw_pq_get_size(pe->pq);

    tmp = pe->stats.s_pe_event_ties - last_event_counters.s_pe_event_ties;
    memcpy(&data[index], &tmp, sizeof(tw_stat));
    index += sizeof(tw_stat);
    last_event_counters.s_pe_event_ties = pe->stats.s_pe_event_ties;

    tmp = g_tw_fossil_attempts - last_event_counters.s_fc_attempts;
    memcpy(&data[index], &tmp, sizeof(tw_stat));
    index += sizeof(tw_stat);
    last_event_counters.s_fc_attempts = g_tw_fossil_attempts;

    tmp = g_tw_gvt_done - last_event_counters.s_ngvts;
    memcpy(&data[index], &tmp, sizeof(tw_stat));
    index += sizeof(tw_stat);
    last_event_counters.s_ngvts = g_tw_gvt_done;

    /* KP granularity */
    for(i = 0; i < g_tw_nkp; i++)
    {
        kp = tw_getkp(i);

        tmp = kp->s_rb_total - kp->last_s_rb_total_rt;
        memcpy(&data[index], &tmp, sizeof(tw_stat));
        index += sizeof(tw_stat);
        kp->last_s_rb_total_rt = kp->s_rb_total;

        tmp = kp->s_rb_secondary - kp->last_s_rb_secondary_rt;
        memcpy(&data[index], &tmp, sizeof(tw_stat));
        index += sizeof(tw_stat);
        kp->last_s_rb_secondary_rt = kp->s_rb_secondary;
    }


    /* LP granularity */
    for (i=0; i < g_tw_nlp; i++)
    {
        lp = tw_getlp(i);

        tmp = lp->event_counters->s_nevent_processed - lp->prev_event_counters_rt->s_nevent_processed;
        memcpy(&data[index], &tmp, sizeof(tw_stat));
        index += sizeof(tw_stat);
        lp->prev_event_counters_rt->s_nevent_processed = lp->event_counters->s_nevent_processed;

        tmp = lp->event_counters->s_e_rbs - lp->prev_event_counters_rt->s_e_rbs;
        memcpy(&data[index], &tmp, sizeof(tw_stat));
        index += sizeof(tw_stat);
        lp->prev_event_counters_rt->s_e_rbs = lp->event_counters->s_e_rbs;

        tmp = lp->event_counters->s_nsend_network - lp->prev_event_counters_rt->s_nsend_network;
        memcpy(&data[index], &tmp, sizeof(tw_stat));
        index += sizeof(tw_stat);
        lp->prev_event_counters_rt->s_nsend_network = lp->event_counters->s_nsend_network;

        tmp = lp->event_counters->s_nread_network - lp->prev_event_counters_rt->s_nread_network;
        memcpy(&data[index], &tmp, sizeof(tw_stat));
        index += sizeof(tw_stat);
        lp->prev_event_counters_rt->s_nread_network = lp->event_counters->s_nread_network;
    }
    for (i=0; i < g_tw_nlp; i++)
    {
        lp = tw_getlp(i);
        // next stat not guaranteed to always be non-decreasing
        // can't use tw_stat (unsigned long long) for negative number
        tmp2 = (long long)lp->event_counters->s_nsend_net_remote - (long long)lp->prev_event_counters_rt->s_nsend_net_remote;
        memcpy(&data[index], &tmp2, sizeof(long long));
        index += sizeof(long long);
        lp->prev_event_counters_rt->s_nsend_net_remote = lp->event_counters->s_nsend_net_remote;
    }
}

void st_collect_memory_usage(char *data)
{
    size_t mem_allocated, mem_wasted, tmp;
    int index = 0;
    tw_calloc_stats(&mem_allocated, &mem_wasted);

    tmp = mem_allocated - last_mem_usage.mem_allocated;
    memcpy(&data[index], &tmp, sizeof(size_t));
    index += sizeof(size_t);
    tmp = mem_wasted - last_mem_usage.mem_wasted;
    memcpy(&data[index], &tmp, sizeof(size_t));

    last_mem_usage.mem_allocated = mem_allocated;
    last_mem_usage.mem_wasted = mem_wasted;
}


