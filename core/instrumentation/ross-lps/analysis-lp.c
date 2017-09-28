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
    int i, j, idx = 0;
    tw_lp *cur_lp;

    // set our id relative to all analysis LPs
    s->analysis_id = lp->gid - analysis_start_gid;
    s->num_lps = ceil((double)g_tw_nlp / g_tw_nkp);

    s->sample_sz = sizeof(model_sample_data);

    // create list of LPs this is responsible for
    s->lp_list = tw_calloc(TW_LOC, "analysis LPs", sizeof(tw_lpid), s->num_lps);
    // size of lp_list is max number of LPs this analysis LP is responsible for
    for (i = 0; i < s->num_lps; i++)
        s->lp_list[i] = ULLONG_MAX;

    for (i = 0; i < g_tw_nlp; i++)
    {
        cur_lp = g_tw_lp[i];

        // check if this LP even needs sampling performed
        if (cur_lp->model_types->sample_struct_sz == 0)
            continue;

        if (cur_lp->kp->id == s->analysis_id % g_tw_nkp)
        {
            s->lp_list[idx] = cur_lp->gid;
            s->sample_sz += cur_lp->model_types->sample_struct_sz;
            idx++;
        }
    }

    // update num_lps
    s->num_lps = idx;

    // setup memory to use for samples
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

    // call the sampling function for each LP on this KP
    for (i = 0; i < s->num_lps; i++)
    {
        if (s->lp_list[i] == ULLONG_MAX)
            break;

        model_lp = tw_getlocal_lp(s->lp_list[i]);
        if (model_lp->model_types->sample_struct_sz == 0)
            continue;

        model_lp->model_types->sample_event_fn(model_lp->cur_state, bf, lp, sample->lp_data[i]);
    } 
    
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
    // start at beginning
    for (i = s->start_sample; i <= s->current_sample; i++)
    {
        sample = &s->model_samples[i];
        if (sample->timestamp == m->timestamp)
        {
            for (j = 0; j < s->num_lps; j++)
            {
                model_lp = tw_getlocal_lp(s->lp_list[j]);
                char buffer[model_lp->model_types->sample_struct_sz];
                memcpy(&buffer[0], (char*)sample->lp_data[j], model_lp->model_types->sample_struct_sz);
                st_buffer_push(ANALYSIS_LP, &buffer[0], model_lp->model_types->sample_struct_sz); 
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
    if (tw_now(lp) + g_st_vt_interval < g_st_sampling_end)
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
