#include <ross.h>

//TODO probably make this configurable
#define BUFFER_SIZE 8000000

st_buf_block *g_st_buffer = NULL;
st_buf_block *g_st_buf_end = NULL;
st_buf_block *g_st_buf_read = NULL; // TODO probably unnecessary
st_buf_block *g_st_buf_write = NULL;
static int passed_buf_end = 0;
static long missed_blocks = 0;
static long total_blocks = 0;
static MPI_File fh;

/* initialize circular buffer for stats collection */
void st_buffer_init()
{
    total_blocks = (long) BUFFER_SIZE / sizeof(st_buf_block);
    // start off by creating 8 MB of memory for buffer
    g_st_buffer = tw_calloc(TW_LOC, "statistics collection (buffer)", sizeof(st_buf_block), total_blocks);

    // start/end pointers should never change after this
    g_st_buf_end = g_st_buffer + total_blocks;

    // set up read/write pointers
    g_st_buf_read = g_st_buffer;
    g_st_buf_write = g_st_buffer;

    // set up MPI File
    // TODO make name configurable
    char filename[100];
    sprintf(filename, "ross-stats.bin");
    MPI_File_open(MPI_COMM_WORLD, filename, MPI_MODE_CREATE | MPI_MODE_WRONLY, MPI_INFO_NULL, &fh);
}

/* write stats to buffer */
// TODO need to add in missing data reporting
void st_buffer_push(st_block_type type, st_stats *stats)
{
    if (!passed_buf_end)
    {
        // copy to buffer at location of write ptr
        switch(type)
        {
            case ST_LP:
                memcpy(&(g_st_buf_write->stats.lp_stats), stats, sizeof(st_lp_stats));
                break;
            case ST_KP:
                memcpy(&(g_st_buf_write->stats.kp_stats), stats, sizeof(st_kp_stats));
                break;
            case ST_PE:
                memcpy(&(g_st_buf_write->stats.pe_stats), stats, sizeof(st_pe_stats));
                break;
        }
    }
    else
        missed_blocks++;
    // check to see if write ptr passes end of buffer for monitoring data loss 
    if (g_st_buf_write == g_st_buf_end)
    {
        passed_buf_end = 1;
        g_st_buf_write = g_st_buffer;
        printf("WARNING: Stats buffer overflow on rank %lu\n", g_tw_mynode);
    }
    else
        g_st_buf_write++;
}

/* determine whether to dump buffer to file */
void st_buffer_write(int end_of_sim)
{
    //TODO need metadata file
    MPI_Offset offset = 0;
    int write_to_file = 0;
    long my_write_size = 0;
    long free_blocks = g_st_buf_end - g_st_buf_write;

    if ((double)free_blocks / total_blocks < .25 || end_of_sim)
    {
        my_write_size = (total_blocks-free_blocks) * sizeof(st_buf_block);
        write_to_file = 1;
    }

    MPI_Exscan(&my_write_size, &offset, 1, MPI_LONG, MPI_SUM, MPI_COMM_WORLD);
    if (write_to_file)
    {
        // dump buffer to file
        MPI_Status status;

        // for now write everything to same file, maybe later make it so you can split files?
        // TODO actually make one file each for PE, KP, and LP data?
        // also need to create (human readable) metadata file in order to read the binary file
        //MPI_Comm_split(MPI_COMM_WORLD, file_number, file_position, &file_comm);
        MPI_File_write_at(fh, offset, g_st_buffer, my_write_size, MPI_BYTE, &status);

        // reset the buffer
        passed_buf_end = 0;
        g_st_buf_write = g_st_buffer;
    }
}

void st_buffer_finalize()
{
    // check if any data needs to be written out
    if (g_st_buf_write != g_st_buffer)
        st_buffer_write(1);

    if (g_tw_mynode == g_tw_masternode)
        printf("There were %ld blocks of data missed because of buffer overflow\n", missed_blocks);
    
    // close MPI file
    MPI_File_close(&fh);

}
