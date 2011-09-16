#include "ross.h"
#include "olsr.h"
#include <assert.h>

/**
 * @file
 * @brief OLSR Driver
 *
 * Simple driver to test out various functionalities in the OLSR impl.
 */

#define GRID_MAX 100
#define STAGGER_MAX 10
#define HELLO_DELTA 0.0001

static unsigned int nlp_per_pe = OLSR_MAX_NEIGHBORS;

/**
 * Initializer for OLSR
 */
void olsr_init(node_state *s, tw_lp *lp)
{
    hello *h;
    tw_event *e;
    olsr_msg_data *msg;
    tw_stime ts;
    
    s->num_tuples = 0;
    s->num_neigh  = 0;
    s->local_address = lp->gid;
    s->lng = tw_rand_unif(lp->rng) * GRID_MAX;
    s->lat = tw_rand_unif(lp->rng) * GRID_MAX;
    
    ts = tw_rand_unif(lp->rng) * STAGGER_MAX;
    
    e = tw_event_new(lp->gid, ts, lp);
    msg = tw_event_data(e);
    msg->type = HELLO_TX;
    msg->originator = s->local_address;
    msg->lng = s->lng;
    msg->lat = s->lat;
    h = &msg->mt.h;
    h->num_neighbors = 0;
    //h->neighbor_addrs[0] = s->local_address;
    tw_event_send(e);
}

/**
 * Event handler.  Basically covers two events at the moment:
 * - HELLO_TX: HELLO transmit required now, so package up all of our
 * neighbors into a message and send it.  Also schedule our next TX
 * - HELLO_RX: HELLO received, at the moment this only supports 1-hop
 * so we just pull the neighbor's address (neighbor_addrs[0]) from
 * the message.  HELLO_RX is dictated by HELLO_TX so don't schedule another
 *
 * TODO: Re-arrange logic so TX generates a single RX, which then schedules
 * another RX, which schedules another RX, etc. until all or informed
 */
