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

    // create list of LPs this is responsible for
    s->lp_list = (tw_lpid*)tw_calloc(TW_LOC, "analysis LPs", sizeof(tw_lpid), s->num_lps);
    s->lp_list_sim = (tw_lpid*)tw_calloc(TW_LOC, "analysis LPs", sizeof(tw_lpid), s->num_lps);
    // size of lp_list is max number of LPs this analysis LP is responsible for
    for (i = 0; i < s->num_lps; i++)
    {
        s->lp_list[i] = ULLONG_MAX;
        s->lp_list_sim[i] = ULLONG_MAX;
    }

    for (i = 0; (unsigned int)i < g_tw_nlp; i++)
    {
        cur_lp = g_tw_lp[i];

        if (cur_lp->kp->id == s->analysis_id % g_tw_nkp)
        {
            s->lp_list_sim[sim_idx] = cur_lp->gid;
            sim_idx++;

            // check if this LP even needs sampling performed
            if (cur_lp->model_types == NULL || cur_lp->model_types->sample_struct_sz == 0)
                continue;

            s->lp_list[idx] = cur_lp->gid;
            idx++;
        }
    }

    // update num_lps
    s->num_lps = idx;
    s->num_lps_sim = sim_idx;

    // setup memory to use for model samples
    if ((g_st_model_stats == VT_STATS || g_st_model_stats == ALL_STATS) && s->num_lps > 0)
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
            if (s->num_lps <= 0)
                tw_error(TW_LOC, "s->num_lps <= 0!");
            sample->lp_data = (void**) tw_calloc(TW_LOC, "analysis LPs", sizeof(void*), s->num_lps);
            for (j = 0; j < s->num_lps; j++)
            {
                cur_lp = tw_getlocal_lp(s->lp_list[j]);
                sample->lp_data[j] = (void *) tw_calloc(TW_LOC, "analysis LPs", cur_lp->model_types->sample_struct_sz, 1);
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

    if ((g_st_model_stats == VT_STATS || g_st_model_stats == ALL_STATS) && s->num_lps > 0)
    {
        model_sample_data *sample = s->model_samples_current;
        // TODO handle this situation better
        if (sample == s->model_samples_tail)
            printf("WARNING: last available sample space for analysis lp!\n");

        sample->timestamp = tw_now(lp);
        m->timestamp = tw_now(lp);

        // call the model sampling function for each LP on this KP
        for (i = 0; i < s->num_lps; i++)
        {
            if (s->lp_list[i] == ULLONG_MAX)
                break;

            model_lp = tw_getlocal_lp(s->lp_list[i]);
            if (model_lp->model_types == NULL || model_lp->model_types->sample_struct_sz == 0)
                continue;

            model_lp->model_types->sample_event_fn(model_lp->cur_state, bf, lp, sample->lp_data[i]);
        }

        s->model_samples_current = s->model_samples_current->next;
    }

    // sim engine sampling
    if (g_tw_synchronization_protocol != SEQUENTIAL &&
            (g_st_engine_stats == VT_STATS || g_st_engine_stats == ALL_STATS))
    {
#ifdef USE_DAMARIS
        if (g_st_damaris_enabled)
            st_damaris_expose_data(lp->pe, tw_now(lp), ANALYSIS_LP);
        else
            st_collect_engine_data(lp->pe, ANALYSIS_LP);
#else
        st_collect_engine_data(lp->pe, ANALYSIS_LP);
#endif
    }
        //collect_sim_engine_data(lp->pe, lp, s, (tw_stime) tw_clock_read() / g_tw_clock_rate);

    // create next sampling event
    st_create_sample_event(lp);
}

void analysis_event_rc(analysis_state *s, tw_bf *bf, analysis_msg *m, tw_lp *lp)
{
    tw_lp *model_lp;
    int j;

    lp->pe->stats.s_alp_e_rbs++;

    if ((g_st_model_stats == VT_STATS || g_st_model_stats == ALL_STATS) && s->num_lps > 0)
    {
        // need to remove sample associated with this event from the list
        model_sample_data *sample;
        // start at end, because it's most likely closer to the timestamp we're looking for
        for (sample = s->model_samples_current->prev; sample != NULL; sample = sample->prev)
        {
            //sample = &s->model_samples[i];
            if (TW_STIME_CMP(sample->timestamp, m->timestamp) == 0)
            {
                for (j = 0; j < s->num_lps; j++)
                {
                    if (s->lp_list[j] == ULLONG_MAX)
                        break;

                    // first call the appropriate RC fn, to allow it to undo any state changes
                    model_lp = tw_getlocal_lp(s->lp_list[j]);
                    if (model_lp->model_types == NULL || model_lp->model_types->sample_struct_sz == 0)
                        continue;
                    model_lp->model_types->sample_revent_fn(model_lp->cur_state, bf, lp, sample->lp_data[j]);
                }

                sample->timestamp = TW_STIME_CRT(0);

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
    (void) bf;
    (void) lp;
    if ((g_st_model_stats == VT_STATS || g_st_model_stats == ALL_STATS) && s->num_lps > 0)
    {
        // write committed data to buffer
        model_sample_data *sample;
        int j;
        tw_lp *model_lp;
        lp_metadata metadata;
        // start at beginning
        for (sample = s->model_samples_head; sample != NULL; sample = sample->next)
        {
            if (TW_STIME_CMP(sample->timestamp, m->timestamp) == 0)
            {
                for (j = 0; j < s->num_lps; j++)
                {
                    model_lp = tw_getlocal_lp(s->lp_list[j]);
                    if (model_lp->model_types == NULL || model_lp->model_types->sample_struct_sz == 0)
                        continue;
                    metadata.lpid = model_lp->gid;
                    metadata.kpid = model_lp->kp->id;
                    metadata.peid = model_lp->pe->id;
                    metadata.ts = m->timestamp;
                    metadata.sample_sz = model_lp->model_types->sample_struct_sz;
                    metadata.flag = MODEL_TYPE;

                    char buffer[sizeof(lp_metadata) + model_lp->model_types->sample_struct_sz];
                    memcpy(&buffer[0], (char*)&metadata, sizeof(lp_metadata));
                    memcpy(&buffer[sizeof(lp_metadata)], (char*)sample->lp_data[j], model_lp->model_types->sample_struct_sz);
                    if (g_tw_synchronization_protocol != SEQUENTIAL)
                        st_buffer_push(ANALYSIS_LP, &buffer[0], sizeof(lp_metadata) + model_lp->model_types->sample_struct_sz);
                    else if (g_tw_synchronization_protocol == SEQUENTIAL && !g_st_disable_out)
                        fwrite(buffer, sizeof(lp_metadata) + model_lp->model_types->sample_struct_sz, 1, seq_analysis);
                }

                sample->timestamp = TW_STIME_CRT(0);

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
    (void) lp;
    // TODO all samples should be written by the commit function, right?
    // Does anything actually need to be done here?
    if (s->model_samples_head != s->model_samples_current)
        printf("WARNING: there is model sample data that has not been committed!\n");
}

static void st_create_sample_event(tw_lp *lp)
{
    if (TW_STIME_DBL(tw_now(lp)) + g_st_vt_interval <= g_st_sampling_end)
    {
        tw_event *e = tw_event_new(lp->gid,TW_STIME_CRT(g_st_vt_interval), lp);
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
