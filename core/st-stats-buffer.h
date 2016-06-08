#include <ross.h>

struct st_lp_stats{
    tw_clock time;
    tw_lpid id;
    tw_kpid kp_id;
    tw_peid pe_id;
    // add stats that need to be collected per LP
};

struct st_kp_stats{
    tw_clock time;
    tw_kpid id;
    tw_peid pe_id;
    // stats to be collected per KP
    tw_stime vt_ahead_gvt;
};

struct st_pe_stats{
    tw_clock time;
    tw_peid id;
    // stats to be collected per PE
};

/* use a union for each block in the buffer to store only one of these per block */
typedef union {
    st_lp_stats lp_stats;
    st_kp_stats kp_stats;
    st_pe_stats pe_stats;
} st_stats;

typedef enum { //st_block_type {
    ST_LP,
    ST_KP,
    ST_PE
} st_block_type;

/* struct for actually storing a block in the buffer */
struct st_buf_block{
    st_buf_block *next;
    st_block_type type;
    st_stats stats;
};


void st_buffer_init();
void st_buffer_push( );
void st_buffer_write();