void olsr_event(node_state *s, tw_bf *bf, olsr_msg_data *m, tw_lp *lp)
{
    int in;
    int i, j;
    hello *h;
    tw_event *e;
    tw_stime ts;
    olsr_msg_data *msg;
    
    switch(m->type) {
        case HELLO_TX:
            ts = tw_rand_exponential(lp->rng, HELLO_DELTA);
            
            tw_lp *cur_lp = g_tw_lp[0];
            
            e = tw_event_new(cur_lp->gid, ts, lp);
            msg = tw_event_data(e);
            msg->type = HELLO_RX;
            msg->originator = m->originator;
            msg->lng = s->lng;
            msg->lat = s->lat;
            msg->target = 0;
            h = &msg->mt.h;
            h->num_neighbors = s->num_neigh;// + 1;
            //h->neighbor_addrs[0] = s->local_address;
            for (j = 0; j < s->num_neigh; j++) {
                h->neighbor_addrs[j] = s->neighSet[j].neighborMainAddr;
            }
            tw_event_send(e);
            
            /*
            for (i = 0; i < g_tw_nlp; i++) {
                tw_stime ts = tw_rand_exponential(lp->rng, HELLO_DELTA);
                
                tw_lp *cur_lp = g_tw_lp[i];
                
                e = tw_event_new(cur_lp->gid, ts, lp);
                msg = tw_event_data(e);
                msg->type = HELLO_RX;
                msg->lng = s->lng;
                msg->lat = s->lat;
                h = &msg->mt.h;
                h->num_neighbors = s->num_neigh + 1;
                h->neighbor_addrs[0] = s->local_address;
                for (j = 0; j < s->num_neigh; j++) {
                    h->neighbor_addrs[j+1] = s->neighSet[j].neighborMainAddr;
                }
                tw_event_send(e);
            }
            */
            
            e = tw_event_new(lp->gid, HELLO_INTERVAL, lp);
            msg = tw_event_data(e);
            msg->type = HELLO_TX;
            msg->originator = s->local_address;
            msg->lng = s->lng;
            msg->lat = s->lat;
            h = &msg->mt.h;
            h->num_neighbors = 0;//1;
            //h->neighbor_addrs[0] = s->local_address;
            tw_event_send(e);
            
            break;
            
        case HELLO_RX:
            // TODO: Check if we can hear this message here?
            // Add neighbor_addrs[0] to 1-hop neighbor list
            
            h = &m->mt.h;
            
            in = 0;
            
            // If we receive our own message, don't add ourselves but
            // DO generate a new event for the next guy!
            
            // Copy the message we just received; we can't add data to
            // a message sent by another node
            if (m->target < g_tw_nlp - 1) {
                ts = tw_rand_exponential(lp->rng, HELLO_DELTA);
                
                tw_lp *cur_lp = g_tw_lp[m->target + 1];
                
                e = tw_event_new(cur_lp->gid, ts, lp);
                msg = tw_event_data(e);
                msg->type = HELLO_RX;
                msg->originator = m->originator;
                msg->lng = m->lng;
                msg->lat = m->lat;
                msg->target = m->target + 1;
                h = &msg->mt.h;
                h->num_neighbors = m->mt.h.num_neighbors;//m->num_neigh + 1;
                //h->neighbor_addrs[0] = s->local_address;
                for (j = 0; j < h->num_neighbors; j++) {
                    h->neighbor_addrs[j] = m->mt.h.neighbor_addrs[j];
                    //h->neighbor_addrs[j] = s->neighSet[j].neighborMainAddr;
                }
                tw_event_send(e);
            }
            
            if (s->local_address == m->originator) {
                return;
            }
            
            for (i = 0; i < s->num_neigh; i++) {
                if (s->neighSet[i].neighborMainAddr == m->originator) {
                    in = 1;
                }
            }
            
            if (!in) {
                s->neighSet[s->num_neigh].neighborMainAddr = m->originator;
                s->num_neigh++;
                assert(s->num_neigh < OLSR_MAX_NEIGHBORS);
            }
            
            break;
            
            /*
        case HELLO:
            // TODO: Add new nodes
            // ...
            
            // Schedule next event
            e = tw_event_new(lp->gid, HELLO_INTERVAL, lp);
            msg = tw_event_data(e);
            msg->type = HELLO;
            msg->node_id = lp->gid;
            msg->lng = s->lng;
            msg->lat = s->lat;
            // Should this also happen for num_tuples or what?
            for (i = 0; i < s->num_neigh; i++) {
                msg->mt.h.neighbor_addrs[i] = s->neighSet[i].neighborMainAddr;
            }
            msg->mt.h.num_neighbors = s->num_neigh;
            tw_event_send(e);
             */
    }
}

void olsr_final(node_state *s, tw_lp *lp)
{
    int i;
    
    printf("node %p contains %d neighbors\n", s->local_address, s->num_neigh);
    for (i = 0; i < s->num_neigh; i++) {
        printf("   neighbor[%d] is %p\n", i, s->neighSet[i].neighborMainAddr);
    }
}

tw_peid olsr_map(tw_lpid gid)
{
    return (tw_peid)gid / g_tw_nlp;
}

tw_lptype olsr_lps[] = {
    {
        (init_f) olsr_init,
        (event_f) olsr_event,
        (revent_f) NULL,
        (final_f) olsr_final,
        (map_f) olsr_map,
        sizeof(node_state)
    },
    { 0 },
};

const tw_optdef olsr_opts[] = {
    TWOPT_GROUP("OLSR Model"),
    TWOPT_UINT("lp_per_pe", nlp_per_pe, "number of LPs per processor"),
    TWOPT_END(),
};

int main(int argc, char *argv[])
{
    int i;
    
    g_tw_events_per_pe = nlp_per_pe * nlp_per_pe * 1;
    
    tw_opt_add(olsr_opts);
    
    tw_init(&argc, &argv);
    
    tw_define_lps(nlp_per_pe, sizeof(olsr_msg_data), 0);
    
    for (i = 0; i < g_tw_nlp; i++) {
        tw_lp_settype(i, &olsr_lps[0]);
    }
    
    tw_run();
    
    if (tw_ismaster()) {
        printf("Complete.\n");
    }
    
    tw_end();
    
    return 0;
}