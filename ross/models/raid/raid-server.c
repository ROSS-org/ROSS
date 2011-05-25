/**
 * RAID File Server Farm Simulation
 *
 * FILE: raid-server.c
 * AUTHOR: Matt Perry
 * DATE: Apr 27 2011
 */
#include "raid.h"

// Helpers
void raid_server_calculate_bandwidth( ServerState* s )
{
   s->bandwidth = CONT_FS_BW * ( s->num_controllers_good_health +
      s->num_controllers_rebuilding * THROTTLE ); 
}

long raid_server_num_blocks( ServerState* s, tw_lp* lp )
{
    double blocks = ( tw_now( lp ) - s->mode_change_timestamp ) * 
        ( s->bandwidth / BLOCK_SIZE );
    return (long)blocks;
}

// Initialize
void raid_server_init( ServerState* s, tw_lp* lp )
{
    // Initialize State
    s->mode = IDLE;
    s->mode_change_timestamp = tw_now( lp );
    s->num_blocks_wr = 0;
    s->num_controllers_good_health = CONT_PER_FS;
    s->num_controllers_rebuilding = 0;
    s->num_controllers_failure = 0;
}

// Event Handler
void raid_server_eventhandler( ServerState* s, tw_bf* cv, MsgData* m, tw_lp* lp )
{
    // Check message type
    switch( m->event_type )
    {
        case IO_IDLE:
            s->mode = IDLE; 
            break;
        case IO_BUSY:
            s->mode = BUSY; 
            break;
        case REBUILD_START:
            s->num_controllers_rebuilding++;
            s->num_controllers_failure--;
            break;
        case REBUILD_FINISH:
            s->num_controllers_good_health++;
            s->num_controllers_rebuilding--;
            break;
        case RAID_FAILURE:
            s->num_controllers_failure++;
            s->num_controllers_rebuilding--;
            break;
        case DISK_FAILURE:
            s->num_controllers_failure++;
            s->num_controllers_good_health--;
            break;
        default:
            message_error( m->event_type );
    }

    // Recalculate bandwidth
    raid_server_calculate_bandwidth( s );
    // Add to the blocks read/written count
    if( m->event_type != IO_BUSY )
        s->num_blocks_wr += m->rc.server_blocks = raid_server_num_blocks( s, lp );
    // Save current timestamp
    m->rc.server_timestamp = s->mode_change_timestamp;
    // Set new timestamp
    s->mode_change_timestamp = tw_now( lp );
}

// Reverse Event Handler
void raid_server_eventhandler_rc( ServerState* s, tw_bf* cv, MsgData* m, tw_lp* lp )
{
    // Check message type
    switch( m->event_type )
    {
        case IO_IDLE:
            s->mode = BUSY;  
            break;
        case IO_BUSY:
            s->mode = IDLE;
            break;
        case REBUILD_START:
            s->num_controllers_rebuilding--;
            s->num_controllers_failure++;
            break;
        case REBUILD_FINISH:
            s->num_controllers_good_health--;
            s->num_controllers_rebuilding++;
            break;
        case RAID_FAILURE:
            s->num_controllers_failure--;
            s->num_controllers_rebuilding++;
            break;
        case DISK_FAILURE:
            s->num_controllers_failure--;
            s->num_controllers_good_health++;
            break;
        default:
            message_error( m->event_type );
    }

    // Recalculate bandwidth
    raid_server_calculate_bandwidth( s );
    // Reset the timestamp
    s->mode_change_timestamp = m->rc.server_timestamp;
    // Reset the blocks read/written count and reverse the rng
    if( m->event_type != IO_BUSY )
    {
        s->num_blocks_wr -= m->rc.server_blocks;
        tw_rand_reverse_unif( lp->rng );
    }
}

// Finish
void raid_server_finish( ServerState* s, tw_lp* lp )
{
    g_stats.ttl_blocks_wr += s->num_blocks_wr;
}
