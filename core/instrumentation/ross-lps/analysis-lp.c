#include "ross.h"
#include "analysis-lp.h"
#include <math.h>
#include <limits.h>
#include <instrumentation/st-stats-buffer.h>
#include <instrumentation/st-instrumentation-internal.h>
#include <instrumentation/st-sim-engine.h>
#include <instrumentation/st-model-data.h>

/*
 * The code for virtual time sampling is based off of the model-net sampling for
 * CODES network models
 */

static void st_create_sample_event(tw_lp *lp);

// TODO potential bug: Assumes that LPs are assigned to KPs in a round robin fashion
void analysis_init(analysis_state *s, tw_lp *lp)
{
    int i, j, idx = 0, sim_idx = 0;
    tw_lp *cur_lp;

    // set our id relative to all analysis LPs
    s->analysis_id = lp->gid - analysis_start_gid;
    s->num_lps = ceil((double)g_tw_nlp / g_tw_nkp);
    s->event_id = 0;
    s->model_sample_size = 0;

    // create list of LPs this is responsible for
    s->lp_list = (tw_lpid*)tw_calloc(TW_LOC, "analysis LPs", sizeof(tw_lpid), s->num_lps);
    s->lp_list_sim = (tw_lpid*)tw_calloc(TW_LOC, "analysis LPs", sizeof(tw_lpid), s->num_lps);
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
            if (cur_lp->model_types == NULL || !cur_lp->model_types->vts_event_fn
                    || cur_lp->model_types->num_vars <= 0)
                continue;

            s->model_sample_size += sizeof(st_model_data);
            for (j = 0; j < cur_lp->model_types->num_vars; j++)
            {
                s->model_sample_size += sizeof(model_var_md);
                s->model_sample_size += model_var_type_size(cur_lp->model_types->model_vars[j].type) *
                    cur_lp->model_types->model_vars[j].num_elems;
            }

            s->lp_list[idx] = cur_lp->gid;
            idx++;
        }
    }

    // update num_lps
    s->num_lps = idx;
    s->num_lps_sim = sim_idx;

    // setup memory to use for model samples
    if ((model_modes[VT_INST]) && s->num_lps > 0)
    {
        s->model_samples_head = (model_sample_data*) tw_calloc(TW_LOC, "analysis LPs", sizeof(model_sample_data), g_st_sample_count);
        s->model_samples_current = s->model_samples_head;
        model_sample_data *sample = s->model_samples_head;
        for (i = 0; i < g_st_sample_count; i++)
        {
            if (i == 0)
            {
                sample->prev = NULL;
                sample->next = sample + 1;
                sample->next->prev = sample;
            }
            else if (i == g_st_sample_count - 1)
            {
                sample->next = NULL;
                s->model_samples_tail = sample;
            }
            else
            {
                sample->next = sample + 1;
                sample->next->prev = sample;
            }
            sample->lp_data = (model_lp_sample*) calloc(sizeof(model_lp_sample), s->num_lps);
            sample->num_lps = s->num_lps;
            for (j = 0; j < s->num_lps; j++)
            {
                cur_lp = tw_getlocal_lp(s->lp_list[j]);
                sample->lp_data[j].lp_name = cur_lp->model_types->lp_name;
                sample->lp_data[j].lpid = cur_lp->gid;
                sample->lp_data[j].num_vars = cur_lp->model_types->num_vars;
                sample->lp_data[j].vars = (model_var_data*)calloc(sizeof(model_var_data), cur_lp->model_types->num_vars);
            }
            sample = sample->next;
        }
    }

    // schedule 1st sampling event
    st_create_sample_event(lp);
}

