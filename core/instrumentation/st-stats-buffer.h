#ifndef ST_STATS_BUFFER_H
#define ST_STATS_BUFFER_H

typedef struct{
    char *buffer;
    int size;
    int write_pos;
    int read_pos;
    int count;
} st_stats_buffer;

char stats_directory[INST_MAX_LENGTH];

void st_buffer_allocate();
void st_buffer_init(int type);
void st_buffer_push(int type, char *data, int size);
char* st_buffer_pointer(int type, size_t size);
void st_buffer_write(int end_of_sim, int type);
void st_buffer_finalize(int type);
void write_file_metadata(int type);

#endif // ST_STATS_BUFFER_H
