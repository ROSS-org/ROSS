#include "la-pdes.h"
#include <math.h>

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

// forward decl of event handlers
void lapdes_send(lapdes_state *s, tw_bf *bf, lapdes_message *m, tw_lp *lp);
void lapdes_send_rc(lapdes_state *s, tw_bf *bf, lapdes_message *m, tw_lp *lp);
void lapdes_recv(lapdes_state *s, tw_bf *bf, lapdes_message *m, tw_lp *lp);
void lapdes_recv_rc(lapdes_state *s, tw_bf *bf, lapdes_message *m, tw_lp *lp);

tw_peid lapdes_map(tw_lpid gid)
{
    return (tw_peid) gid / g_tw_nlp;
}

void lapdes_init(lapdes_state *s, tw_lp *lp)
{
    double seed = lp->gid + init_seed;
    tw_rand_initial_seed(lp->rng, seed);

    double prob = 0.0;

    // compute # events that LP will send out
    if (p_send == 0) // uniform case
        prob = 1.0 / n_ent;
    else
    {
        if (invert)
            prob = p_send * pow((1 - p_send), (n_ent - lp->gid));
        else
            prob = p_send * pow((1 - p_send), lp->gid);
    }

    unsigned long target_sends = prob * target_global_sends;
    if (target_sends > 0)
        s->local_intersend_delay = end_time / target_sends;
    else
        s->local_intersend_delay = 10 * end_time;

    // allocate appropriate memory space
    if (p_list == 0) // uniform case
        prob = 1.0 / n_ent;
    else
        prob = p_list * pow((1 - p_list), lp->gid);
    s->list_size = (prob * n_ent * m_ent) + 1;
    s->list = tw_calloc(TW_LOC, "LA-PDES", sizeof(double), s->list_size);
    s->ops = (prob * n_ent * ops_ent) + 1;
    s->active_elements = cache_friendliness * s->list_size;
    int i;
    for (i = 0; i < s->list_size; i++)
        s->list[i] = tw_rand_unif(lp->rng);

    // set up queue size
    s->q_target = q_avg / s_ent * target_sends;
    s->q_size = 1;
    s->last_scheduled = tw_now(lp);

    // set up statistics
    s->ops_min = ULLONG_MAX;  // everything else is already 0 in state struct

    // send first event
    tw_event *e = tw_event_new(lp->gid, 0, lp);
    lapdes_message *msg = tw_event_data(e);
    msg->event_type = SEND;
    tw_event_send(e);
}

void lapdes_event_handler(lapdes_state *s, tw_bf *bf, lapdes_message *m, tw_lp *lp)
{
    switch (m->event_type)
    {
        case SEND:
            lapdes_send(s, bf, m, lp);
            break;
        case RECV:
            lapdes_recv(s, bf, m, lp);
            break;
        default:
            tw_error(TW_LOC, "LA-PDES event_type not found!");
    }
}

void lapdes_event_handler_rc(lapdes_state *s, tw_bf *bf, lapdes_message *m, tw_lp *lp)
{
    switch (m->event_type)
    {
        case SEND:
            lapdes_send_rc(s, bf, m, lp);
            break;
        case RECV:
            lapdes_recv_rc(s, bf, m, lp);
            break;
        default:
            tw_error(TW_LOC, "LA-PDES event_type not found!");
    }
}

void lapdes_send(lapdes_state *s, tw_bf *bf, lapdes_message *m, tw_lp *lp)
{
    s->send_count++;
    s->q_size--;
    //s->time_sends[floor(tw_now(lp) / (end_time + 0.0001) * time_bins)]++;

    // reschedule self until q is full or time is out
    while(s->q_size < s->q_target && !(s->last_scheduled > end_time))
    {
        double own_delay = tw_rand_exponential(lp->rng, s->local_intersend_delay);
        printf("own_delay %f\n", own_delay);
        s->last_scheduled += own_delay;
        if (s->last_scheduled < end_time)
        {
            s->q_size++;
            //printf("pe %ld lp %lu sending event to lp %lu at %f\n", g_tw_mynode, lp->gid, lp->gid, tw_now(lp));
            tw_event *e = tw_event_new(lp->gid, s->last_scheduled - tw_now(lp), lp);
            lapdes_message *msg = tw_event_data(e);
            msg->event_type = SEND;
            tw_event_send(e);
            //printf("Send: scheduling next self event at %f\n", s->last_scheduled - tw_now(lp));
        }
    }

    // generate computation request event to dest
    tw_lpid dest_id;
    if (p_recv == 1.0)
        dest_id = 0;
    else if (p_recv == 0.0)
        dest_id = tw_rand_unif(lp->rng) * n_ent;
    else
    {
        double u = r_min + (1.0 - r_min) * tw_rand_unif(lp->rng);
        dest_id = (ceil(log(u) / log(1.0 - p_recv))) - 1;
    }
    //printf("pe %ld lp %lu sending event to lp %lu at %f\n", g_tw_mynode, lp->gid, dest_id, tw_now(lp));
    if (tw_now(lp) + min_delay < end_time)
    {
        tw_event *e = tw_event_new(dest_id, min_delay, lp);
        lapdes_message *msg = tw_event_data(e);
        msg->event_type = RECV;
        tw_event_send(e);
        //printf("Send: sending event to %lu at %f\n", dest_id, min_delay);
    }
}

void lapdes_send_rc(lapdes_state *s, tw_bf *bf, lapdes_message *m, tw_lp *lp)
{

}

