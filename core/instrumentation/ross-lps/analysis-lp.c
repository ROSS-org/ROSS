#include "ross.h"
#include "analysis-lp.h"
#include <math.h>
#include <limits.h>

/*
 * The code for virtual time sampling is based off of the model-net sampling for
 * CODES network models
 */

static void st_create_sample_event(tw_lp *lp);

void analysis_init(analysis_state *s, tw_lp *lp)
{
    int i, j, idx = 0, sim_idx = 0;
    tw_lp *cur_lp;

    // set our id relative to all analysis LPs
    s->analysis_id = lp->gid - analysis_start_gid;
    s->num_lps = ceil((double)g_tw_nlp / g_tw_nkp);

    s->sample_sz = sizeof(model_sample_data);

    // create list of LPs this is responsible for
    s->lp_list = tw_calloc(TW_LOC, "analysis LPs", sizeof(tw_lpid), s->num_lps);
    s->lp_list_sim = tw_calloc(TW_LOC, "analysis LPs", sizeof(tw_lpid), s->num_lps);
    // size of lp_list is max number of LPs this analysis LP is responsible for
    for (i = 0; i < s->num_lps; i++)
    {
        s->lp_list[i] = ULLONG_MAX;
        s->lp_list_sim[i] = ULLONG_MAX;
    }

    for (i = 0; i < g_tw_nlp; i++)
    {
        cur_lp = g_tw_lp[i];

        if (cur_lp->kp->id == s->analysis_id % g_tw_nkp)
        {
            s->lp_list_sim[sim_idx] = cur_lp->gid;
            sim_idx++;

            // check if this LP even needs sampling performed
            if (cur_lp->model_types->sample_struct_sz == 0)
                continue;

            s->lp_list[idx] = cur_lp->gid;
            s->sample_sz += cur_lp->model_types->sample_struct_sz;
            idx++;
        }
    }

    // update num_lps
    s->num_lps = idx;
    s->num_lps_sim = sim_idx;

    // setup memory to use for model samples
    s->model_samples = (model_sample_data*) tw_calloc(TW_LOC, "analysis LPs", sizeof(model_sample_data), g_st_sample_count); 
    for (i = 0; i < g_st_sample_count; i++)
    {
        s->model_samples[i].lp_data = (void**) tw_calloc(TW_LOC, "analysis LPs", sizeof(void*), s->num_lps);
        for (j = 0; j < s->num_lps; j++)
        {
            cur_lp = tw_getlocal_lp(s->lp_list[j]);
            s->model_samples[i].lp_data[j] = (void *) tw_calloc(TW_LOC, "analysis LPs", cur_lp->model_types->sample_struct_sz, 1);
        }
    }
    s->start_sample = 0;
    s->current_sample = -1;

    // set up sample structs for sim engine data
    s->prev_data_kp.time_ahead_gvt = 0;
    s->prev_data_kp.rb_total = 0;
    s->prev_data_kp.rb_secondary = 0;

    s->prev_data_lp = tw_calloc(TW_LOC, "analysis LPs", sizeof(sim_engine_data_lp), s->num_lps_sim);
    for (i = 0; i < s->num_lps_sim; i++)
    {
        s->prev_data_lp[i].nevent_processed = 0; 
        s->prev_data_lp[i].e_rbs = 0;
        s->prev_data_lp[i].nsend_network = 0;
        s->prev_data_lp[i].nread_network = 0;
    }

    // schedule 1st sampling event 
    st_create_sample_event(lp);
}

void analysis_event(analysis_state *s, tw_bf *bf, analysis_msg *m, tw_lp *lp)
{
    int i;
    tw_lp *model_lp;

    s->current_sample++;
    model_sample_data *sample = &s->model_samples[s->current_sample];
    sample->timestamp = tw_now(lp);
    m->timestamp = tw_now(lp);

    // call the model sampling function for each LP on this KP
    for (i = 0; i < s->num_lps; i++)
    {
        if (s->lp_list[i] == ULLONG_MAX)
            break;

        model_lp = tw_getlocal_lp(s->lp_list[i]);
        if (model_lp->model_types->sample_struct_sz == 0)
            continue;

        model_lp->model_types->sample_event_fn(model_lp->cur_state, bf, lp, sample->lp_data[i]);
    } 

    // sim engine sampling
    collect_sim_engine_data(lp->pe, lp, s, (tw_stime) tw_clock_read() / g_tw_clock_rate);
    
    // create next sampling event
    st_create_sample_event(lp);
}

