#ifndef ST_MODEL_DATA_H
#define ST_MODEL_DATA_H

#include <instrumentation/st-instrumentation.h>
#include <instrumentation/ross-lps/analysis-lp.h>

//#define typename(x) _Generic((x),  \
//            int:     MODEL_INT,    \
//            long:    MODEL_LONG,   \
//            float:   MODEL_FLOAT,  \
//            double:  MODEL_DOUBLE, \
//            default: MODEL_OTHER)

st_model_types *g_st_model_types;

typedef struct st_model_data
{
    unsigned int kpid;
    unsigned int lpid;
    int num_vars;
} st_model_data;

typedef struct model_var_md
{
    int var_id;        /**< @brief id into st_model_types->var_names */
    int type;          /**< @brief data type for this model variable */
    int num_elems;     /**< @brief number of elements for this variable */
} model_var_md;

//typedef struct model_file_md
//{
//    size_t name_sz;
//    size_t name_id;
//} model_file_md;
//
//typedef struct model_var_file_md
//{
//    int model_type_id;
//    int var_id;
//    int type;
//    unsigned int num_elems;
//
//} model_var_file_md;

typedef struct model_buffer
{
    char* buffer;      /**< @brief pointer to start of model data in buffer */
    char* cur_pos;     /**< @brief pointer to free space for next variable */
    size_t cur_size;   /**< @brief space used so far */
    size_t total_size; /**< @brief total space allocated for this sample */
    int inst_mode;     /**< @brief current mode for sampling */
} model_buffer;

typedef struct vts_model_sample
{
    int in_progress;
    model_sample_data *cur_sample;
    model_lp_sample *cur_lp;
    model_var_data *cur_var;
} vts_model_sample;

void st_collect_model_data(tw_pe *pe, int inst_mode, char* buffer, size_t data_size);
void st_collect_model_data_vts(tw_pe *pe, tw_lp* lp, int inst_mode, char* buffer,
        sample_metadata *sample_md, size_t data_size);

int variable_id_lookup(tw_lp* lp, const char* var_name);
size_t calc_model_sample_size(int inst_mode, int *num_lps);
//size_t calc_model_sample_size_vts(int inst_mode, tw_lp* lp);
size_t model_var_type_size(st_model_typename type);
int get_num_model_lps(int inst_mode);
void vts_start_sample();
void vts_next_lp();
void vts_next_lp_rev(tw_lpid id);
void vts_end_sample();

#endif // ST_MODEL_DATA_H