void lapdes_recv(lapdes_state *s, tw_bf *bf, lapdes_message *m, tw_lp *lp)
{
    unsigned int rng_calls;
    double num = tw_rand_normal_sd(lp->rng, s->ops, s->ops*ops_sigma, &rng_calls);
    unsigned int r_ops = MAX(1, num);
    unsigned int r_active_elements = s->active_elements * ((double)r_ops / s->ops);
    r_active_elements = MIN(r_active_elements, s->list_size);
    r_active_elements = MAX(1, r_active_elements);
    unsigned int r_ops_per_element = r_ops / r_active_elements;

    s->recv_count++;
    s->ops_max = MAX(s->ops_max, r_ops);
    s->ops_min = MIN(s->ops_min, r_ops);
    s->ops_mean = (s->ops_mean * (s->recv_count - 1) + r_ops) / s->recv_count;

    double value = 0.0;
    int i, j;
    for (i = 0; i < r_active_elements; i++)
    {
        for (j = 0; j < r_ops_per_element; j++)
            value += s->list[i] * tw_rand_unif(lp->rng);
    }
}

void lapdes_recv_rc(lapdes_state *s, tw_bf *bf, lapdes_message *m, tw_lp *lp)
{

}

void lapdes_commit(lapdes_state *s, tw_bf *bf, lapdes_message *m, tw_lp *lp)
{
}

void
lapdes_finish(lapdes_state *s, tw_lp *lp)
{
}

tw_lptype mylps[] = {
    {(init_f) lapdes_init,
     (pre_run_f) NULL,
     (event_f) lapdes_event_handler,
     (revent_f) lapdes_event_handler_rc,
     (commit_f) lapdes_commit,
     (final_f) lapdes_finish,
     (map_f) lapdes_map,
    sizeof(lapdes_state)},
    {0},
};

const tw_optdef app_opt[] =
{
    TWOPT_GROUP("LA-PDES Benchmark Model"),
    TWOPT_UINT("n-ent", n_ent, "Number of entities/LPs per PE"),
    TWOPT_UINT("s-ent", s_ent, "Average number of send events per entity"),
    TWOPT_STIME("q-avg", q_avg, "Average number of events in the event queue per entity"),
    TWOPT_STIME("p-recv", p_recv, "geometric distribution for choosing destination entities"),
    TWOPT_STIME("p-send", p_send, "geometric distribution of source entities"),
    TWOPT_FLAG("invert", invert, "invert send and recv distributions?"),
    TWOPT_UINT("m-ent", m_ent, "average memory footprint per entity"),
    TWOPT_STIME("p-list", p_list, "geometric distribution of linear list sizes"),
    TWOPT_STIME("ops-ent", ops_ent, "average operations per handler per entity"),
    TWOPT_STIME("ops-sigma", ops_sigma, "variance of number of operations per handler per entity"),
    TWOPT_STIME("cache-friendliness", cache_friendliness, "determines how many different list elements are accessed during operations"),
    TWOPT_UINT("init-seed", init_seed, "initial random seed"),
    TWOPT_FLAG("compute-end", compute_end, "if on, end time = n-end * s-ent, else use --end"),
    TWOPT_UINT("time-bins", time_bins, "number of bins for time and event reporting"),
    TWOPT_STIME("min-delay", min_delay, "min delay for synch between MPI ranks"),
    TWOPT_END()
};

// ROSS Instrumentation
void sample_model(lapdes_state *s, tw_bf *bf, tw_lp *lp, lapdes_sample *sample)
{
    sample->send_count = s->send_count;
    sample->recv_count = s->recv_count;
    sample->ops_max = s->ops_max;
    sample->ops_min = s->ops_min;
    sample->ops_mean = s->ops_mean;
}

void sample_model_rc(lapdes_state *state, tw_bf *bf, tw_lp *lp, lapdes_sample *sample)
{

}

st_model_types model_types[] = {
    {NULL,
     0,
     NULL,
     0,
     (sample_event_f) sample_model,
     (sample_revent_f) sample_model_rc,
     sizeof(lapdes_sample)},
    {0}
};

int
main(int argc, char **argv, char **env)
{

    tw_opt_add(app_opt);
    tw_init(&argc, &argv);

    // they have min_delay as an option, but then override it to always be 1
    min_delay = 1.0;

    // when they make their simian call for init sim params,
    // they pass endTime +10*minDelay as the sim end time,
    // apparently to account for collecting statistics?
    if (compute_end)
        g_tw_ts_end = n_ent * s_ent;
    end_time = g_tw_ts_end;
    g_tw_ts_end += 1; // to allow for collection of statistics

    r_min = pow((1 - p_recv), n_ent);
    target_global_sends = n_ent * s_ent;

    tw_define_lps(n_ent, sizeof(lapdes_message));

    int i;
    for(i = 0; i < g_tw_nlp; i++)
    {
        tw_lp_settype(i, &mylps[0]);
        st_model_settype(i, &model_types[0]);
    }

    if( g_tw_mynode == 0 )
    {
        printf("========================================\n");
        printf("LA-PDES Model Configuration..............\n");
        printf("  Communication Params:\n");
        printf("    n_ent                  %u\n", n_ent);
        printf("    s_ent                  %u\n", s_ent);
        printf("    q_avg                  %lf\n", q_avg);
        printf("    p_recv                 %lf\n", p_recv);
        printf("    p_send                 %lf\n", p_send);
        printf("    invert                 %u\n", invert);
        printf("  Memory Params:\n");
        printf("    m_ent                  %u\n", m_ent);
        printf("    p_list                 %lf\n", p_list);
        printf("  Computation Params:\n");
        printf("    ops_ent                %lf\n", ops_ent);
        printf("    ops_sigma              %lf\n", ops_sigma);
        printf("    cache_friendliness     %lf\n", cache_friendliness);
        printf("  PDES Params:\n");
        printf("    time_bins              %u\n", time_bins);
        printf("    init_seed              %u\n", init_seed);
        printf("========================================\n\n");
    }

    tw_run();
    tw_end();

    return 0;
}
