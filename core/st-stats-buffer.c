#include <ross.h>
#include <sys/stat.h>

st_stats_buffer *g_st_buffer = NULL;
static long missed_bytes = 0;
static MPI_File fh;
static MPI_Offset prev_offset = 0;
static char st_directory[13];

/* initialize circular buffer for stats collection 
 * basically the read position marks the beginning of used space in the buffer
 * while the write postion marks the end of used space in the buffer
 */
st_stats_buffer *st_buffer_init(int size)
{
    st_stats_buffer *buffer = tw_calloc(TW_LOC, "statistifcs collection (buffer)", sizeof(st_stats_buffer), 1);
    buffer->size  = size;
    buffer->write_pos = 0;
    buffer->read_pos = 0;
    buffer->count = 0;
    buffer->buffer = tw_calloc(TW_LOC, "statistics collection (buffer)", 1, buffer->size);

    // set up MPI File
    if (!g_st_disable_out)
    {
        sprintf(st_directory, "stats-output");
        mkdir(st_directory, S_IRUSR | S_IWUSR | S_IXUSR);
        char filename[100];
        if (g_st_stats_out[0])
            sprintf(filename, "%s/%s.bin", st_directory, g_st_stats_out);
        else
            sprintf(filename, "%s/ross-stats.bin", st_directory);
        MPI_File_open(MPI_COMM_WORLD, filename, MPI_MODE_CREATE | MPI_MODE_WRONLY, MPI_INFO_NULL, &fh);
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
        printf("WARNING: Stats buffer overflow on rank %lu\n", g_tw_mynode);
        missed_bytes += size - st_buffer_free_space(buffer);
        size = st_buffer_free_space(buffer);
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
            memcpy(buffer->buffer, data + size1 + 1, size2);
            buffer->write_pos += size2;
        }
    }
    buffer->count += size;
}

/* determine whether to dump buffer to file */
void st_buffer_write(st_stats_buffer *buffer, int end_of_sim)
{
    //TODO need metadata file
    MPI_Offset offset = prev_offset;
    int write_to_file = 0;
    int my_write_size = 0;
    int i;
    int write_sizes[tw_nnodes()];

    if ((double) st_buffer_free_space(buffer) < .15 || end_of_sim)
    {
        my_write_size = buffer->count;
        write_to_file = 1;
    }

    MPI_Allgather(&my_write_size, 1, MPI_INT, &write_sizes[0], 1, MPI_INT, MPI_COMM_WORLD);
    for (i = 0; i < tw_nnodes(); i++)
    {
        if (i < g_tw_mynode)
            offset += write_sizes[i];
        prev_offset += write_sizes[i];
    };
    //MPI_Exscan(&my_write_size, &offset, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);

    if (write_to_file)
    {
        //printf("rank %ld writing %d bytes at offset %lld\n", g_tw_mynode, my_write_size, offset);
        // dump buffer to file
        MPI_Status status;

        //MPI_Comm_split(MPI_COMM_WORLD, file_number, file_position, &file_comm);
        tw_clock start_cycle_time = tw_clock_read();;
        MPI_File_write_at(fh, offset, st_buffer_read_ptr(buffer), my_write_size, MPI_BYTE, &status);
        stat_write_cycle_counter += tw_clock_read() - start_cycle_time;

        // reset the buffer
        buffer->write_pos = 0;
        buffer->read_pos = 0;
        buffer->count = 0;
    }
}

void st_buffer_finalize(st_stats_buffer *buffer)
{
    // check if any data needs to be written out
    if (!g_st_disable_out && buffer->count)
        st_buffer_write(buffer, 1);

    if (g_tw_mynode == g_tw_masternode)
        printf("There were %ld bytes of data missed because of buffer overflow\n", missed_bytes);
    
    // close MPI file
    MPI_File_close(&fh);

}
