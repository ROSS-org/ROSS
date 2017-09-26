#include "ross.h"
#include "analysis-lp.h"
#include <math.h>
#include <limits.h>

/*
 * The code for virtual time sampling is based off of the model-net sampling for
 * CODES network models
 */

//static model_sample_data *model_samples = NULL;
//static model_sample_data *current_sample = NULL;

static void st_create_sample_event(tw_lp *lp);

void analysis_init(analysis_state *s, tw_lp *lp)
{
    int i, idx = 0;
    tw_lp *cur_lp;

    // set our id relative to all analysis LPs
    s->analysis_id = lp->gid - analysis_start_gid;

    // create list of LPs this is responsible for
    s->lp_list = tw_calloc(TW_LOC, "analysis LPs", sizeof(tw_lpid), ceil(g_tw_nlp / g_tw_nkp));
    for (i = 0; i < ceil(g_tw_nlp / g_tw_nkp); i++)
        s->lp_list[i] = ULLONG_MAX;

    for (i = 0; i < g_tw_nlp; i++)
    {
        cur_lp = g_tw_lp[i];
        if (cur_lp->kp->id == s->analysis_id % g_tw_nkp)
        {
            s->lp_list[idx] = cur_lp->gid;
            idx++;
        }
    }

    // schedule 1st sampling event 
    st_create_sample_event(lp);
}

// TODO need to handle memory better; as is it will create memory leaks
// look at AVL tree for how it handles memory management?
void analysis_event(analysis_state *s, tw_bf *bf, analysis_msg *m, tw_lp *lp)
{
    int i;
    tw_lp *model_lp;

    // create struct for storing this sample's data 
    model_sample_data *sample = tw_calloc(TW_LOC, "analysis LPs", sizeof(struct model_sample_data), 1); 
    sample->next = NULL;
    sample->prev = NULL;
    sample->timestamp = tw_now(lp);
    m->timestamp = tw_now(lp);
    sample->lp_data = tw_calloc(TW_LOC, "analysis LPs", sizeof(void *), lp->kp->lp_count);    

    // call the sampling function for each LP on this KP
    for (i = 0; i < lp->kp->lp_count; i++)
    {
        if (s->lp_list[i] == ULLONG_MAX)
            break;

        model_lp = g_tw_lp[s->lp_list[i]];
        sample->lp_data[i] = tw_calloc(TW_LOC, "analysis LPs", model_lp->model_types->sample_struct_sz, 1);
        model_lp->model_types->sample_event_fn(model_lp->cur_state, bf, lp, sample->lp_data[i]);
    } 
    
    // save this sample in the list
    if (!s->model_samples)
        s->model_samples = sample;
    else
        s->current_sample->next = sample;

    sample->prev = s->current_sample;
    s->current_sample = sample;

    // create next sampling event
    st_create_sample_event(lp);
}

void analysis_event_rc(analysis_state *s, tw_bf *bf, analysis_msg *m, tw_lp *lp)
{
    tw_lp *model_lp;
    int i;
    // need to remove sample associated with this event from the list
    model_sample_data *sample;
    // start at end, because it's most likely closer to the timestamp we're looking for
    for (sample = s->current_sample; sample->prev != NULL; sample = sample->prev)
    {
        if (sample->timestamp == m->timestamp)
        {
            for (i = 0; i < lp->kp->lp_count; i++)
            {
                if (s->lp_list[i] = ULLONG_MAX)
                    break;

                // first call the appropriate RC fn, to allow it to undo any state changes
                model_lp = g_tw_lp[s->lp_list[i]];
                model_lp->model_types->sample_revent_fn(model_lp->cur_state, bf, lp, sample->lp_data[i]);
            }

            // TODO actually for RC to be correct, we need to undo each event
            sample->prev->next = NULL; // guaranteed that we won't need to keep samples after this point in time
            s->current_sample = sample->prev;
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
    int i;
    tw_lp *model_lp;
    // start at beginning
    for (sample = s->model_samples; sample->next != NULL; sample = sample->next)
    {
        if (sample->timestamp == m->timestamp)
        {
            for (i = 0; i < lp->kp->lp_count; i++)
            {
                model_lp = g_tw_lp[s->lp_list[i]];
                st_buffer_push(ANALYSIS_LP, sample->lp_data[i], model_lp->model_types->sample_struct_sz); 
            }
            // remove committed data from model_samples
            s->model_samples = sample->next;
            s->model_samples->prev = NULL;
            break;
        }
        else if (sample->timestamp < m->timestamp)
            // can stop searching now
            break;
    }
}

void analysis_finish(analysis_state *s, tw_lp *lp)
{
    // write all remaining samples to buffer
    model_sample_data *sample;
    int i;
    tw_lp *model_lp;
    // start at beginning
    for (sample = s->model_samples; sample->next != NULL; sample = sample->next)
    {
        for (i = 0; i < lp->kp->lp_count; i++)
        {
            model_lp = g_tw_lp[s->lp_list[i]];
            st_buffer_push(ANALYSIS_LP, sample->lp_data[i], model_lp->model_types->sample_struct_sz); 
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
 * Actually: easiest to just throw the specialized LPs at the end of each PE
 * but then provide a ROSS gid and a gid so current models do not break
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
