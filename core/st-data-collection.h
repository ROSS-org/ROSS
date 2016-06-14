#include <ross.h>

extern const tw_optdef *tw_stats_setup();
extern void tw_gvt_stats_file_setup(tw_peid id);
extern void tw_interval_stats_file_setup(tw_peid id);
extern char g_st_stats_out[128]; 
extern int g_st_stats_enabled;
extern long g_st_time_interval;
extern int g_st_pe_per_file;
extern int g_st_my_file_id;
extern MPI_File gvt_file;
extern MPI_File interval_file;
extern MPI_Comm  stats_comm;
extern int g_st_disable_out;

extern void get_time_ahead_GVT(tw_pe *me, tw_stime current_rt);
void st_stats_init();
