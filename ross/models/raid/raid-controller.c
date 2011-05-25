/**
 * RAID File Server Farm Simulation
 *
 * FILE: raid-controller.c
 * AUTHOR: Matt Perry
 * DATE: Apr 27 2011
 */
#include "raid.h"

// Helpers
void raid_controller_gen_send( tw_lpid dest, Event event_type, tw_stime event_time, tw_lp* lp )
{
    // Generate and send message to dest
    tw_event* event = tw_event_new( dest, event_time, lp );
    MsgData* message = (MsgData*)tw_event_data( event );
    message->event_type = event_type;
    tw_event_send( event );
}

tw_lpid raid_controller_find_server( tw_lp* lp )
{
    return ( LPS_PER_FS * ( lp->gid / LPS_PER_FS ) ) + FS_PER_IO;
}

// Initialize
void raid_controller_init( ContState* s, tw_lp* lp )
{
    // Initialize state
    s->m_server = raid_controller_find_server( lp );
    s->condition = HEALTHY;
    s->num_rebuilds = 0;
}

// Event Handler
void raid_controller_eventhandler( ContState* s, tw_bf* cv, MsgData* m, tw_lp* lp )
{
    // Save the controller's initial condition
    m->rc.controller_condition = s->condition;
    // Only handle events if the controller isnt dead
    if( s->condition != FAILED )
    {
        // Check message type
        switch( m->event_type )
        {
            case DISK_FAILURE:
                switch( s->condition )
                {
                    case WAIT:
                        s->condition = FAILED;
                        break;
                    case REBUILD:
                        s->condition = FAILED;
                        // Generate and send message to the file server
                        raid_controller_gen_send( s->m_server, RAID_FAILURE, 0, lp );
                        break;
                    default:
                        s->condition = WAIT;
                        // Generate and send message to the file server
                        raid_controller_gen_send( s->m_server, DISK_FAILURE, 0, lp );
                        break;
                }
                break;
            case DISK_REPLACED:
                s->condition = REBUILD;
                s->num_rebuilds++;

                // Schedule rebuild completion
                tw_stime rebuild_time = tw_rand_normal_sd( lp->rng, REBUILD_TIME, 
                        STD_DEV * REBUILD_TIME, &(m->rc.rng_calls) );
                raid_controller_gen_send( lp->gid, REBUILD_FINISH, rebuild_time, lp );

                // Generate and send message to the file server
                raid_controller_gen_send( s->m_server, REBUILD_START, 0, lp );
                break;
            case REBUILD_FINISH:
                s->condition = HEALTHY;

                // Generate and send message to the file server
                raid_controller_gen_send( s->m_server, REBUILD_FINISH, 0, lp );
                break;
            default:
                message_error( m->event_type );
        }
    }
}

// Reverse Event Handler
void raid_controller_eventhandler_rc( ContState* s, tw_bf* cv, MsgData* m, tw_lp* lp )
{
    int i;
    s->condition = m->rc.controller_condition;
    if( m->event_type == DISK_REPLACED || s->condition != FAILED )
    {
        s->num_rebuilds--;
        for( i = 0; i < m->rc.rng_calls; ++i )
            tw_rand_reverse_unif( lp->rng );
    }
}

// Finish
void raid_controller_finish( ContState* s, tw_lp* lp )
{
    g_stats.ttl_controller_rebuilds += s->num_rebuilds;
    if( s->condition == FAILED )
        g_stats.ttl_controller_failures++;
}
