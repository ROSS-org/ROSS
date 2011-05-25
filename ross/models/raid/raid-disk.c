/**
 * RAID File Server Farm Simulation
 *
 * FILE: raid-disk.c
 * AUTHOR: Matt Perry
 * DATE: Apr 27 2011
 */
#include "raid.h"

// Helpers
void raid_disk_gen_send( tw_lpid dest, Event event_type, tw_stime event_time, tw_lp* lp )
{
    // Generate and send message to dest
    tw_event* event = tw_event_new( dest, event_time, lp );
    MsgData* message = (MsgData*)tw_event_data( event );
    message->event_type = event_type;
    tw_event_send( event );
}

tw_stime raid_disk_fail_time( tw_lp* lp )
{
    tw_stime fail_time;

    switch( g_disk_distro )
    {
        case 0:
            fail_time = (tw_stime)((double)tw_rand_integer( lp->rng, 0, 2 * MTTF ));
            break;

        case 1:
            fail_time = (tw_stime)tw_rand_exponential( lp->rng, MTTF );
            break;

        case 2:
            fail_time = (tw_stime)tw_rand_weibull( lp->rng, MTTF, 0.1 );
            break;

        case 3:
            fail_time = (tw_stime)tw_rand_weibull( lp->rng, MTTF, 0.5 );
            break;

        case 4:
            fail_time = (tw_stime)tw_rand_weibull( lp->rng, MTTF, 1.0 );
            break;

        case 5:
            fail_time = (tw_stime)tw_rand_weibull( lp->rng, MTTF, 5.0 );
            break;

        case 6:
            fail_time = (tw_stime)tw_rand_weibull( lp->rng, MTTF, 10.0 );
            break;

        default:
            printf( "Bad disk_distro setting (%d)\n", g_disk_distro );
            exit( -1 );
    }

    return fail_time;
}

tw_lpid raid_disk_find_controller( tw_lp* lp )
{
    tw_lpid rcid = ( ( lp->gid % LPS_PER_FS ) - ( 1 + CONT_PER_FS + FS_PER_IO ) ) / DISK_PER_RC;
    return ( LPS_PER_FS * ( lp->gid / LPS_PER_FS ) ) + FS_PER_IO + 1 + rcid;
}

// Initialize
void raid_disk_init( DiskState* s, tw_lp* lp )
{
    // Initialize state
    s->m_controller = raid_disk_find_controller( lp );
    s->num_failures = 0;

    // Initialize event handler
    MsgData init_msg;
    tw_bf init_bf;
    init_msg.event_type = DISK_REPLACED;
    raid_disk_eventhandler( s, &init_bf, &init_msg, lp );
}

// Event Handler
void raid_disk_eventhandler( DiskState* s, tw_bf* cv, MsgData* m, tw_lp* lp )
{
    // Declare time variable
    tw_stime time;
    // Check message type
    switch( m->event_type )
    {
        case DISK_FAILURE:
            time = tw_rand_normal_sd( lp->rng, REPLACE_TIME, 
                        STD_DEV * REPLACE_TIME, &(m->rc.rng_calls) );
            // Generate and send messages
            raid_disk_gen_send( s->m_controller, DISK_REPLACED, time, lp );
            raid_disk_gen_send( lp->gid, DISK_REPLACED, time, lp );
            break;
        case DISK_REPLACED:
            time = raid_disk_fail_time( lp );
            if( ( tw_now( lp ) + time ) < (tw_stime)MAX_HOURS )
            {
                // Generate and send messages
                raid_disk_gen_send( s->m_controller, DISK_FAILURE, time, lp );
                raid_disk_gen_send( lp->gid, DISK_FAILURE, time, lp );

                // Modify state
                cv->c0 = 1;
                s->num_failures++;
            }
            break;
        default:
            message_error( m->event_type );
    }
}

// Reverse Event Handler
void raid_disk_eventhandler_rc( DiskState* s, tw_bf* cv, MsgData* m, tw_lp* lp )
{
    int i;
    // Check message type
    switch( m->event_type )
    {
        case DISK_FAILURE:
            for( i = 0; i < m->rc.rng_calls; ++i )
                tw_rand_reverse_unif( lp->rng );
            break;
        case DISK_REPLACED:
            tw_rand_reverse_unif( lp->rng );
            if( cv->c0 )
                s->num_failures--;
            break;
        default:
            message_error( m->event_type );
    }
}

// Finish
void raid_disk_finish( DiskState* s, tw_lp* lp )
{
    g_stats.ttl_disk_failures += s->num_failures;
}
