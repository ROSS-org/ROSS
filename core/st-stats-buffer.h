#include <ross.h>

#define st_buffer_free_space(buf) (buf->size - buf->count)
#define st_buffer_write_ptr(buf) (buf->buffer + buf->write_pos)
#define st_buffer_read_ptr(buf) (buf->buffer + buf->read_pos)

typedef struct{
    char *buffer;
    int size;
    int write_pos;
    int read_pos;
    int count;
} st_stats_buffer;

extern st_stats_buffer *g_st_buffer;
extern char g_st_directory[13];

st_stats_buffer *st_buffer_init(int size);
void st_buffer_push(st_stats_buffer *buffer, char *data, int size);
void st_buffer_write(st_stats_buffer *buffer, int end_of_sim);
void st_buffer_finalize(st_stats_buffer *buffer);
