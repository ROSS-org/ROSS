#include <ross.h>

typedef struct{
    tw_stime time;
    tw_lpid id;
    tw_kpid kp_id;
    tw_peid pe_id;
    // add stats that need to be collected per LP
} st_lp_stats;

typedef struct{
    tw_stime time;
    tw_kpid id;
    tw_peid pe_id;
    // stats to be collected per KP
    tw_stime vt_ahead_gvt;
} st_kp_stats;

typedef struct{
    tw_stime time;
    tw_peid id;
    // stats to be collected per PE
} st_pe_stats;

/* use a union for each block in the buffer to store only one of these per block */
typedef union{
    st_lp_stats lp_stats;
    st_kp_stats kp_stats;
    st_pe_stats pe_stats;
} st_stats;

typedef enum{
    ST_LP,
    ST_KP,
    ST_PE
} st_block_type;

/* struct for actually storing a block in the buffer */
typedef struct{
    //st_buf_block *next;
    st_block_type type;
    st_stats stats;
} st_buf_block;

extern st_buf_block *g_st_buffer;
extern st_buf_block *g_st_buf_end;
extern st_buf_block *g_st_buf_read;
extern st_buf_block *g_st_buf_write;

void st_buffer_init();
void st_buffer_push(st_block_type type, st_stats *stats);
void st_buffer_write(int end_of_sim);
void st_buffer_finalize();
