#ifndef ST_MODEL_DATA_H
#define ST_MODEL_DATA_H

#include <instrumentation/st-instrumentation.h>
#include <instrumentation/ross-lps/analysis-lp.h>

extern st_model_types *g_st_model_types;

#define lp_name_len 16

/**
 * @brief Stores data about a model LP
 */
typedef struct st_model_data
{
    char lp_name[lp_name_len]; /**< @brief user specified name of LP, limited to first lp_name_len chars */
    unsigned int kpid;
    unsigned int lpid;
    int num_vars;              /**< @brief user specified number of model variables for this LP */
} st_model_data;

/**
 * @brief Stores metadata about a given variable for a model LP
 */
typedef struct model_var_md
{
    int var_id;        /**< @brief id into st_model_types->var_names */
    int type;          /**< @brief data type for this model variable */
    int num_elems;     /**< @brief number of elements for this variable */
} model_var_md;

/**
 * @brief Used to keep track of current buffer position when making calls to model's callback function
 *
 * Note: GVT-based and RT sampling modes only
 */
typedef struct model_buffer
{
    char* buffer;      /**< @brief pointer to start of model data in buffer */
    char* cur_pos;     /**< @brief pointer to free space for next variable */
    size_t cur_size;   /**< @brief space used so far */
    size_t total_size; /**< @brief total space allocated for this sample */
    int inst_mode;     /**< @brief current mode for sampling */
} model_buffer;

/**
 * @brief Used to keep track of samples for VTS when making calls to model's callback function
 */
typedef struct vts_model_sample
{
    int in_progress;               /**< @brief Is a VTS sample in progress? */
    model_sample_data *cur_sample; /**< @brief Current sample from Analysis LP */
    model_lp_sample *cur_lp;       /**< @brief Data for LP currently being sampled */
    model_var_data *cur_var;       /**< @brief Data for variable currently being sampled */
} vts_model_sample;

/**
 * @brief Collect model data at sampling time on this PE for GVT and RT sampling modes.
 *
 * @param[in] pe Pointer to the PE
 * @param[in] inst_mode Instrumentation mode
 * @param[in] buffer pointer to the buffer address for storing sample data
 * @param[in] data_size size of the model sample
 */
void st_collect_model_data(tw_pe *pe, int inst_mode, char* buffer, size_t data_size);

/**
 * @brief Collect model data at sampling time on this PE for VT sampling only.
 *
 * To support rollback, the data is not stored in the buffer at this point.
 * It is kept in the Analysis LP's state, until either rollback (where it is discarded),
 * or commit (where it is actually saved to the buffer).
 *
 * @param[in] pe Pointer to the PE
 * @param[in] lp Pointer to the Analysis LP
 * @param[in] inst_mode Instrumentation mode
 * @param[in] buffer pointer to the buffer address for storing sample data
 * @param[in] data_size size of the model sample
 */
void st_collect_model_data_vts(tw_pe *pe, tw_lp* lp, int inst_mode, char* buffer,
        sample_metadata *sample_md, size_t data_size);

/**
 * @brief Look up the id for a variable
 * @param[in] lp pointer to the model LP
 * @param[in] var_name name of the variable to look up on this LP
 * @return id of the variable on the given LP
 */
int variable_id_lookup(tw_lp* lp, const char* var_name);

/**
 * @brief Calculate the size of a single model sample on this PE for GVT or RT sampling.
 *
 * @param[in] inst_mode Instrumentation mode
 * @param[out] num_lps Number of LPs that have model data collected
 * @return size of the model sample to be stored in the buffer
 */
size_t calc_model_sample_size(int inst_mode, int *num_lps);

/**
 * @brief Get size of the type for a model variable
 *
 * @param[in] type type of model variable
 * @return size of the model variable's type
 */
size_t model_var_type_size(st_model_typename type);

/**
 * @brief Needs to be called before starting a VT sample
 * @param[in] sample pointer to the sample location stored in the Analysis LP
 */
void vts_start_sample(model_sample_data* sample);

/**
 * @brief Update pointer to the next LP
 * @param[in] lp 
 */
void vts_next_lp(tw_lpid id);

/**
 * @brief Update pointer to the next LP when undoing a sample in reverse computation
 * @param[in] id global id of the next LP to return model data for RC
 */
void vts_next_lp_rev(tw_lpid id);

/**
 * @brief Cleans up pointers and flags at the end of a VT sample
 */
void vts_end_sample();

#endif // ST_MODEL_DATA_H
