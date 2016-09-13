#include <ross.h>

#define st_buffer_free_space(buf) (buf->size - buf->count)
#define st_buffer_write_ptr(buf) (buf->buffer + buf->write_pos)
#define st_buffer_read_ptr(buf) (buf->buffer + buf->read_pos)

typedef enum{
    GVT_COL,
    RT_COL,
    EV_TRACE
} collection_types;

typedef enum{
    NO_TRACE,
    FULL_TRACE,
    RB_TRACE
} traces_enum;

typedef struct{
    char *buffer;
    int size;
    int write_pos;
    int read_pos;
    int count;
} st_stats_buffer;

extern st_stats_buffer *g_st_buffer_gvt;
extern st_stats_buffer *g_st_buffer_rt;
extern st_stats_buffer *g_st_buffer_evrb;
extern char g_st_directory[13];
extern int g_st_buffer_size;
extern int g_st_buffer_free_percent;
extern MPI_File g_st_gvt_fh;
extern MPI_File g_st_rt_fh;
extern MPI_File g_st_evrb_fh;

st_stats_buffer *st_buffer_init(char *suffix, MPI_File *fh);
void st_buffer_push(st_stats_buffer *buffer, char *data, int size);
void st_buffer_write(st_stats_buffer *buffer, int end_of_sim, int type);
void st_buffer_finalize(st_stats_buffer *buffer, int type);