void analysis_event_rc(analysis_state *s, tw_bf *bf, analysis_msg *m, tw_lp *lp)
{
    tw_lp *model_lp;
    int i, j;
    // need to remove sample associated with this event from the list
    model_sample_data *sample;
    // start at end, because it's most likely closer to the timestamp we're looking for
    for (i = s->current_sample; i >= s->start_sample; i --)
    { 
        sample = &s->model_samples[i];
        if (sample->timestamp == m->timestamp)
        {
            for (j = 0; j < s->num_lps; j++)
            {
                if (s->lp_list[j] == ULLONG_MAX)
                    break;

                // first call the appropriate RC fn, to allow it to undo any state changes
                model_lp = tw_getlocal_lp(s->lp_list[j]);
                model_lp->model_types->sample_revent_fn(model_lp->cur_state, bf, lp, sample->lp_data[j]);
            }

            s->current_sample--;

            break;
        }
        else if (sample->timestamp < m->timestamp)
            break;
    }
    
}

void analysis_commit(analysis_state *s, tw_bf *bf, analysis_msg *m, tw_lp *lp)
{
    // write committed data to buffer
    model_sample_data *sample;
    int i, j;
    tw_lp *model_lp;
    lp_metadata metadata;
    // start at beginning
    for (i = s->start_sample; i <= s->current_sample; i++)
    {
        sample = &s->model_samples[i];
        if (sample->timestamp == m->timestamp)
        {
            for (j = 0; j < s->num_lps; j++)
            {
                model_lp = tw_getlocal_lp(s->lp_list[j]);
                metadata.lpid = model_lp->gid;
                metadata.kpid = model_lp->kp->id;
                metadata.peid = model_lp->pe->id;
                metadata.ts = m->timestamp;
                metadata.sample_sz = model_lp->model_types->sample_struct_sz;
                metadata.flag = MODEL_TYPE;

                char buffer[sizeof(lp_metadata) + model_lp->model_types->sample_struct_sz];
                memcpy(&buffer[0], (char*)&metadata, sizeof(lp_metadata));
                memcpy(&buffer[sizeof(lp_metadata)], (char*)sample->lp_data[j], model_lp->model_types->sample_struct_sz);
                st_buffer_push(ANALYSIS_LP, &buffer[0], sizeof(lp_metadata) + model_lp->model_types->sample_struct_sz); 
            }
            s->start_sample++;
            break;
        }
        else if (sample->timestamp > m->timestamp)
            // can stop searching now
            break;
    }
}

void analysis_finish(analysis_state *s, tw_lp *lp)
{
    // write all remaining samples to buffer
    model_sample_data *sample;
    int i, j;
    tw_lp *model_lp;
    // start at beginning
    for (i = s->start_sample; i <= s->current_sample; i++)
    {
        sample = &s->model_samples[i];
        for (j = 0; j < s->num_lps; j++)
        {
            model_lp = tw_getlocal_lp(s->lp_list[j]);
            char buffer[model_lp->model_types->sample_struct_sz];
            memcpy(&buffer[0], (char*)sample->lp_data[j], model_lp->model_types->sample_struct_sz);
            st_buffer_push(ANALYSIS_LP, &buffer[0], model_lp->model_types->sample_struct_sz); 
        }
    }

}

static void st_create_sample_event(tw_lp *lp)
{
    if (tw_now(lp) + g_st_vt_interval <= g_st_sampling_end)
    {
        tw_event *e = tw_event_new(lp->gid, g_st_vt_interval, lp);
        analysis_msg *m = tw_event_data(e);
        m->src = lp->gid;
        tw_event_send(e);
    }
}

/*
 * any specialized ROSS LP should be hidden from the model
 * need to add in these LP after all model LPs
 */
tw_peid analysis_map(tw_lpid gid)
{
    tw_lpid local_id = gid - analysis_start_gid;
    return local_id / g_tw_nkp; // because there is 1 LP for each KP 
}

