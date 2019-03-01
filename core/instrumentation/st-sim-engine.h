#ifndef ST_SIM_ENGINE_H
#define ST_SIM_ENGINE_H

void st_collect_engine_data(tw_pe* pe, int col_type, char* buffer, size_t data_size, sample_metadata* sample_md, tw_lp* lp);
void st_collect_engine_data_pes(tw_pe* pe, tw_statistics* s, int col_type, st_pe_stats* pe_stats);
void st_collect_engine_data_kps(tw_pe* pe, tw_kp* kp, tw_statistics* s, int col_type, st_kp_stats* kp_stats);
void st_collect_engine_data_lps(tw_pe* pe, tw_lp* lp, tw_statistics* s, int col_type, st_lp_stats* lp_stats);
size_t calc_sim_engine_sample_size();
size_t calc_sim_engine_sample_size_vts(tw_lp* lp);

#endif // ST_SIM_ENGINE_H
