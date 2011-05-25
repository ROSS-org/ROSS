/**
 * RAID File Server Farm Simulation
 *
 * FILE: raid-io.c
 * AUTHOR: Matt Perry
 * DATE: Apr 27 2011
 */
#include "raid.h"

// Helpers
void raid_io_gen_send( tw_lpid dest, Event event_type, tw_stime event_time, tw_lp* lp )
{
    // Generate and send message to dest
    tw_event* event = tw_event_new( dest, event_time, lp );
    MsgData* message = (MsgData*)tw_event_data( event );
    message->event_type = event_type;
    message->rc.io_time = event_time;
    tw_event_send( event );
}

tw_lpid raid_io_find_server( tw_lp* lp )
{
    return ( LPS_PER_FS * ( lp->gid / LPS_PER_FS ) ) + FS_PER_IO;
}

// Initialize
void raid_io_init( IOState* s, tw_lp* lp )
{
    // Initial State
    s->m_server = raid_io_find_server( lp );
    s->ttl_busy = 0;
    s->ttl_idle = 0;

    // Initialize event handler
    MsgData init_msg;
    init_msg.event_type = IO_IDLE;
    raid_io_eventhandler( s, NULL, &init_msg, lp );
}

// Event Handler
void raid_io_eventhandler( IOState* s, tw_bf* cv, MsgData* m, tw_lp* lp )
{
    Event next_event_type;
    tw_stime this_event_length;

    // Check the message type
    switch( m->event_type )
    {
        case IO_BUSY:
            s->mode = BUSY;
            // Calculate the time to spend in BUSY mode
            this_event_length = tw_rand_normal_sd( lp->rng, BUSY_TIME, 
                        STD_DEV * BUSY_TIME, &(m->rc.rng_calls) );
            s->ttl_busy += this_event_length; 
            next_event_type = IO_IDLE;
            break;
        case IO_IDLE:
            s->mode = IDLE;
            // Calculate the time to spend in IDLE mode
            this_event_length = tw_rand_normal_sd( lp->rng, IDLE_TIME, 
                        STD_DEV * IDLE_TIME, &(m->rc.rng_calls) );
            s->ttl_idle += this_event_length; 
            next_event_type = IO_BUSY;
            break;
        default:
            message_error( m->event_type );
    } 

    // Generate and send the BUSY message to self
    raid_io_gen_send( lp->gid, next_event_type, this_event_length, lp );
    // Generate and send the BUSY message to server
    raid_io_gen_send( s->m_server, next_event_type, this_event_length, lp );
}

// Reverse Event Handler
void raid_io_eventhandler_rc( IOState* s, tw_bf* cv, MsgData* m, tw_lp* lp )
{
    // Check the message type, reverse state
    switch( m->event_type )
    {
        case IO_BUSY:
            s->mode = IDLE;
            s->ttl_busy -= m->rc.io_time;
            break;
        case IO_IDLE:
            s->mode = BUSY;
            s->ttl_idle -= m->rc.io_time;
            break;
        default:
            message_error( m->event_type );
    }

    int i;
    for( i = 0; i < m->rc.rng_calls; ++i )
        tw_rand_reverse_unif( lp->rng );
}

// Finish
void raid_io_finish( IOState* s, tw_lp* lp )
{
    g_stats.ttl_idle_time += s->ttl_idle;
    g_stats.ttl_busy_time += s->ttl_busy;
}