tw_lptype analysis_lp[] = {
	{(init_f) analysis_init,
     (pre_run_f) NULL,
	 (event_f) analysis_event,
	 (revent_f) analysis_event_rc,
	 (commit_f) analysis_commit,
	 (final_f) analysis_finish,
	 (map_f) analysis_map,
	sizeof(analysis_state)},
	{0},
};

void st_analysis_lp_settype(tw_lpid lpid)
{
    tw_lp_settype(lpid, &analysis_lp[0]);
}

// although collected at virtual time sampling intervals, do not roll back this collected data
// i.e., immediately put into the data buffer instead of waiting until commit events
// TODO should we collect perf data on all LPs on this KP, or just the ones we're collecting model data for?
void collect_sim_engine_data(tw_pe *pe, tw_lp *lp, analysis_state *s, tw_stime current_rt)
{
    lp_metadata metadata;
    tw_lp *cur_lp;
    sim_engine_data_kp cur_data_kp;
    sim_engine_data_lp cur_data_lp;
    char kp_buffer[sizeof(lp_metadata) + sizeof(sim_engine_data_kp)];
    char lp_buffer[sizeof(lp_metadata) + sizeof(sim_engine_data_lp)];
    int i;

    // kp data
    metadata.lpid = lp->gid;
    metadata.kpid = lp->kp->id;
    metadata.peid = lp->pe->id;
    metadata.ts = tw_now(lp);
    metadata.real_time = current_rt;
    metadata.sample_sz = sizeof(sim_engine_data_kp);
    metadata.flag = KP_TYPE;

    cur_data_kp.time_ahead_gvt = lp->kp->last_time - pe->GVT;
    cur_data_kp.rb_total = lp->kp->s_rb_total - s->prev_data_kp.rb_total;
    cur_data_kp.rb_secondary = lp->kp->s_rb_secondary - s->prev_data_kp.rb_secondary;
    s->prev_data_kp.rb_total = lp->kp->s_rb_total;
    s->prev_data_kp.rb_secondary = lp->kp->s_rb_secondary;

    memcpy(&kp_buffer[0], (char*)&metadata, sizeof(lp_metadata));
    memcpy(&kp_buffer[sizeof(lp_metadata)], (char*)&cur_data_kp, sizeof(sim_engine_data_kp));

    st_buffer_push(ANALYSIS_LP, &kp_buffer[0], sizeof(lp_metadata) + sizeof(sim_engine_data_kp));

    // lp data
    metadata.sample_sz = sizeof(sim_engine_data_lp);
    metadata.flag = LP_TYPE;
    for (i = 0; i < s->num_lps_sim; i++)
    {
        cur_lp = tw_getlocal_lp(s->lp_list_sim[i]);

        metadata.lpid = cur_lp->gid;

        cur_data_lp.nevent_processed = cur_lp->event_counters->s_nevent_processed - s->prev_data_lp[i].nevent_processed;
        cur_data_lp.e_rbs = cur_lp->event_counters->s_e_rbs - s->prev_data_lp[i].e_rbs;
        cur_data_lp.nsend_network = cur_lp->event_counters->s_nsend_network - s->prev_data_lp[i].nsend_network;
        cur_data_lp.nread_network = cur_lp->event_counters->s_nread_network - s->prev_data_lp[i].nread_network;
        s->prev_data_lp[i].nevent_processed = cur_lp->event_counters->s_nevent_processed;
        s->prev_data_lp[i].e_rbs = cur_lp->event_counters->s_e_rbs;
        s->prev_data_lp[i].nsend_network = cur_lp->event_counters->s_nsend_network;
        s->prev_data_lp[i].nread_network = cur_lp->event_counters->s_nread_network;

        memcpy(&lp_buffer[0], (char*)&metadata, sizeof(lp_metadata));
        memcpy(&lp_buffer[sizeof(lp_metadata)], (char*)&cur_data_lp, sizeof(sim_engine_data_lp));

        st_buffer_push(ANALYSIS_LP, &lp_buffer[0], sizeof(lp_metadata) + sizeof(sim_engine_data_lp));
    }
}
