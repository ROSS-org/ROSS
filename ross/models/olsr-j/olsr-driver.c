#include "ross.h"
#include "olsr.h"

/**
 * @file
 * @brief OLSR Driver
 *
 * Simple driver to test out various functionalities in the OLSR impl.
 */

#define GRID_MAX 100
#define STAGGER_MAX 10

static unsigned int nlp_per_pe = 8;

/**
 * Initializer for OLSR
 */
void olsr_init(node_state *s, tw_lp *lp)
{
    tw_event *e;
    olsr_msg_data *msg;
    tw_stime ts;
    
    s->num_tuples = 0;
    s->num_neigh  = 0;
    s->lng = tw_rand_unif(lp->rng) * GRID_MAX;
    s->lat = tw_rand_unif(lp->rng) * GRID_MAX;
    
    ts = tw_rand_unif(lp->rng) * STAGGER_MAX;
    
    e = tw_event_new(lp->gid, ts, lp);
    msg = tw_event_data(e);
    msg->type = HELLO;
    tw_event_send(e);
}

void olsr_event(node_state *s, tw_bf *bf, olsr_msg_data *m, tw_lp *lp)
{
    int i;
    tw_event *e;
    olsr_msg_data *msg;
    
    switch(m->type) {
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
    }
}

void olsr_final(node_state *s, tw_lp *lp)
{
    
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