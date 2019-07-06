#ifndef ST_SIM_ENGINE_H
#define ST_SIM_ENGINE_H

/**
 * @brief Collect simulation engine data
 * @param[in] pe Pointer to the PE
 * @param[in] col_type Instrumentation mode
 * @param[in] buffer Pointer to the buffer for storing data
 * @param[in] data_size Size of the sim engine sample
 * @param[in] sample_md Pointer to this sample's metadata
 * @param[in] lp Pointer to Analysis LP, only used for VTS
 */
void st_collect_engine_data(tw_pe* pe, int col_type, char* buffer, size_t data_size, sample_metadata* sample_md, tw_lp* lp);

/**
 * @brief Collect PE data
 * @param[in] pe Pointer to the PE
 * @param[in] s Pointer to statistics that have been collected for this PE
 * @param[in] col_type Instrumentation mode
 * @param[in] pe_stats Struct to store PE Instrumentation data in
 */
void st_collect_engine_data_pes(tw_pe* pe, tw_statistics* s, int col_type, st_pe_stats* pe_stats);

/**
 * @brief Collect KP data
 * @param[in] pe Pointer to the PE
 * @param[in] kp Pointer to the KP
 * @param[in] s Pointer to statistics that have been collected for this PE
 * @param[in] col_type Instrumentation mode
 * @param[in] kp_stats Struct to store KP Instrumentation data in
 */
void st_collect_engine_data_kps(tw_pe* pe, tw_kp* kp, tw_statistics* s, int col_type, st_kp_stats* kp_stats);

/**
 * @brief Collect LP data
 * @param[in] pe Pointer to the PE
 * @param[in] lp Pointer to the LP
 * @param[in] s Pointer to statistics that have been collected for this PE
 * @param[in] col_type Instrumentation mode
 * @param[in] lp_stats Struct to store LP Instrumentation data in
 */
void st_collect_engine_data_lps(tw_pe* pe, tw_lp* lp, tw_statistics* s, int col_type, st_lp_stats* lp_stats);

/**
 * @brief Calculate the size of a simulation engine sample for GVT or RT sampling modes
 * @return size of a single sample
 */
size_t calc_sim_engine_sample_size(void);

/**
 * @brief Calculate the size of a simulation engine sample for VT sampling mode
 * @return size of a single sample
 */
size_t calc_sim_engine_sample_size_vts(tw_lp* lp);

size_t calc_lvt_sample_size();
void st_collect_lvt_data(char* buffer, size_t data_size, sample_metadata* sample_md);


#endif // ST_SIM_ENGINE_H
