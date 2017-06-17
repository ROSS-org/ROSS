#include <ross.h>
#include <time.h>

extern MPI_Comm MPI_COMM_ROSS;

static long missed_bytes = 0;
static MPI_Offset *prev_offsets = NULL;
static MPI_File *buffer_fh = NULL;
char stats_directory[INST_MAX_LENGTH];
int g_st_buffer_size = 8000000;
int g_st_buffer_free_percent = 15;
static int buffer_overflow_warned = 0;
static const char *file_suffix[NUM_COL_TYPES];
FILE *seq_ev_trace, *seq_model;

/* initialize circular buffer for stats collection
 * basically the read position marks the beginning of used space in the buffer
 * while the write postion marks the end of used space in the buffer
 */
st_stats_buffer *st_buffer_init(int type)
{
    int i;
    char filename[INST_MAX_LENGTH];
    file_suffix[0] = "gvt";
    file_suffix[1] = "rt";
    file_suffix[2] = "evtrace";
    file_suffix[3] = "model";
    
    st_stats_buffer *buffer = (st_stats_buffer*) tw_calloc(TW_LOC, "statistics collection (buffer)", sizeof(st_stats_buffer), 1);
    buffer->size  = g_st_buffer_size;
    buffer->write_pos = 0;
    buffer->read_pos = 0;
    buffer->count = 0;
    buffer->buffer = (char*) tw_calloc(TW_LOC, "statistics collection (buffer)", 1, buffer->size);

    if (!prev_offsets)
    {
        prev_offsets = (MPI_Offset*) tw_calloc(TW_LOC, "statistics collection (buffer)", sizeof(MPI_Offset), NUM_COL_TYPES); 
        for (i = 0; i < NUM_COL_TYPES; i++)
            prev_offsets[i] = 0;
    }

    if (!buffer_fh)
        buffer_fh = (MPI_File*) tw_calloc(TW_LOC, "statistics collection (buffer)", sizeof(MPI_File), NUM_COL_TYPES);

    // set up MPI File
    if (!g_st_disable_out)
    {
        if (!g_st_stats_out[0])
            sprintf(g_st_stats_out, "ross-stats");
        sprintf(filename, "%s/%s-%s.bin", stats_directory, g_st_stats_out, file_suffix[type]);
        if (g_tw_synchronization_protocol != SEQUENTIAL)
            MPI_File_open(MPI_COMM_ROSS, filename, MPI_MODE_CREATE | MPI_MODE_EXCL | MPI_MODE_WRONLY, MPI_INFO_NULL, &buffer_fh[type]);
        else if (strcmp(file_suffix[type], "evtrace") == 0 && g_tw_synchronization_protocol == SEQUENTIAL)
            seq_ev_trace = fopen(filename, "w");
        else if (strcmp(file_suffix[type], "model") == 0 && g_tw_synchronization_protocol == SEQUENTIAL)
            seq_model = fopen(filename, "w");
        
    }

    if (strcmp(file_suffix[type], "evtrace") == 0)
    {
        if (g_st_ev_trace == RB_TRACE || g_st_ev_trace == COMMIT_TRACE)
            g_st_buf_size = sizeof(tw_lpid) * 2 + sizeof(tw_stime) * 2;
        else if (g_st_ev_trace == FULL_TRACE)
            g_st_buf_size = sizeof(tw_lpid) * 2 + sizeof(tw_stime) * 4;
    }

    return buffer;
}

/* write stats to buffer
 * currently does not overwrite in cases of overflow, just records the amount of overflow in bytes
 * for later reporting
 */
void st_buffer_push(st_stats_buffer *buffer, char *data, int size)
{
    int size1, size2;
    if (!g_st_disable_out && st_buffer_free_space(buffer) < size)
    {
        if (!buffer_overflow_warned)
        {
            printf("WARNING: Stats buffer overflow on rank %lu\n", g_tw_mynode);
            buffer_overflow_warned = 1;
            printf("tw_now() = %f\n", tw_now(g_tw_lp[0]));
        }
        missed_bytes += size;
        size = 0; // if we can't push it all, don't push anything to buffer
    }

    if (size)
    {
        if ((size1 = buffer->size - buffer->write_pos) >= size)
        {
            // can use only one memcpy here
            memcpy(st_buffer_write_ptr(buffer), data, size);
            buffer->write_pos += size;
        }
        else // data to be stored wraps around end of physical array
        {
            size2 = size - size1;
            memcpy(st_buffer_write_ptr(buffer), data, size1);
            memcpy(buffer->buffer, data + size1, size2);
            buffer->write_pos = size2;
        }
    }
    buffer->count += size;
    //printf("wrote %d bytes to buffer; %d bytes of free space left\n", size, st_buffer_free_space(buffer));
}

/* determine whether to dump buffer to file */
void st_buffer_write(st_stats_buffer *buffer, int end_of_sim, int type)
{
    //TODO need metadata file
    MPI_Offset offset = prev_offsets[type];
    MPI_File *fh = &buffer_fh[type];
    int write_to_file = 0;
    int my_write_size = 0;
    int i;
    int write_sizes[tw_nnodes()];

    if ((double) st_buffer_free_space(buffer) / g_st_buffer_size < g_st_buffer_free_percent / 100.0 || end_of_sim)
    {
        //printf("free space %f, free percent %f\n", (double)st_buffer_free_space(buffer)/g_st_buffer_size, g_st_buffer_free_percent/100.0);
        my_write_size = buffer->count;
        write_to_file = 1;
    }

    MPI_Allgather(&my_write_size, 1, MPI_INT, &write_sizes[0], 1, MPI_INT, MPI_COMM_ROSS);
    for (i = 0; i < tw_nnodes(); i++)
    {
        if (i < g_tw_mynode)
            offset += write_sizes[i];
        prev_offsets[type] += write_sizes[i];
    };

    if (write_to_file)
    {
        //printf("rank %ld writing %d bytes at offset %lld\n", g_tw_mynode, my_write_size, offset);
        // dump buffer to file
        MPI_Status status;

        //MPI_Comm_split(MPI_COMM_ROSS, file_number, file_position, &file_comm);
        tw_clock start_cycle_time = tw_clock_read();;
        MPI_File_write_at(*fh, offset, st_buffer_read_ptr(buffer), my_write_size, MPI_BYTE, &status);
        g_st_stat_write_ctr += tw_clock_read() - start_cycle_time;

        // reset the buffer
        buffer->write_pos = 0;
        buffer->read_pos = 0;
        buffer->count = 0;
        buffer_overflow_warned = 0;
    }
}

void st_buffer_finalize(st_stats_buffer *buffer, int type)
{
    // check if any data needs to be written out
    if (!g_st_disable_out && buffer->count)
        st_buffer_write(buffer, 1, type);

    if (g_tw_mynode == g_tw_masternode)
        printf("There were %ld bytes of data missed because of buffer overflow\n", missed_bytes);

    MPI_File_close(&buffer_fh[type]);

}
