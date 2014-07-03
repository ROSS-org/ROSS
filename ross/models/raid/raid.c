/**
 * RAID File Server Farm Simulation
 *
 * FILE: raid.c
 * AUTHOR: Matt Perry
 * DATE: Apr 27 2011
 */

#include "raid.h"

void message_error( Event event_type )
{
    printf( "Unexpected Message Type (%d)\n", event_type );
    exit( -1 );
}

tw_peid raid_mapping( tw_lpid gid )
{
    return (tw_peid) gid / g_tw_nlp;
}

tw_lptype mylps[] =
{
    // IO LP
    {
        (init_f) raid_io_init,
        (pre_run_f) NULL,
        (event_f) raid_io_eventhandler,
        (revent_f) raid_io_eventhandler_rc,
        (final_f) raid_io_finish,
        (map_f) raid_mapping,
        sizeof( IOState )
    },
    // File Server LP
    {
        (init_f) raid_server_init,
        (pre_run_f) NULL,
        (event_f) raid_server_eventhandler,
        (revent_f) raid_server_eventhandler_rc,
        (final_f) raid_server_finish,
        (map_f) raid_mapping,
        sizeof( ServerState )
    },
    // RAID Controller LP
    {
        (init_f) raid_controller_init,
        (pre_run_f) NULL,
        (event_f) raid_controller_eventhandler,
        (revent_f) raid_controller_eventhandler_rc,
        (final_f) raid_controller_finish,
        (map_f) raid_mapping,
        sizeof( ContState )
    },
    // Hard Disk LP
    {
        (init_f) raid_disk_init,
        (pre_run_f) NULL,
        (event_f) raid_disk_eventhandler,
        (revent_f) raid_disk_eventhandler_rc,
        (final_f) raid_disk_finish,
        (map_f) raid_mapping,
        sizeof( DiskState )
    },
    {0},
};

const tw_optdef app_opts[] =
{
    TWOPT_GROUP( "RAIDSIM Model" ),
    TWOPT_UINT( "disk-distro", g_disk_distro, "Failure probability distribution" ),
    TWOPT_UINT( "nfs", g_ttl_fs, "Number of file server clusters" ),
    TWOPT_UINT( "memory", g_add_mem, "Additional memory buffers" ),
    TWOPT_END()
};

int main( int argc, char** argv )
{
    int i,j;
    
    // Initialize ROSS environment
    tw_opt_add( app_opts );
    tw_init( &argc, &argv );

    // Calculate number of LPs per PE
    g_nfs_per_pe = g_ttl_fs / ( tw_nnodes() * g_tw_npe );
    g_nlp_per_pe = g_nfs_per_pe * LPS_PER_FS;

    // Set up event memory
    g_tw_memory_nqueues = 16;
    g_raid_start_events = 16;
    g_tw_events_per_pe = ( g_nlp_per_pe * g_raid_start_events ) + g_add_mem;

    // Define the LPs
    tw_define_lps( g_nlp_per_pe, sizeof( MsgData ), 0 );

    for( i = 0; i < g_nfs_per_pe; ++i )
    {
        for( j = 0; j < FS_PER_IO; ++j )
            tw_lp_settype( i * LPS_PER_FS + j, &mylps[0] );
        tw_lp_settype( i * LPS_PER_FS + FS_PER_IO, &mylps[1] );
        for( j = 0; j < CONT_PER_FS; ++j )
            tw_lp_settype( i * LPS_PER_FS + FS_PER_IO + 1 + j, &mylps[2] );
        for( j = 0; j < CONT_PER_FS * DISK_PER_RC; ++j )
            tw_lp_settype( i * LPS_PER_FS + FS_PER_IO + 1 + CONT_PER_FS + j, &mylps[3] );
    }    

    // Initialize stats
    g_stats.ttl_disk_failures = 0;
    g_stats.ttl_controller_rebuilds = 0;
    g_stats.ttl_controller_failures = 0;
    g_stats.ttl_blocks_wr = 0;
    g_stats.ttl_idle_time = 0;
    g_stats.ttl_busy_time = 0;

    tw_run();

    // Aggregate stats
    Stats agg_stats;
    MPI_Reduce( &(g_stats.ttl_disk_failures),
                &(agg_stats.ttl_disk_failures),
                1,
                MPI_INT,
                MPI_SUM,
                g_tw_masternode,
                MPI_COMM_WORLD );
    MPI_Reduce( &(g_stats.ttl_controller_rebuilds),
                &(agg_stats.ttl_controller_rebuilds),
                1,
                MPI_INT,
                MPI_SUM,
                g_tw_masternode,
                MPI_COMM_WORLD );
    MPI_Reduce( &(g_stats.ttl_controller_failures),
                &(agg_stats.ttl_controller_failures),
                1,
                MPI_INT,
                MPI_SUM,
                g_tw_masternode,
                MPI_COMM_WORLD );
    MPI_Reduce( &(g_stats.ttl_blocks_wr),
                &(agg_stats.ttl_blocks_wr),
                1,
                MPI_LONG_DOUBLE,
                MPI_SUM,
                g_tw_masternode,
                MPI_COMM_WORLD );
    MPI_Reduce( &(g_stats.ttl_idle_time),
                &(agg_stats.ttl_idle_time),
                1,
                MPI_DOUBLE,
                MPI_SUM,
                g_tw_masternode,
                MPI_COMM_WORLD );
    MPI_Reduce( &(g_stats.ttl_busy_time),
                &(agg_stats.ttl_busy_time),
                1,
                MPI_DOUBLE,
                MPI_SUM,
                g_tw_masternode,
                MPI_COMM_WORLD );

    if( tw_ismaster() )
    {
        // Print stats
        printf( "\n" );
        printf( "Total disk failures: %d\n", agg_stats.ttl_disk_failures );
        printf( "Total RAID rebuilds: %d\n", agg_stats.ttl_controller_rebuilds );
        printf( "Total RAID failures: %d\n", agg_stats.ttl_controller_failures );
        printf( "Total blocks read/written: %.0Lf\n", agg_stats.ttl_blocks_wr );
        printf( "Total time spent idling: %.5f\n", agg_stats.ttl_idle_time );
        printf( "Total time spent reading/writing: %.5f\n", agg_stats.ttl_busy_time );
        printf( "Total average throughput: %.5Lf\n", (agg_stats.ttl_blocks_wr / (long double)agg_stats.ttl_busy_time) );
    }

    tw_end();
    return 0;
}
