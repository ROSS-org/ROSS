#include <ross.h>

st_stats_buffer *g_st_buffer = NULL;
static long missed_bytes = 0;
static MPI_File fh;

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
    // TODO make name configurable
    char filename[100];
    sprintf(filename, "ross-stats.bin");
    MPI_File_open(MPI_COMM_WORLD, filename, MPI_MODE_CREATE | MPI_MODE_WRONLY, MPI_INFO_NULL, &fh);

    return buffer;
}

/* write stats to buffer 
 * currently does not overwrite in cases of overflow, just records the amount of overflow in bytes
 * for later reporting
 */
void st_buffer_push(st_stats_buffer *buffer, char *data, int size) 
{
    int size1, size2;
    if (st_buffer_free_space(buffer) < size)
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
            buffer->write_pos = size;
        }
        else // data to be stored wraps around end of physical array
        {
            size2 = size - size1;
            memcpy(st_buffer_write_ptr(buffer), data, size1);
            memcpy(buffer->buffer, data + size1 + 1, size2);
            buffer->write_pos = size2;
        }
    }
    buffer->count += size;
}

/* determine whether to dump buffer to file */
void st_buffer_write(st_stats_buffer *buffer, int end_of_sim)
{
    //TODO need metadata file
    MPI_Offset offset = 0;
    int write_to_file = 0;
    int my_write_size = 0;

    if ((double) st_buffer_free_space(buffer) < .1 || end_of_sim)
    {
        my_write_size = buffer->count;
        write_to_file = 1;
    }

    MPI_Exscan(&my_write_size, &offset, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);

    if (write_to_file)
    {
        // dump buffer to file
        MPI_Status status;

        // for now write everything to same file, maybe later make it so you can split files?
        // TODO actually make one file each for PE, KP, and LP data?
        // also need to create (human readable) metadata file in order to read the binary file
        //MPI_Comm_split(MPI_COMM_WORLD, file_number, file_position, &file_comm);
        MPI_File_write_at(fh, offset, st_buffer_read_ptr(buffer), my_write_size, MPI_BYTE, &status);

        // reset the buffer
        buffer->write_pos = 0;
        buffer->read_pos = 0;
        buffer->count = 0;
    }
}

void st_buffer_finalize(st_stats_buffer *buffer)
{
    // check if any data needs to be written out
    if (buffer->count)
        st_buffer_write(buffer, 1);

    if (g_tw_mynode == g_tw_masternode)
        printf("There were %ld bytes of data missed because of buffer overflow\n", missed_bytes);
    
    // close MPI file
    MPI_File_close(&fh);

}
