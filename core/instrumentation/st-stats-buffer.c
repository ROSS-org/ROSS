#include <ross.h>
#include <time.h>
#include <sys/stat.h>

static long missed_bytes = 0;
static MPI_Offset *prev_offsets = NULL;
static MPI_File *buffer_fh = NULL;
char stats_directory[INST_MAX_LENGTH];
int g_st_buffer_size = 8000000;
int g_st_buffer_free_percent = 15;
static int buffer_overflow_warned = 0;
static const char *file_suffix[NUM_COL_TYPES];
FILE *seq_ev_trace, *seq_model, *seq_analysis;
static st_stats_buffer **g_st_buffer;

void st_buffer_allocate()
{
    if (!(g_st_engine_stats || g_st_model_stats || g_st_ev_trace || g_st_use_analysis_lps))
        return;

    int i, rc;

    // setup directory for instrumentation output
    if (g_tw_mynode == g_tw_masternode)
    {
        if (!g_st_stats_path[0])
            sprintf(g_st_stats_path, "stats-output");
        rc = mkdir(g_st_stats_path, S_IRUSR | S_IWUSR | S_IXUSR);
        if (rc == -1)
        {
	  // this check gets rid of the GCC warning about trunciated string inputs
	  if( snprintf(stats_directory, sizeof(stats_directory), "%s-%ld-%ld", g_st_stats_path, (long)getpid(), (long)time(NULL)) ==
	      sizeof(stats_directory)  )
	    {
	      printf("Error in st_buffer_allocate: stats_directory name lacked sufficient space and was truncaited\n");
	      exit(-1);
	    }
	  mkdir(stats_directory, S_IRUSR | S_IWUSR | S_IXUSR);
        }
        else
            sprintf(stats_directory, "%s", g_st_stats_path);
    }

    // make sure everyone has the directory name
    MPI_Bcast(stats_directory, INST_MAX_LENGTH, MPI_CHAR, g_tw_masternode, MPI_COMM_ROSS);

    // allocate buffer pointers
    g_st_buffer = (st_stats_buffer**) tw_calloc(TW_LOC, "instrumentation (buffer)", sizeof(st_stats_buffer*), NUM_COL_TYPES);

    // setup MPI file offsets
    if (!prev_offsets)
    {
        prev_offsets = (MPI_Offset*) tw_calloc(TW_LOC, "statistics collection (buffer)", sizeof(MPI_Offset), NUM_COL_TYPES);
        for (i = 0; i < NUM_COL_TYPES; i++)
            prev_offsets[i] = 0;
    }

    // set up file handlers
    if (!buffer_fh)
        buffer_fh = (MPI_File*) tw_calloc(TW_LOC, "statistics collection (buffer)", sizeof(MPI_File), NUM_COL_TYPES);

}

/* initialize circular buffer for stats collection
 * basically the read position marks the beginning of used space in the buffer
 * while the write postion marks the end of used space in the buffer
 */
void st_buffer_init(int type)
{
    char filename[INST_MAX_LENGTH];
    file_suffix[0] = "gvt";
    file_suffix[1] = "rt";
    file_suffix[2] = "analysis-lps";
    file_suffix[3] = "evtrace";
    file_suffix[4] = "model";

    g_st_buffer[type] = (st_stats_buffer*) tw_calloc(TW_LOC, "statistics collection (buffer)", sizeof(st_stats_buffer), 1);
    g_st_buffer[type]->size  = g_st_buffer_size;
    g_st_buffer[type]->write_pos = 0;
    g_st_buffer[type]->read_pos = 0;
    g_st_buffer[type]->count = 0;
    g_st_buffer[type]->buffer = (char*) tw_calloc(TW_LOC, "statistics collection (buffer)", 1, g_st_buffer[type]->size);

    // set up MPI File
    if (!g_st_disable_out)
    {
        if (!g_st_stats_out[0])
            sprintf(g_st_stats_out, "ross-stats");
	// this check gets rid of the GCC warning about trunciated string inputs
        if( snprintf(filename, sizeof(filename), "%s/%s-%s.bin", stats_directory, g_st_stats_out, file_suffix[type]) ==
	    sizeof(filename))
	  {
	    printf("Error in st_buffer_init: filename lacked sufficient space and was truncaited\n");
	    exit(-1);
	  }
        if (g_tw_synchronization_protocol != SEQUENTIAL)
            MPI_File_open(MPI_COMM_ROSS, filename, MPI_MODE_CREATE | MPI_MODE_EXCL | MPI_MODE_WRONLY, MPI_INFO_NULL, &buffer_fh[type]);
        else if (strcmp(file_suffix[type], "evtrace") == 0 && g_tw_synchronization_protocol == SEQUENTIAL)
            seq_ev_trace = fopen(filename, "w");
        else if (strcmp(file_suffix[type], "model") == 0 && g_tw_synchronization_protocol == SEQUENTIAL)
            seq_model = fopen(filename, "w");
        else if (type == ANALYSIS_LP && g_tw_synchronization_protocol == SEQUENTIAL)
            seq_analysis = fopen(filename, "w");

    }
}