void analysis_event(analysis_state *s, tw_bf *bf, analysis_msg *m, tw_lp *lp)
{
    int i;
    tw_lp *model_lp;

    lp->pe->stats.s_alp_nevent_processed++; //don't undo in RC

    model_sample_data *sample = NULL;
    if ((model_modes[VT_INST]) && s->num_lps > 0)
    {
        sample = s->model_samples_current;
        // TODO handle this situation better
        if (sample == s->model_samples_tail)
            printf("WARNING: last available sample space for analysis lp!\n");

        sample->vts = tw_now(lp);
        sample->rts = tw_clock_read() / (double)g_tw_clock_rate;
        m->timestamp = tw_now(lp);
        vts_start_sample(sample);
        //printf("PE %ld KP %lu: analysis_event at %f (rt %f)\n", g_tw_mynode, lp->kp->id,
        //        sample->vts, sample->rts);

        // call the model sampling function for each LP on this KP
        for (i = 0; i < s->num_lps; i++)
        {
            if (s->lp_list[i] == ULLONG_MAX)
                break;

            model_lp = tw_getlocal_lp(s->lp_list[i]);
            if (model_lp->model_types == NULL || !model_lp->model_types->vts_event_fn
                    || model_lp->model_types->num_vars <= 0)
                continue;
            //printf("%lu: Analysis LP %lu, model_lp: %lu\n", g_tw_mynode, lp->gid, model_lp->gid);
            vts_next_lp(model_lp->gid);

            model_lp->model_types->vts_event_fn(model_lp->cur_state, bf, model_lp);
        }

        vts_end_sample();
        s->model_samples_current = s->model_samples_current->next;
#ifdef USE_RISA
        if (g_st_risa_enabled)
        {
            vts_start_sample(sample);
            inst_sample(lp->pe, VT_INST, lp, 0);
            vts_end_sample(sample);
        }
#endif
    }

    // inst_sample() won't collect model data for VTS, if not using RISA
#ifdef USE_RISA
    if (!g_st_risa_enabled)
#endif
        inst_sample(lp->pe, VT_INST, lp, 0);

    st_create_sample_event(lp);
}

void analysis_event_rc(analysis_state *s, tw_bf *bf, analysis_msg *m, tw_lp *lp)
{
    tw_lp *model_lp;
    int j;

    lp->pe->stats.s_alp_e_rbs++;

#ifdef USE_RISA
    //if (g_st_risa_enabled)
    //{
//  //      s->event_id--;
    //    int kp_gid = (int)(g_tw_mynode * g_tw_nkp) + (int)lp->kp->id;
    //    //printf("[R-D] KP %d invalidating data with event_id %d\n", kp_gid, s->event_id);
    //    risa_invalidate_sample(tw_now(lp), kp_gid, s->event_id);
    //    //st_damaris_set_rc_data(m->rc_data, m->offset);
    //    //for (i = 0; i < s->num_lps; i++)
    //    //{
    //    //    model_lp = tw_getlocal_lp(s->lp_list[i]);
    //    //    if (model_lp->model_types == NULL || !model_lp->model_types->vts_revent_fn
    //    //            || model_lp->model_types->num_vars <= 0)
    //    //        continue;
    //    //    model_lp->model_types->vts_revent_fn(model_lp->cur_state, bf, model_lp);
    //    //}
    //    //st_damaris_delete_rc_data();
    //    //return;
    //}
#endif

    if ((model_modes[VT_INST]) && s->num_lps > 0)
    {
        //printf("PE %ld KP %lu: analysis_event_rc at time %f\n", g_tw_mynode, lp->kp->id, m->timestamp);
        // need to remove sample associated with this event from the list
        model_sample_data *sample;
        // start at end, because it's most likely closer to the timestamp we're looking for
        for (sample = s->model_samples_current->prev; sample != NULL; sample = sample->prev)
        {
            //sample = &s->model_samples[i];
            if (sample->vts == m->timestamp)
            {
#ifdef USE_RISA
                if (g_st_risa_enabled)
                {
                    int kp_gid = (int)(g_tw_mynode * g_tw_nkp) + (int)lp->kp->id;
                    risa_invalidate_sample(sample->vts, sample->rts, kp_gid);
                }
#endif
                vts_start_sample(sample);
                for (j = 0; j < s->num_lps; j++)
                {
                    if (s->lp_list[j] == ULLONG_MAX)
                        break;

                    // first call the appropriate RC fn, to allow it to undo any state changes
                    model_lp = tw_getlocal_lp(s->lp_list[j]);
                    if (model_lp->model_types == NULL || !model_lp->model_types->vts_revent_fn
                            || model_lp->model_types->num_vars <= 0)
                        continue;
                    vts_next_lp_rev(model_lp->gid);
                    model_lp->model_types->vts_revent_fn(model_lp->cur_state, bf, model_lp);
                }

                vts_end_sample();
                sample->vts = 0;
                sample->rts = 0;

                if (sample->prev)
                    sample->prev->next = sample->next;
                if (sample->next)
                    sample->next->prev = sample->prev;
                if (s->model_samples_head == sample)
                    s->model_samples_head = sample->next;
                if (s->model_samples_tail != sample)
                { // move this freed sample to the end of the list, so we don't have a memory leak
                    sample->prev = s->model_samples_tail;
                    sample->prev->next = sample;
                    sample->next = NULL;
                    s->model_samples_tail = sample;
                }


                break;
            }
        }
    }

}