/* write stats to buffer
 * currently does not overwrite in cases of overflow, just records the amount of overflow in bytes
 * for later reporting
 */
void st_buffer_push(int type, char *data, int size)
{
    int size1, size2;
    if (!g_st_disable_out && st_buffer_free_space(g_st_buffer[type]) < size)
    {
        if (!buffer_overflow_warned)
        {
            printf("WARNING: Stats buffer overflow on rank %lu\n", g_tw_mynode);
            buffer_overflow_warned = 1;
            printf("tw_now() = %f\n", TW_STIME_DBL(tw_now(g_tw_lp[0])));
        }
        missed_bytes += size;
        size = 0; // if we can't push it all, don't push anything to buffer
    }

    if (size)
    {
        if ((size1 = g_st_buffer[type]->size - g_st_buffer[type]->write_pos) >= size)
        {
            // can use only one memcpy here
            memcpy(st_buffer_write_ptr(g_st_buffer[type]), data, size);
            g_st_buffer[type]->write_pos += size;
        }
        else // data to be stored wraps around end of physical array
        {
            size2 = size - size1;
            memcpy(st_buffer_write_ptr(g_st_buffer[type]), data, size1);
            memcpy(g_st_buffer[type]->buffer, data + size1, size2);
            g_st_buffer[type]->write_pos = size2;
        }
    }
    g_st_buffer[type]->count += size;
    //printf("PE %ld wrote %d bytes to buffer; %d bytes of free space left\n", g_tw_mynode, size, st_buffer_free_space(g_st_buffer[type]));
}

/* determine whether to dump buffer to file
 * should only be called at GVT! */
void st_buffer_write(int end_of_sim, int type)
{
    MPI_Offset offset = prev_offsets[type];
    MPI_File *fh = &buffer_fh[type];
    int write_to_file = 0;
    int my_write_size = 0;
    unsigned int i;
    int write_sizes[tw_nnodes()];
    tw_clock start_cycle_time = tw_clock_read();

    my_write_size = g_st_buffer[type]->count;

    MPI_Allgather(&my_write_size, 1, MPI_INT, &write_sizes[0], 1, MPI_INT, MPI_COMM_ROSS);
    if (end_of_sim)
        write_to_file = 1;
    else
    {
        for (i = 0; i < tw_nnodes(); i++)
        {
            if ((double) write_sizes[i] / g_st_buffer_size >= g_st_buffer_free_percent / 100.0)
                write_to_file = 1;
        }
    }

    if (write_to_file)
    {
        for (i = 0; i < tw_nnodes(); i++)
        {
            if (i < g_tw_mynode)
                offset += write_sizes[i];
            prev_offsets[type] += write_sizes[i];
        }
        //printf("rank %ld writing %d bytes at offset %lld (prev_offsets[ANALYSIS_LP] = %lld)\n", g_tw_mynode, my_write_size, offset, prev_offsets[type]);
        // dump buffer to file
        MPI_Status status;
        g_tw_pe->stats.s_stat_comp += tw_clock_read() - start_cycle_time;
        start_cycle_time = tw_clock_read();
        MPI_File_write_at_all(*fh, offset, st_buffer_read_ptr(g_st_buffer[type]), my_write_size, MPI_BYTE, &status);
        g_tw_pe->stats.s_stat_write += tw_clock_read() - start_cycle_time;

        // reset the buffer
        g_st_buffer[type]->write_pos = 0;
        g_st_buffer[type]->read_pos = 0;
        g_st_buffer[type]->count = 0;
        buffer_overflow_warned = 0;
    }
    else
        g_tw_pe->stats.s_stat_comp += tw_clock_read() - start_cycle_time;
}

/* make sure we write out any remaining buffer data */
void st_buffer_finalize(int type)
{
    // check if any data needs to be written out
    if (!g_st_disable_out)
        st_buffer_write(1, type);

    printf("PE %ld: There were %ld bytes of data missed because of buffer overflow\n", g_tw_mynode, missed_bytes);

    MPI_File_close(&buffer_fh[type]);

}