void analysis_commit(analysis_state *s, tw_bf *bf, analysis_msg *m, tw_lp *lp)
{
    //printf("PE %ld KP %lu: analysis_commit at %f\n", g_tw_mynode, lp->kp->id, tw_now(lp));
    if ((model_modes[VT_INST]) && s->num_lps > 0)
    {
        // write committed data to buffer
        model_sample_data *sample;
        //int i, j;
        //tw_lp *model_lp;
        //lp_metadata metadata;
        // start at beginning
        for (sample = s->model_samples_head; sample != NULL; sample = sample->next)
        {
            if (sample->vts == m->timestamp)
            {
#ifdef USE_RISA
                if (g_st_risa_enabled)
                {
                    int kp_gid = (int)(g_tw_mynode * g_tw_nkp) + (int)lp->kp->id;
                    risa_validate_sample(sample->vts, sample->rts, kp_gid);
                }
                else
                {
#endif
                vts_start_sample(sample);
                inst_sample(lp->pe, VT_INST, lp, 1);
                vts_end_sample();
#ifdef USE_RISA
                }
#endif

                sample->vts = 0;
                sample->rts = 0;

                if (sample->prev)
                    sample->prev->next = sample->next;
                if (sample->next)
                    sample->next->prev = sample->prev;
                if (s->model_samples_head == sample)
                    s->model_samples_head = sample->next;
                if (s->model_samples_tail != sample)
                { // move this freed sample to the end of the list, so we don't have a memory leak
                    sample->prev = s->model_samples_tail;
                    sample->prev->next = sample;
                    sample->next = NULL;
                    s->model_samples_tail = sample;
                }

                break;
            }
        }
    }
}

void analysis_finish(analysis_state *s, tw_lp *lp)
{
    // TODO all samples should be written by the commit function, right?
    // Does anything actually need to be done here?
    if (s->model_samples_head != s->model_samples_current)
        printf("WARNING: there is model sample data that has not been committed!\n");
}

static void st_create_sample_event(tw_lp *lp)
{
    if (tw_now(lp) + g_st_vt_interval <= g_st_sampling_end)
    {
        tw_event *e = tw_event_new(lp->gid, g_st_vt_interval, lp);
        analysis_msg *m = (analysis_msg*) tw_event_data(e);
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

tw_lpid *get_sim_lp_list(analysis_state *s, int* num_lps)
{
    if (num_lps)
        *num_lps = s->num_lps_sim;
    return s->lp_list_sim;
}

size_t get_model_data_size(analysis_state *s, int* num_lps)
{
    if (num_lps)
        *num_lps = s->num_lps;
    return s->model_sample_size;
}
