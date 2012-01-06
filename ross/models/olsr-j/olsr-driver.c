#include "ross.h"
#include "olsr.h"
#include <assert.h>

/**
 * @file
 * @brief OLSR Driver
 *
 * Simple driver to test out various functionalities in the OLSR impl.
 */

double g_X[OLSR_MAX_NEIGHBORS];
double g_Y[OLSR_MAX_NEIGHBORS];

#define GRID_MAX 100
#define STAGGER_MAX 10
#define HELLO_DELTA 0.0001
#define OLSR_NO_FINAL_OUTPUT 1
#define USE_RADIO_DISTANCE 1
#define RWALK_INTERVAL 20

#define DEBUG 0

static unsigned int nlp_per_pe = OLSR_MAX_NEIGHBORS;

// Used as scratch space for MPR calculations
unsigned g_Dy[OLSR_MAX_NEIGHBORS];
unsigned g_num_one_hop;
neigh_tuple g_mpr_one_hop[OLSR_MAX_NEIGHBORS];
unsigned g_num_two_hop;
two_hop_neigh_tuple g_mpr_two_hop[OLSR_MAX_2_HOP];
unsigned g_reachability[OLSR_MAX_NEIGHBORS];
neigh_tuple g_mpr_neigh_to_add;
unsigned g_mpr_num_add_nodes;
char g_covered[BITNSLOTS(OLSR_MAX_NEIGHBORS)];
char g_olsr_mobility = 'N';

unsigned long long g_olsr_event_stats[OLSR_END_EVENT];

unsigned region(o_addr a)
{
    return a / OLSR_MAX_NEIGHBORS;
}

/**
 * Initializer for OLSR
 */
void olsr_init(node_state *s, tw_lp *lp)
{
    hello *h;
    TC *t;
    tw_event *e;
    olsr_msg_data *msg;
    tw_stime ts;
    int i;

    //s->num_tuples = 0;
    s->num_neigh  = 0;
    s->num_two_hop = 0;
    s->num_mpr = 0;
    s->num_mpr_sel = 0;
    s->num_top_set = 0;
    s->num_dupes = 0;
    for (i = 0; i < OLSR_MAX_NEIGHBORS; i++) {
        s->SA_per_node[i] = 0;
    }
    // Now we store the GID as opposed to an int from 0-OMN
    s->local_address = lp->gid;// % OLSR_MAX_NEIGHBORS;
    s->lng = tw_rand_unif(lp->rng) * GRID_MAX;
    s->lat = tw_rand_unif(lp->rng) * GRID_MAX;
    // printf("Initializing node %lu on CPU %llu\n", lp->gid, lp->pe->id);
    
    //g_X[s->local_address] = s->lng;
    //g_Y[s->local_address] = s->lat;
    
    // Build our initial HELLO_TX messages
    ts = tw_rand_unif(lp->rng) * STAGGER_MAX;
    e = tw_event_new(lp->gid, ts, lp);
    msg = tw_event_data(e);
    msg->type = HELLO_TX;
    msg->originator = s->local_address;
    msg->lng = s->lng;
    msg->lat = s->lat;
    h = &msg->mt.h;
    h->num_neighbors = 0;
    tw_event_send(e);
    
    // Build our initial TC_TX messages
    ts = tw_rand_unif(lp->rng) * STAGGER_MAX;
    e = tw_event_new(lp->gid, ts, lp);
    msg = tw_event_data(e);
    msg->type = TC_TX;
    msg->originator = s->local_address;
    msg->lng = s->lng;
    msg->lat = s->lat;
    t = &msg->mt.t;
    //t->num_mpr_sel = 0;
    t->num_neighbors = 0;
    tw_event_send(e);
    
    // Build our initial SA_TX messages
    ts = tw_rand_unif(lp->rng) * STAGGER_MAX + SA_INTERVAL;
    e = tw_event_new(lp->gid, ts, lp);
    msg = tw_event_data(e);
    msg->type = SA_TX;
    msg->originator = s->local_address;
    msg->destination = MASTER_NODE;
    msg->lng = s->lng;
    msg->lat = s->lat;
    tw_event_send(e);
    
    if (g_olsr_mobility != 'n' && g_olsr_mobility != 'N') {
        // Build our initial RWALK_CHANGE messages
        ts = tw_rand_unif(lp->rng) * RWALK_INTERVAL + 1.0;
        e = tw_event_new(lp->gid, ts, lp);
        msg = tw_event_data(e);
        msg->type = RWALK_CHANGE;
        msg->lng = tw_rand_unif(lp->rng) * GRID_MAX;
        msg->lat = tw_rand_unif(lp->rng) * GRID_MAX;
        tw_event_send(e);
    }
    
#if 0 /* Source of instability */
    // Build our initial SA_MASTER_TX messages
    if (s->local_address == MASTER_NODE) {
        ts = tw_rand_unif(lp->rng) * MASTER_SA_INTERVAL + MASTER_SA_INTERVAL;
        e = tw_event_new(lp->gid, ts, lp);
        msg = tw_event_data(e);
        msg->type = SA_MASTER_TX;
        msg->originator = s->local_address;
        // Always send these to node zero, who receives all SA_MASTER msgs
        msg->destination = 0;
        msg->lng = s->lng;
        msg->lat = s->lat;
        tw_event_send(e);
    }
#endif
}

/**
 Copied from ns3 - propogation-loss-model.cc
 */
double 
DoCalcRxPower (double txPowerDbm,
               node_state *s,
               olsr_msg_data *m)
{
    /*
     * Friis free space equation:
     * where Pt, Gr, Gr and P are in Watt units
     * L is in meter units.
     *
     *    P     Gt * Gr * (lambda^2)
     *   --- = ---------------------
     *    Pt     (4 * pi * d)^2 * L
     *
     * Gt: tx gain (unit-less)
     * Gr: rx gain (unit-less)
     * Pt: tx power (W)
     * d: distance (m)
     * L: system loss
     * lambda: wavelength (m)
     *
     * Here, we ignore tx and rx gain and the input and output values 
     * are in dB or dBm:
     *
     *                           lambda^2
     * rx = tx +  10 log10 (-------------------)
     *                       (4 * pi * d)^2 * L
     *
     * rx: rx power (dB)
     * tx: tx power (dB)
     * d: distance (m)
     * L: system loss (unit-less)
     * lambda: wavelength (m)
     */
    
    double sender_lng = m->lng;
    double sender_lat = m->lat;
    double receiver_lng = s->lng;
    double receiver_lat = s->lat;
    
    // We have to make sure that everyone is in the same region even though
    // they may have overlapping x/y coordinates, i.e. a region describes the
    // local plane of existence for the nodes
    assert(region(s->local_address) == region(m->originator));
    
    double distance = (sender_lng - receiver_lng) * (sender_lng - receiver_lng);
    distance += (sender_lat - receiver_lat) * (sender_lat - receiver_lat);
    
    distance = sqrt(distance);
        
    //double distance = a->GetDistanceFrom (b);
    double m_minDistance = 1.0; // A reasonable default
    if (distance <= m_minDistance)
    {
        return txPowerDbm;
    }
    //double m_lambda = 3.0e8 / 2437000000.0; // (2.437 GHz, chan 6)
    double m_lambda = 0.058; // Stolen from Ken's slides, ~ 5GHz
    double numerator = m_lambda * m_lambda;
    double denominator = 16 * M_PI * M_PI * distance * distance;// * m_systemLoss;
    double pr = 10 * log10 (numerator / denominator);
    //printf("distance=%fm, attenuation coefficient=%fdB\n", distance, pr);
    return txPowerDbm + pr;
}

#define RANGE 60.0

static inline int out_of_radio_range(node_state *s, olsr_msg_data *m)
{
#if USE_RADIO_DISTANCE
    const double range = RANGE;
    
    double sender_lng = m->lng;
    double sender_lat = m->lat;
    double receiver_lng = s->lng;
    double receiver_lat = s->lat;
    
    // We have to make sure that everyone is in the same region even though
    // they may have overlapping x/y coordinates, i.e. a region describes the
    // local plane of existence for the nodes
    assert(region(s->local_address) == region(m->originator));
    
    double dist = (sender_lng - receiver_lng) * (sender_lng - receiver_lng);
    dist += (sender_lat - receiver_lat) * (sender_lat - receiver_lat);
    
    dist = sqrt(dist);
    
    if (dist > range) {
        return 1;
    }
    
    return 0;
#else
    if (DoCalcRxPower(OLSR_MPR_POWER, s, m) < -96.0)
        return 1;
    
    return 0;
#endif
}

/**
 * Compute D(y) as described in the "MPR Computation" section.  Description:
 *
 * The degree of an one hop neighbor node y (where y is a member of N), is
 * defined as the number of symmetric neighbors of node y, EXCLUDING all the
 * members of N and EXCLUDING the node performing the computation.
 *
 * N is the subset of neighbors of the node, which are neighbors of the
 * interface I.
 */
static inline unsigned Dy(node_state *s, o_addr target)
{
    int i, j, in;
    o_addr temp[OLSR_MAX_NEIGHBORS];
    int temp_size = 0;
    
    for (i = 0; i < s->num_two_hop; i++) {
        
        in = 0;
        for (j = 0; j < s->num_neigh; j++) {
            if (s->twoHopSet[i].twoHopNeighborAddr == s->neighSet[j].neighborMainAddr) {
                // EXCLUDING all members of N...
                in = 1;
                continue;
            }
        }
        
        if (in) continue;
        
        if (s->twoHopSet[i].neighborMainAddr == target) {
            in = 0;
            // Add s->twoHopSet[i].twoHopNeighborAddr to this set
            for (j = 0; j < temp_size; j++) {
                if (temp[j] == s->twoHopSet[i].twoHopNeighborAddr) {
                    in = 1;
                }
            }
            
            if (!in) {
                temp[temp_size] = s->twoHopSet[i].twoHopNeighborAddr;
                temp_size++;
                assert(temp_size < OLSR_MAX_NEIGHBORS);
            }
        }
    }
    
    return temp_size;
}

/**
 * Remove "n" from N2 (stored in g_mpr_two_hop)
 */
static inline void remove_node_from_n2(o_addr n)
{
    int i;
    int index_to_remove;
    
    while (1) {
        index_to_remove = -1;
        for (i = 0; i < g_num_two_hop; i++) {
            if (g_mpr_two_hop[i].twoHopNeighborAddr == n) {
                index_to_remove = i;
                break;
            }
        }
        
        if (index_to_remove == -1) break;
        
        //printf("Removing %d\n", index_to_remove);
        g_mpr_two_hop[index_to_remove].expirationTime = g_mpr_two_hop[g_num_two_hop-1].expirationTime;
        g_mpr_two_hop[index_to_remove].neighborMainAddr = g_mpr_two_hop[g_num_two_hop-1].neighborMainAddr;
        g_mpr_two_hop[index_to_remove].twoHopNeighborAddr = g_mpr_two_hop[g_num_two_hop-1].twoHopNeighborAddr;
        g_num_two_hop--;
    }
}

/**
 * Ensure that all nodes in MPR set are unique (hence "set")
 */
void mpr_set_uniq(node_state *s)
{
    int i;
    
    // Presumably if we just added a MPR, we only need to check all the
    // others against the last one
    o_addr last = s->mprSet[s->num_mpr-1];
    
    for (i = 0; i < s->num_mpr - 1; i++) {
        if (s->mprSet[i] == last) {
            s->num_mpr--;
            return;
        }
    }
}

/**
 * Ensure that all nodes in MPR selector set are unique (hence "set")
 */
void mpr_sel_set_uniq(node_state *s)
{
    int i;
    
    // Presumably if we just added a MPR, we only need to check all the
    // others against the last one
    o_addr last = s->mprSelSet[s->num_mpr_sel-1].mainAddr;
    
    for (i = 0; i < s->num_mpr_sel - 1; i++) {
        if (s->mprSelSet[i].mainAddr == last) {
            s->num_mpr_sel--;
            return;
        }
    }
    
    s->ansn++;
}

/**
 * Direct ripoff of corresponding ns3 function
 */
top_tuple * FindNewerTopologyTuple(o_addr last, uint16_t ansn, node_state *s)
{
    int i;
    
    for (i = 0; i < s->num_top_set; i++) {
        if (s->topSet[i].lastAddr == last && s->topSet[i].sequenceNumber > ansn)
            return &s->topSet[i];
    }
    
    return NULL;
}

/**
 * Direct ripoff of corresponding ns3 function
 */
void EraseOlderTopologyTuples(o_addr last, uint16_t ansn, node_state *s)
{
    int i;
    int index_to_remove;
    
    while (1) {
        index_to_remove = -1;
        for (i = 0; i < s->num_top_set; i++) {
            if (s->topSet[i].lastAddr == last && s->topSet[i].sequenceNumber < ansn) {
                index_to_remove = i;
                break;
            }
        }
        
        if (index_to_remove == -1) break;
        
        s->topSet[index_to_remove].destAddr = s->topSet[s->num_top_set-1].destAddr;
        s->topSet[index_to_remove].expirationTime = s->topSet[s->num_top_set-1].expirationTime;
        s->topSet[index_to_remove].lastAddr = s->topSet[s->num_top_set-1].lastAddr;
        s->topSet[index_to_remove].sequenceNumber = s->topSet[s->num_top_set-1].sequenceNumber;
        s->num_top_set--;
    }
}

/**
 * Direct ripoff of corresponding ns3 function
 */
top_tuple * FindTopologyTuple(o_addr destAddr, o_addr lastAddr, node_state *s)
{
    int i;
    
    for (i = 0; i < s->num_top_set; i++) {
        if (s->topSet[i].destAddr == destAddr && s->topSet[i].lastAddr == lastAddr) {
            return &s->topSet[i];
        }
    }
    
    return NULL;
}

neigh_tuple * FindSymNeighborTuple(node_state *s, o_addr mainAddr)
{
    int i;
    
    for (i = 0; i < s->num_neigh; i++) {
        if (s->neighSet[i].neighborMainAddr == mainAddr) {
            return &s->neighSet[i];
        }
    }
    
    return NULL;
}

RT_entry * Lookup(node_state *s, o_addr dest)
{
    int i;
    
    for (i = 0; i < s->num_routes; i++) {
        if (s->route_table[i].destAddr == dest) {
            return &s->route_table[i];
        }
    }
    
    return NULL;
}

/**
 * Trying to ripoff the corresponding ns3 function :)
 * Fortunately we don't need steps 4 or 5 since we don't support
 * multiple interfaces or HNA.
 */
void RoutingTableComputation(node_state *s)
{
    int i, h;
    RT_entry *route;
    
    // 1. All the entries from the routing table are removed.
    s->num_routes = 0;
    
    // 2. The new routing entries are added starting with the
    // symmetric neighbors (h=1) as the destination nodes.
    for (i = 0; i < s->num_neigh; i++) {
        s->route_table[s->num_routes].destAddr = s->neighSet[i].neighborMainAddr;
        s->route_table[s->num_routes].nextAddr = s->neighSet[i].neighborMainAddr;
        s->route_table[s->num_routes].distance = 1;
        s->num_routes++;
        assert(s->num_routes < OLSR_MAX_ROUTES);
    }
    
    //  3. for each node in N2, i.e., a 2-hop neighbor which is not a
    //  neighbor node or the node itself, and such that there exist at
    //  least one entry in the 2-hop neighbor set where
    //  N_neighbor_main_addr correspond to a neighbor node with
    //  willingness different of WILL_NEVER,
    for (i = 0; i < s->num_two_hop; i++) {
        
        if (FindSymNeighborTuple(s, s->twoHopSet[i].twoHopNeighborAddr)) {
            continue;
        }
        
        if (s->twoHopSet[i].twoHopNeighborAddr == s->local_address) {
            continue;
        }
        
        // one selects one 2-hop tuple and creates one entry in the routing table with:
        //                R_dest_addr  =  the main address of the 2-hop neighbor;
        //                R_next_addr  = the R_next_addr of the entry in the
        //                               routing table with:
        //                                   R_dest_addr == N_neighbor_main_addr
        //                                                  of the 2-hop tuple;
        //                R_dist       = 2;
        //                R_iface_addr = the R_iface_addr of the entry in the
        //                               routing table with:
        //                                   R_dest_addr == N_neighbor_main_addr
        //                                                  of the 2-hop tuple;
        if ((route = Lookup(s, s->twoHopSet[i].neighborMainAddr))) {
            s->route_table[s->num_routes].destAddr = s->twoHopSet[i].twoHopNeighborAddr;
            s->route_table[s->num_routes].nextAddr = route->nextAddr;
            s->route_table[s->num_routes].distance = 2;
            s->num_routes++;
            assert(s->num_routes < OLSR_MAX_ROUTES);
        }
    }
    
    // 3.1. For each topology entry in the topology table, if its
    // T_dest_addr does not correspond to R_dest_addr of any
    // route entry in the routing table AND its T_last_addr
    // corresponds to R_dest_addr of a route entry whose R_dist
    // is equal to h, then a new route entry MUST be recorded in
    // the routing table (if it does not already exist)
    for (h = 2; ; h++) {
        int added = 0;
        
        for (i = 0; i < s->num_top_set; i++) {
            //printf("Looking at node %lu top_tuple[%d] dest: %lu, last: %lu, seq: %d\n", s->local_address, i, s->topSet[i].destAddr, s->topSet[i].lastAddr, s->topSet[i].sequenceNumber);
            RT_entry *destAddrEntry = Lookup(s, s->topSet[i].destAddr);
            RT_entry *lastAddrEntry = Lookup(s, s->topSet[i].lastAddr);
            if (!destAddrEntry && lastAddrEntry && lastAddrEntry->distance == h) {
                s->route_table[s->num_routes].destAddr = s->topSet[i].destAddr;
                s->route_table[s->num_routes].nextAddr = lastAddrEntry->nextAddr;
                s->route_table[s->num_routes].distance = h + 1;
                s->num_routes++;
                assert(s->num_routes < OLSR_MAX_ROUTES);
                added = 1;
            }
            else {
                if (lastAddrEntry && !destAddrEntry) {
                    //printf("Not right length!\n");
                }
            }
        }
        
        if (!added) {
            break;
        }
    }
}

dup_tuple * FindDuplicateTuple(o_addr addr, uint16_t seq_num, node_state *s)
{
    int i;
    
    for (i = 0; i < s->num_dupes; i++) {
        if (s->dupSet[i].address == addr && s->dupSet[i].sequenceNumber == seq_num) {
            return &s->dupSet[i];
        }
    }
    
    return NULL;
}

/**
 * Had to add this function to minimize our dupe array
 */
void AddDuplicate(o_addr originator,
                  uint16_t seq_num,
                  Time ts,
                  int retransmitted,
                  node_state *s,
                  tw_lp *lp)
{
    int i;
    int index_to_remove;
    Time exp = tw_now(lp);
    
    while (1) {
        index_to_remove = -1;
        for (i = 0; i < s->num_dupes; i++) {
            if (s->dupSet[i].expirationTime < exp) {
                index_to_remove = i;
                break;
            }
        }
        
        if (index_to_remove == -1) break;
        
        //printf("Expiring Dupe\n");
        
        s->dupSet[index_to_remove].address        = s->dupSet[s->num_dupes-1].address;
        s->dupSet[index_to_remove].expirationTime = s->dupSet[s->num_dupes-1].expirationTime;
        s->dupSet[index_to_remove].retransmitted  = s->dupSet[s->num_dupes-1].retransmitted;
        s->dupSet[index_to_remove].sequenceNumber = s->dupSet[s->num_dupes-1].sequenceNumber;
        s->num_dupes--;
    }
    
    if (s->num_dupes == OLSR_MAX_DUPES - 1) {
        // Find the oldest and replace it
        int oldest = 0;
        
        for (i = 0; i < s->num_dupes; i++) {
            if (s->dupSet[i].expirationTime < s->dupSet[oldest].expirationTime) {
                oldest = i;
            }
        }
        
        //printf("node %lu (lpid = %llu) evicting dup %d (%lu) at time %f\n", s->local_address, lp->gid,
         //      oldest, s->dupSet[oldest].address, tw_now(lp));
        
        s->dupSet[oldest].address = originator;
        s->dupSet[oldest].sequenceNumber = seq_num;
        s->dupSet[oldest].expirationTime = ts;
        s->dupSet[oldest].retransmitted = retransmitted;
    }
    else {
        s->dupSet[s->num_dupes].address = originator;
        s->dupSet[s->num_dupes].sequenceNumber = seq_num;
        s->dupSet[s->num_dupes].expirationTime = ts;
        s->dupSet[s->num_dupes].retransmitted = retransmitted;
        s->num_dupes++;
        assert(s->num_dupes < OLSR_MAX_DUPES);
    }
}

void printTC(olsr_msg_data *m, node_state *s)
{
#ifdef JML_DEBUG
    int i;
    
    printf("Node %lu has %d neighbors:\n", s->local_address, s->num_neigh);
    for (i = 0; i < s->num_neigh; i++) {
        printf("   neighbor %lu\n", s->neighSet[i].neighborMainAddr);
    }
    printf("Received TC message with %d neighbors of node %lu\n",
           m->mt.t.num_neighbors, m->originator);
    for (i = 0; i < m->mt.t.num_neighbors; i++) {
        printf("   TC-NEIGH %lu\n", m->mt.t.neighborAddresses[i]);
    }
    printf("\n");
    
#endif /* JML_DEBUG */
    
    //printf("Node %lu just heard a TC from %lu\n", s->local_address, m->originator);
}

///
/// \brief OLSR's default forwarding algorithm.
///
/// See RFC 3626 for details.
///
/// \param p the %OLSR packet which has been received.
/// \param msg the %OLSR message which must be forwarded.
/// \param dup_tuple NULL if the message has never been considered for forwarding,
/// or a duplicate tuple in other case.
/// \param local_iface the address of the interface where the message was received from.
///
void ForwardDefault(olsr_msg_data *olsrMessage,
                    dup_tuple *duplicated,
                    o_addr localIface,
                    o_addr senderAddress,
                    node_state *s,
                    tw_lp *lp)
{
    int i;
    int j;
    TC *t;
    tw_event *e;
    tw_stime ts;
    tw_lp *cur_lp;
    olsr_msg_data *msg;
    
    // If the sender interface address is not in the symmetric
    // 1-hop neighborhood the message must not be forwarded
    if (NULL == FindSymNeighborTuple(s, senderAddress)) {
        return;
    }
    
    // If the message has already been considered for forwarding,
    // it must not be retransmitted again
    if (duplicated != NULL && duplicated->retransmitted)
    {
        return;
    }
    
    // If the sender interface address is an interface address
    // of a MPR selector of this node and ttl is greater than 1,
    // the message must be retransmitted
    int retransmitted = 0;
    for (i = 0; i < s->num_mpr_sel; i++) {
        if (s->mprSelSet[i].mainAddr == senderAddress) {
            // Round-robin-RX
            // Might want to rename HELLO_DELTA...
            ts = g_tw_lookahead + tw_rand_unif(lp->rng) * HELLO_DELTA;
            // ts += 1;
            
            cur_lp = tw_getlocal_lp(region(s->local_address)*OLSR_MAX_NEIGHBORS);
            
            e = tw_event_new(cur_lp->gid, ts, lp);
            msg = tw_event_data(e);
            msg->type = TC_RX;
            msg->ttl = olsrMessage->ttl - 1;
            msg->originator = olsrMessage->originator;
            msg->sender = s->local_address;
            msg->lng = s->lng;
            msg->lat = s->lat;
            msg->target = region(s->local_address) * OLSR_MAX_NEIGHBORS;
            t = &msg->mt.t;
            t->ansn = olsrMessage->mt.t.ansn;
            //t->num_mpr_sel = olsrMessage->mt.t.num_mpr_sel;
            t->num_neighbors = olsrMessage->mt.t.num_neighbors;
            for (j = 0; j < t->num_neighbors; j++) {
                t->neighborAddresses[j] = olsrMessage->mt.t.neighborAddresses[j];
            }
            //if (t->num_mpr_sel > 0) {
            //printTC(t);
            tw_event_send(e);
            //}
            //tw_event_send(e);
            
            
            retransmitted = 1;
            // TODO: Check if ns3 stuff is required here...  Possibly
            // actually do the retransmission here.
        }
    }
    
    if (duplicated != NULL) {
        duplicated->expirationTime = tw_now(lp) + OLSR_DUP_HOLD_TIME;
        duplicated->retransmitted = retransmitted;
    }
    else {
      AddDuplicate(olsrMessage->originator,
		   olsrMessage->seq_num,
		   tw_now(lp) + OLSR_DUP_HOLD_TIME,
		   retransmitted,
		   s, lp);

      //        s->dupSet[s->num_dupes].address = olsrMessage->originator;
      //        s->dupSet[s->num_dupes].sequenceNumber = olsrMessage->seq_num;
      //        s->dupSet[s->num_dupes].expirationTime = tw_now(lp) + OLSR_DUP_HOLD_TIME;
      //        s->dupSet[s->num_dupes].retransmitted = retransmitted;
      //        s->num_dupes++;
      //        assert(s->num_dupes < OLSR_MAX_DUPES);
    }
}

void route_packet(node_state *s, tw_event *e)
{
    olsr_msg_data *m = tw_event_data(e);
    RT_entry * route = Lookup(s, m->destination);
    if (route == NULL) {
        printf("Node %lu doesn't have a route to %lu\n", s->local_address, m->destination);
        return;
    }
    
    m->ttl--;
    
    //printf("routing from %lu to %lu, next hop %lu\n", m->originator,
    //       m->destination, route->nextAddr);
    
    m->sender = route->nextAddr;
    tw_event_send(e);
}

void process_sa(node_state *s, olsr_msg_data *m)
{
    s->SA_per_node[m->originator % OLSR_MAX_NEIGHBORS]++;
}

/**
 * Event handler.  Basically covers two events at the moment:
 * - HELLO_TX: HELLO transmit required now, so package up all of our
 * neighbors into a message and send it.  Also schedule our next TX
 * - HELLO_RX: HELLO received so we just pull the neighbor's address from
 * the message.  HELLO_RX messages are generated by the last HELLO_RX
 * message until we have exhausted all neighbors.
 * - TC_TX: Similar to HELLO_TX but for Topology Control
 * - TC_RX: Similar to HELLO_RX but for Topology Control
 */
void olsr_event(node_state *s, tw_bf *bf, olsr_msg_data *m, tw_lp *lp)
{
    int in;
    int i, j, k;
    int is_mpr;
    TC *t;
    hello *h;
    tw_event *e;
    tw_stime ts;
    tw_lp *cur_lp;
    olsr_msg_data *msg;
    
#if DEBUG
    if( lp->gid == 1023 ) {
        printf("LP DUMP Node %llu on Rank %d at TS=%lf: S Local Address = %llu, M Type = %d,M Sender = %llu, M Originator = %llu \n", 
               lp->gid, g_tw_mynode, tw_now(lp), s->local_address, m->type, m->sender, m->originator );
    }
#endif /* DEBUG */

    g_olsr_event_stats[m->type]++;
    
    switch(m->type) {
        case HELLO_TX:
        {
            ts = g_tw_lookahead + tw_rand_unif(lp->rng) * HELLO_DELTA;
            
            cur_lp = tw_getlocal_lp(region(s->local_address)*OLSR_MAX_NEIGHBORS);
            
            e = tw_event_new(cur_lp->gid, ts, lp);
            msg = tw_event_data(e);
            msg->type = HELLO_RX;
            msg->originator = m->originator;
            msg->lng = s->lng;
            msg->lat = s->lat;
            msg->target = tw_getlocal_lp(region(s->local_address)*OLSR_MAX_NEIGHBORS)->gid;
            h = &msg->mt.h;
            h->num_neighbors = s->num_neigh;// + 1;
            //h->neighbor_addrs[0] = s->local_address;
            for (j = 0; j < s->num_neigh; j++) {
                h->neighbor_addrs[j] = s->neighSet[j].neighborMainAddr;
                // If s->neighSet[j].neighborMainAddr is our MPR, we need
                // to set this appropriately
                is_mpr = 0;
                for (k = 0; k < s->num_mpr; k++) {
                    if (s->mprSet[k] == s->neighSet[j].neighborMainAddr) {
                        is_mpr = 1;
                    }
                }
                if (is_mpr) {
                    h->is_mpr[j] = 1;
                }
                else {
                    h->is_mpr[j] = 0;
                }
            }
            tw_event_send(e);
            
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
        }
        case HELLO_RX:
        {
            h = &m->mt.h;
            
            in = 0;
            
            // If we receive our own message, don't add ourselves but
            // DO generate a new event for the next guy!
            
            // Copy the message we just received; we can't add data to
            // a message sent by another node
            if (m->target < region(s->local_address)*OLSR_MAX_NEIGHBORS+OLSR_MAX_NEIGHBORS-1) {
                ts = g_tw_lookahead + tw_rand_unif(lp->rng) * HELLO_DELTA;
                
                tw_lp *cur_lp = tw_getlocal_lp(m->target + 1);
                
                e = tw_event_new(cur_lp->gid, ts, lp);
                msg = tw_event_data(e);
                msg->type = HELLO_RX;
                msg->originator = m->originator;
                msg->sender = m->sender;
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
            
            // We've already passed along the message which has to happen
            // regardless of whether or not it can be heard, handled, etc.
            
            // Check to see if we can hear this message or not
            if (out_of_radio_range(s, m)) {
                //printf("Out of range!\n");
                return;
            }
            
            if (s->local_address == m->originator) {
                return;
            }
            
            // BEGIN 1-HOP PROCESSING
            for (i = 0; i < s->num_neigh; i++) {
                if (s->neighSet[i].neighborMainAddr == m->originator) {
                    in = 1;
                }
            }
            
            if (!in) {
                s->neighSet[s->num_neigh].neighborMainAddr = m->originator;
                s->num_neigh++;
                assert(s->num_neigh < OLSR_MAX_NEIGHBORS);
                assert(region(s->local_address) == region(m->originator));
                s->ansn++;
            }
            // END 1-HOP PROCESSING
            
            // BEGIN 2-HOP PROCESSING
            
            h = &m->mt.h;
            
            for (i = 0; i < h->num_neighbors; i++) {
                if (s->local_address == h->neighbor_addrs[i]) {
                    // We are not going to be our own 2-hop neighbor!
                    continue;
                }
                
                // Check and see if h->neighbor_addrs[i] is in our list
                // already
                in = 0;
                for (j = 0; j < s->num_two_hop; j++) {
                    if (s->twoHopSet[j].neighborMainAddr == m->originator &&
                        s->twoHopSet[j].twoHopNeighborAddr == h->neighbor_addrs[i]) {
                        in = 1;
                    }
                }
                
                if (!in) {
                    s->twoHopSet[s->num_two_hop].neighborMainAddr = m->originator;
                    s->twoHopSet[s->num_two_hop].twoHopNeighborAddr = h->neighbor_addrs[i];
                    assert(s->twoHopSet[s->num_two_hop].neighborMainAddr !=
                           s->twoHopSet[s->num_two_hop].twoHopNeighborAddr);
                    s->num_two_hop++;
                    assert(s->num_two_hop < OLSR_MAX_2_HOP);
                }
            }
            
            // END 2-HOP PROCESSING
            
            // BEGIN MPR COMPUTATION
            
            // Initially no nodes are covered
            memset(g_covered, 0, BITNSLOTS(OLSR_MAX_NEIGHBORS));
            s->num_mpr = 0;
            
            // Copy all relevant information to scratch space
            g_num_one_hop = s->num_neigh;
            for (i = 0; i < g_num_one_hop; i++) {
                g_mpr_one_hop[i] = s->neighSet[i];
            }
            
            g_num_two_hop = s->num_two_hop;
            for (i = 0; i < g_num_two_hop; i++) {
                g_mpr_two_hop[i] = s->twoHopSet[i];
            }
            
            // Calculate D(y), where y is a member of N, for all nodes in N
            for (i = 0; i < g_num_one_hop; i++) {
                g_Dy[i] = Dy(s, g_mpr_one_hop[i].neighborMainAddr);
                g_reachability[i] = 0;
            }
            
            // Take care of the "unused" bits
//            for (i = g_num_two_hop; i < OLSR_MAX_2_HOP; i++) {
//                BITSET(g_covered, i);
//            }
            
//            // 2. Calculate D(y), where y is a member of N, for all nodes in N.
//            for (i = 0; i < g_num_one_hop; i++) {
//                g_Dy[i] = Dy(s, s->neighSet[i].neighborMainAddr);
//            }
            
            // 3. Add to the MPR set those nodes in N, which are the *only*
            // nodes to provide reachability to a node in N2.
//            std::set<Ipv4Address> coveredTwoHopNeighbors;
//            for (TwoHopNeighborSet::const_iterator twoHopNeigh = N2.begin (); twoHopNeigh != N2.end (); twoHopNeigh++)
//            {
//                bool onlyOne = true;
//                // try to find another neighbor that can reach twoHopNeigh->twoHopNeighborAddr
//                for (TwoHopNeighborSet::const_iterator otherTwoHopNeigh = N2.begin (); otherTwoHopNeigh != N2.end (); otherTwoHopNeigh++)
//                {
//                    if (otherTwoHopNeigh->twoHopNeighborAddr == twoHopNeigh->twoHopNeighborAddr
//                        && otherTwoHopNeigh->neighborMainAddr != twoHopNeigh->neighborMainAddr)
//                    {
//                        onlyOne = false;
//                        break;
//                    }
//                }
//                if (onlyOne)
//                {
//                    NS_LOG_LOGIC ("Neighbor " << twoHopNeigh->neighborMainAddr
//                                  << " is the only that can reach 2-hop neigh. "
//                                  << twoHopNeigh->twoHopNeighborAddr
//                                  << " => select as MPR.");
//                    
//                    mprSet.insert (twoHopNeigh->neighborMainAddr);
//                    
//                    // take note of all the 2-hop neighbors reachable by the newly elected MPR
//                    for (TwoHopNeighborSet::const_iterator otherTwoHopNeigh = N2.begin ();
//                         otherTwoHopNeigh != N2.end (); otherTwoHopNeigh++)
//                    {
//                        if (otherTwoHopNeigh->neighborMainAddr == twoHopNeigh->neighborMainAddr)
//                        {
//                            coveredTwoHopNeighbors.insert (otherTwoHopNeigh->twoHopNeighborAddr);
//                        }
//                    }
//                }
//            }
            
            for (i = 0; i < g_num_two_hop; i++) {
                int onlyOne = 1;
                // try to find another neighbor that can reach twoHopNeigh->twoHopNeighborAddr
                for (j = 0; j < g_num_two_hop; j++) {
                    if (g_mpr_two_hop[j].twoHopNeighborAddr == g_mpr_two_hop[i].twoHopNeighborAddr
                        && g_mpr_two_hop[j].neighborMainAddr != g_mpr_two_hop[i].neighborMainAddr) {
                        onlyOne = 0;
                        break;
                    }
                }
                
                if (onlyOne) {
                    s->mprSet[s->num_mpr] = g_mpr_two_hop[i].neighborMainAddr;
                    s->num_mpr++;
                    assert(s->num_mpr < OLSR_MAX_NEIGHBORS);
                    // Make sure they're all unique!
                    mpr_set_uniq(s);
                    
                    // take note of all the 2-hop neighbors reachable by the newly elected MPR
                    for (j = 0; j < g_num_two_hop; j++) {
                        if (g_mpr_two_hop[j].neighborMainAddr == g_mpr_two_hop[i].neighborMainAddr) {
                            //coveredTwoHopNeighbors.insert (otherTwoHopNeigh->twoHopNeighborAddr);
                            // We don't do that, we use bitfields.  Make sure
                            // our assumptions are correct then create a mask
                            //printf("%lu\n", g_mpr_two_hop[j].neighborMainAddr);
                            assert(region(g_mpr_two_hop[j].neighborMainAddr) == region(s->local_address));
                            BITSET(g_covered, g_mpr_two_hop[j].twoHopNeighborAddr % OLSR_MAX_NEIGHBORS);
                        }
                    }
                }
            }
            
//            // Remove the nodes from N2 which are now covered by a node in the MPR set.
//            for (TwoHopNeighborSet::iterator twoHopNeigh = N2.begin ();
//                 twoHopNeigh != N2.end (); )
//            {
//                if (coveredTwoHopNeighbors.find (twoHopNeigh->twoHopNeighborAddr) != coveredTwoHopNeighbors.end ())
//                {
//                    // This works correctly only because it is known that twoHopNeigh is reachable by exactly one neighbor, 
//                    // so only one record in N2 exists for each of them. This record is erased here.
//                    NS_LOG_LOGIC ("2-hop neigh. " << twoHopNeigh->twoHopNeighborAddr << " is already covered by an MPR.");
//                    twoHopNeigh = N2.erase (twoHopNeigh);
//                }
//                else
//                {
//                    twoHopNeigh++;
//                }
//            }
            // Remove the nodes from N2 which are now covered by a node in the MPR set.
            for (i = 0; i < g_num_two_hop; i++) {
                if (BITTEST(g_covered, g_mpr_two_hop[i].twoHopNeighborAddr % OLSR_MAX_NEIGHBORS)) {
                    //printf("1. g_num_two_hop is %d\n", g_num_two_hop);
                    remove_node_from_n2(g_mpr_two_hop[i].twoHopNeighborAddr);
                    //printf("2. g_num_two_hop is %d\n", g_num_two_hop);
                }
            }
            
            //return;
            
//            // 4. While there exist nodes in N2 which are not covered by at
//            // least one node in the MPR set:
//            while (N2.begin () != N2.end ())
            //printf("\n\n");
            while (g_num_two_hop) {
                //printf(".");
//                // 4.1. For each node in N, calculate the reachability, i.e., the
//                // number of nodes in N2 which are not yet covered by at
//                // least one node in the MPR set, and which are reachable
//                // through this 1-hop neighbor
//                std::map<int, std::vector<const NeighborTuple *> > reachability;
//                std::set<int> rs;
//                for (NeighborSet::iterator it = N.begin (); it != N.end (); it++)
//                {
//                    NeighborTuple const &nb_tuple = *it;
//                    int r = 0;
//                    for (TwoHopNeighborSet::iterator it2 = N2.begin (); it2 != N2.end (); it2++)
//                    {
//                        TwoHopNeighborTuple const &nb2hop_tuple = *it2;
//                        if (nb_tuple.neighborMainAddr == nb2hop_tuple.neighborMainAddr)
//                            r++;
//                    }
//                    rs.insert (r);
//                    reachability[r].push_back (&nb_tuple);
//                }
                
                for (i = 0; i < g_num_one_hop; i++) {
                    int r = 0;
                    
                    for (j = 0; j < g_num_two_hop; j++) {
                        if (g_mpr_one_hop[i].neighborMainAddr == g_mpr_two_hop[j].neighborMainAddr)
                            r++;
                    }
                    // Make sure our neighbors are from our region
                    assert(region(g_mpr_one_hop[i].neighborMainAddr) == region(s->local_address));
                    g_reachability[i] = r;
                }
                
//                // 4.2. Select as a MPR the node with highest N_willingness among
//                // the nodes in N with non-zero reachability. In case of
//                // multiple choice select the node which provides
//                // reachability to the maximum number of nodes in N2. In
//                // case of multiple nodes providing the same amount of
//                // reachability, select the node as MPR whose D(y) is
//                // greater. Remove the nodes from N2 which are now covered
//                // by a node in the MPR set.
//                NeighborTuple const *max = NULL;
//                int max_r = 0;
//                for (std::set<int>::iterator it = rs.begin (); it != rs.end (); it++)
//                {
//                    int r = *it;
//                    if (r == 0)
//                    {
//                        continue;
//                    }
//                    for (std::vector<const NeighborTuple *>::iterator it2 = reachability[r].begin ();
//                         it2 != reachability[r].end (); it2++)
//                    {
//                        const NeighborTuple *nb_tuple = *it2;
//                        if (max == NULL || nb_tuple->willingness > max->willingness)
//                        {
//                            max = nb_tuple;
//                            max_r = r;
//                        }
//                        else if (nb_tuple->willingness == max->willingness)
//                        {
//                            if (r > max_r)
//                            {
//                                max = nb_tuple;
//                                max_r = r;
//                            }
//                            else if (r == max_r)
//                            {
//                                if (Degree (*nb_tuple) > Degree (*max))
//                                {
//                                    max = nb_tuple;
//                                    max_r = r;
//                                }
//                            }
//                        }
//                    }
//                }
                
                int max = 0;
                int max_Dy = 0;
                
                for (i = 0; i < g_num_one_hop; i++) {
                    if (g_reachability[i] == 0) continue;
                    
                    if (g_reachability[i] > max) {
                        max = g_reachability[i];
                        g_mpr_neigh_to_add = g_mpr_one_hop[i];
                        max_Dy = g_Dy[i];
                    }
                    else if (g_reachability[i] == max) {
                        if (g_Dy[i] > max_Dy) {
                            max = g_reachability[i];
                            g_mpr_neigh_to_add = g_mpr_one_hop[i];
                            max_Dy = g_Dy[i];
                        }
                    }
                }
                
                if (max > 0) {
                    s->mprSet[s->num_mpr] = g_mpr_neigh_to_add.neighborMainAddr;
                    s->num_mpr++;
                    assert(s->num_mpr < OLSR_MAX_NEIGHBORS);
                    // Make sure they're all unique!
                    mpr_set_uniq(s);
                    
                    // take note of all the 2-hop neighbors reachable by the newly elected MPR
                    for (j = 0; j < g_num_two_hop; j++) {
                        if (g_mpr_two_hop[j].neighborMainAddr == g_mpr_neigh_to_add.neighborMainAddr) {
                            //coveredTwoHopNeighbors.insert (otherTwoHopNeigh->twoHopNeighborAddr);
                            // We don't do that, we use bitfields.  Make sure
                            // our assumptions are correct then create a mask
                            assert(region(g_mpr_two_hop[j].neighborMainAddr) == region(s->local_address));
                            BITSET(g_covered, g_mpr_two_hop[j].twoHopNeighborAddr % OLSR_MAX_NEIGHBORS);
                        }
                    }
                }
                
                // Remove the nodes from N2 which are now covered by a node in the MPR set.
                for (i = 0; i < g_num_two_hop; i++) {
                    if (BITTEST(g_covered, g_mpr_two_hop[i].twoHopNeighborAddr % OLSR_MAX_NEIGHBORS)) {
                        //printf("1. g_num_two_hop is %d\n", g_num_two_hop);
                        remove_node_from_n2(g_mpr_two_hop[i].twoHopNeighborAddr);
                        //printf("2. g_num_two_hop is %d\n", g_num_two_hop);
                    }
                }
                
            }
            
            // END MPR COMPUTATION
            
            // BEGIN MPR SELECTOR SET
            
            h = &m->mt.h;
            
            for (i = 0; i < h->num_neighbors; i++) {
                if (h->is_mpr[i]) {
                    // Check if it contains OUR address
                    if (h->neighbor_addrs[i] == s->local_address) {
                        // We should add this guy to the selector set
                        s->mprSelSet[s->num_mpr_sel].mainAddr = m->originator;
                        s->num_mpr_sel++;
                        assert(s->num_mpr_sel <= OLSR_MAX_NEIGHBORS);
                        mpr_sel_set_uniq(s);
                    }
                }
            }
            
            // END MPR SELECTOR SET
            
            break;
        }
        case TC_TX:
        {
            // Might want to rename HELLO_DELTA...
            ts = g_tw_lookahead + tw_rand_unif(lp->rng) * HELLO_DELTA;
            
            cur_lp = tw_getlocal_lp(region(s->local_address)*OLSR_MAX_NEIGHBORS);
            
            e = tw_event_new(cur_lp->gid, ts, lp);
            msg = tw_event_data(e);
            msg->type = TC_RX;
            msg->ttl = 255;
            msg->originator = m->originator;
            msg->sender = s->local_address;
            msg->lng = s->lng;
            msg->lat = s->lat;
            msg->target = tw_getlocal_lp(region(s->local_address)*OLSR_MAX_NEIGHBORS)->gid;
            t = &msg->mt.t;
            t->ansn = s->ansn;
            //t->num_mpr_sel = s->num_mpr_sel;
            t->num_neighbors = s->num_neigh;
            for (j = 0; j < s->num_neigh; j++) {
                t->neighborAddresses[j] = s->neighSet[j].neighborMainAddr;
            }
            //if (s->num_mpr_sel > 0) {
            //printTC(t);
            tw_event_send(e);
            //}
            //tw_event_send(e);
            
            e = tw_event_new(lp->gid, TC_INTERVAL, lp);
            msg = tw_event_data(e);
            msg->type = TC_TX;
            msg->originator = s->local_address;
            msg->lng = s->lng;
            msg->lat = s->lat;
            t = &msg->mt.t;
            //t->num_mpr_sel = 0;
            t->num_neighbors = 0;
            //printTC(t);
            tw_event_send(e);
            
            break;
        }
        case TC_RX:
        {
            // 1. When a packet arrives at the router, the router evaluates the TTL.
            // 2a. If the TTL=0, the router drops the packet, and sends an ICMP TTL Exceeded message to the source.
            // 2b. If the TTL>0, then the router decrements the TTL by 1, and then forwards the packet.
            // From http://www.dslreports.com/forum/r25655787-When-is-TTL-Decremented-by-a-Router-
            
            if (m->ttl == 0) {
                printf("TC_RX\n");
                printf("TTL Expired\n");
                return;
            }
            
            m->ttl--;
            
            // Copy the message we just received; we can't add data to
            // a message sent by another node
            
            if (m->target < region(s->local_address)*OLSR_MAX_NEIGHBORS+OLSR_MAX_NEIGHBORS-1) {
                ts = g_tw_lookahead + tw_rand_unif(lp->rng) * HELLO_DELTA;
                
                tw_lp *cur_lp = tw_getlocal_lp(m->target + 1);
                
                e = tw_event_new(cur_lp->gid, ts, lp);
                msg = tw_event_data(e);
                msg->type = TC_RX;
                msg->ttl = m->ttl;
                msg->originator = m->originator;
                msg->sender = m->sender;
                msg->lng = m->lng;
                msg->lat = m->lat;
                msg->target = m->target + 1;
                t = &msg->mt.t;
                t->ansn = m->mt.t.ansn;
                //t->num_mpr_sel = m->mt.t.num_mpr_sel;
                t->num_neighbors = m->mt.t.num_neighbors;
                for (j = 0; j < t->num_neighbors; j++) {
                    t->neighborAddresses[j] = m->mt.t.neighborAddresses[j];
                }
                //printTC(t);
                tw_event_send(e);
            }
            
            // We've already passed along the message which has to happen
            // regardless of whether or not it can be heard, handled, etc.
            
            // Check to see if we can hear this message or not
            if (out_of_radio_range(s, m)) {
                //printf("Out of range!\n");
                return;
            }
            
            if (s->local_address == m->originator) {
                return;
            }
            
            // BEGIN TC PROCESSING

            //int do_forwarding = 1;
            dup_tuple *duplicated = FindDuplicateTuple(m->originator, m->seq_num, s);
            
            if (duplicated != NULL) {
                //break;
            }
            
            ForwardDefault(m, duplicated, s->local_address, m->sender, s, lp);
            
            in = 0;
            
            // 1. If the sender interface of this message is not in the symmetric
            // 1-hop neighborhood of this node, the message MUST be discarded.
            for (i = 0; i < s->num_neigh; i++) {
                if (m->sender == s->neighSet[i].neighborMainAddr)
                    in = 1;
            }
            
            if (!in)
                return;
            
            // 2. If there exist some tuple in the topology set where:
            //    T_last_addr == originator address AND
            //    T_seq       >  ANSN,
            // then further processing of this TC message MUST NOT be
            // performed.
            top_tuple *tt = FindNewerTopologyTuple(m->originator, m->mt.t.ansn, s);
            if (tt != NULL)
                return;
            
            // 3. All tuples in the topology set where:
            //	T_last_addr == originator address AND
            //	T_seq       <  ANSN
            // MUST be removed from the topology set.
            EraseOlderTopologyTuples(m->originator, m->mt.t.ansn, s);
            
            printTC(m, s);
            
            // 4. For each of the advertised neighbor main address received in
            // the TC message:
            for (i = 0; i < m->mt.t.num_neighbors; i++) {
                o_addr addr = m->mt.t.neighborAddresses[i];
                // 4.1. If there exist some tuple in the topology set where:
                //        T_dest_addr == advertised neighbor main address, AND
                //        T_last_addr == originator address,
                // then the holding time of that tuple MUST be set to:
                //        T_time      =  current time + validity time.
                tt = FindTopologyTuple(addr, m->originator, s);
                
                if (tt != NULL) {
#warning "Correct this line - TOP_HOLD_TIME should be in the struct!"
                    tt->expirationTime = tw_now(lp) + TOP_HOLD_TIME;
                }
                else {
                    // 4.2. Otherwise, a new tuple MUST be recorded in the topology
                    // set where:
                    //	T_dest_addr = advertised neighbor main address,
                    //	T_last_addr = originator address,
                    //	T_seq       = ANSN,
                    //	T_time      = current time + validity time.
                    s->topSet[s->num_top_set].destAddr = addr;
                    s->topSet[s->num_top_set].lastAddr = m->originator;
                    s->topSet[s->num_top_set].sequenceNumber = m->mt.t.ansn;
#warning "Correct this line - TOP_HOLD_TIME should be in the struct!"
                    s->topSet[s->num_top_set].expirationTime = tw_now(lp) + TOP_HOLD_TIME;
                    s->num_top_set++;
                    assert(s->num_top_set < OLSR_MAX_TOP_TUPLES);
                }
            }
            
            // END TC PROCESSING
            
            
            break;
        }
        case SA_TX:
        {
            /*
             Situational awareness - every 10 seconds send a UDP packet
             containing a node's location to a specified node with an
             uplink.  Every 60 seconds, that uplink sends a message containing
             all nodes' locations.
             */    
            // Schedule ourselves again...
            ts = SA_INTERVAL;
            e = tw_event_new(lp->gid, ts, lp);
            msg = tw_event_data(e);
            msg->type = SA_TX;
            msg->originator = s->local_address;
            msg->destination = MASTER_NODE;
            msg->lng = s->lng;
            msg->lat = s->lat;
            tw_event_send(e);
            
            
            // Check and see if we are the destination...
            if (m->destination == s->local_address) {
                // This is the final stop
                process_sa(s, m);
                return;
            }
            
            // Might want to rename HELLO_DELTA...
            ts = g_tw_lookahead + tw_rand_unif(lp->rng) * HELLO_DELTA;
            
            cur_lp = tw_getlocal_lp(region(s->local_address)*OLSR_MAX_NEIGHBORS);
            
            // If we don't have a route, don't allocate an event!
            if (Lookup(s, MASTER_NODE) == NULL) {
                return;
            }
            
            e = tw_event_new(cur_lp->gid, ts, lp);
            msg = tw_event_data(e);
            msg->type = SA_RX;
            msg->ttl = 255;
            msg->originator = s->local_address;
            msg->sender = s->local_address;
            msg->destination = MASTER_NODE;
            msg->lng = s->lng;
            msg->lat = s->lat;
            msg->target = region(s->local_address) * OLSR_MAX_NEIGHBORS;
            
            route_packet(s, e);
            
            // We don't need to compute our routing table here so just return!
            return;
        }
        case SA_RX:
        {
            // 1. When a packet arrives at the router, the router evaluates the TTL.
            // 2a. If the TTL=0, the router drops the packet, and sends an ICMP TTL Exceeded message to the source.
            // 2b. If the TTL>0, then the router decrements the TTL by 1, and then forwards the packet.
            // From http://www.dslreports.com/forum/r25655787-When-is-TTL-Decremented-by-a-Router-
            
            if (m->ttl == 0) {
                printf("SA_RX\n");
                printf("TTL Expired\n");
                return;
            }
            
            // Check and see if we are the destination...
            if (m->destination == s->local_address) {
                // This is the final stop
                process_sa(s, m);
                return;
            }
            
            // Copy the message we just received; we can't add data to
            // a message sent by another node
            
            if (m->target < region(s->local_address)*OLSR_MAX_NEIGHBORS+OLSR_MAX_NEIGHBORS-1) {
                ts = g_tw_lookahead + tw_rand_unif(lp->rng) * HELLO_DELTA;
                
                tw_lp *cur_lp = tw_getlocal_lp(m->target + 1);
                
                e = tw_event_new(cur_lp->gid, ts, lp);
                msg = tw_event_data(e);
                msg->type = SA_RX;
                msg->ttl = m->ttl;
                msg->originator = m->originator;
                msg->sender = m->sender;
                msg->destination = m->destination;
                msg->lng = m->lng;
                msg->lat = m->lat;
                msg->target = m->target + 1;
                t = &msg->mt.t;
                t->ansn = m->mt.t.ansn;
                //t->num_mpr_sel = m->mt.t.num_mpr_sel;
                t->num_neighbors = m->mt.t.num_neighbors;
                for (j = 0; j < t->num_neighbors; j++) {
                    t->neighborAddresses[j] = m->mt.t.neighborAddresses[j];
                }
                //printTC(t);
                tw_event_send(e);
            }
            
            // We've already passed along the message which has to happen
            // regardless of whether or not it can be heard, handled, etc.
            
            // Check to see if we can hear this message or not
            if (out_of_radio_range(s, m)) {
                //printf("Out of range!\n");
                return;
            }
            
            if (s->local_address == m->originator) {
                return;
            }
            
            if (m->sender == s->local_address) {
                // If we don't have a route, don't allocate an event!
                if (Lookup(s, MASTER_NODE) == NULL) {
                    return;
                }
                
                ts = g_tw_lookahead + tw_rand_unif(lp->rng) * HELLO_DELTA;
                e = tw_event_new(lp->gid, ts, lp);
                msg = tw_event_data(e);
                msg->type = SA_RX;
                msg->ttl = m->ttl;
                msg->originator = m->originator;
                msg->sender = s->local_address;
                msg->destination = MASTER_NODE;
                msg->lng = s->lng;
                msg->lat = s->lat;
                
                route_packet(s, e);
            }
            
            
            // We don't need to compute our routing table here so just return!
            return;
        }
        case SA_MASTER_TX:
        {
	  //printf("originator is %lu\n", m->originator);
            // Schedule ourselves again...
            ts = MASTER_SA_INTERVAL;
            e = tw_event_new(lp->gid, ts, lp);
            msg = tw_event_data(e);
            msg->type = SA_MASTER_TX;
            msg->originator = s->local_address;
            // Always send these to node zero, who receives all SA_MASTER msgs
            msg->destination = 0;
            msg->lng = s->lng;
            msg->lat = s->lat;
            tw_event_send(e);
                        
            // Send it on to node 0
            ts = g_tw_lookahead + tw_rand_unif(lp->rng) * HELLO_DELTA;
            e = tw_event_new(0, ts, lp);
            msg = tw_event_data(e);
            msg->type = SA_MASTER_RX;
            msg->originator = s->local_address;
            msg->sender = s->local_address;
            msg->destination = 0;
            msg->lng = s->lng;
            msg->lat = s->lat;
            tw_event_send(e);
            
            return;
        }
        case SA_MASTER_RX:
        {
	  //printf("RECEIVED SA_MASTER\n");
            return;
        }
        case RWALK_CHANGE:
        {
            //printf("Changing our location to %f, %f\n",
            //       m->lng, m->lat);
            s->lng = m->lng;
            s->lat = m->lat;
            
            // Build our initial RWALK_CHANGE messages
            ts = tw_rand_unif(lp->rng) * RWALK_INTERVAL + 1.0;
            e = tw_event_new(lp->gid, ts, lp);
            msg = tw_event_data(e);
            msg->type = RWALK_CHANGE;
            msg->lng = tw_rand_unif(lp->rng) * GRID_MAX;
            msg->lat = tw_rand_unif(lp->rng) * GRID_MAX;
            tw_event_send(e);
        }
            
        default:
            return;
    }
    
    RoutingTableComputation(s);
}

void olsr_final(node_state *s, tw_lp *lp)
{
    int i;
    
    if( OLSR_NO_FINAL_OUTPUT )
      return;

    if (s->local_address % OLSR_MAX_NEIGHBORS == 0) {
        for (i = 0; i < OLSR_MAX_NEIGHBORS; i++) {
            printf("node %lu had %d SA packets received.\n", s->local_address, s->SA_per_node[i]);
        }
    }
    
    printf("node %lu contains %d neighbors\n", s->local_address, s->num_neigh);
    printf("x: %f   \ty: %f\n", s->lng, s->lat);
    for (i = 0; i < s->num_neigh; i++) {
        printf("   neighbor[%d] is %lu\n", i, s->neighSet[i].neighborMainAddr);
        printf("   Dy(%lu) is %d\n", s->neighSet[i].neighborMainAddr,
               Dy(s, s->neighSet[i].neighborMainAddr));
    }
    
    printf("node %lu has %d two-hop neighbors\n", s->local_address, 
           s->num_two_hop);
    for (i = 0; i < s->num_two_hop; i++) {
        printf("   two-hop neighbor[%d] is %lu : %lu\n", i, 
               s->twoHopSet[i].neighborMainAddr,
               s->twoHopSet[i].twoHopNeighborAddr);
    }
    
    printf("node %lu has %d MPRs\n", s->local_address, 
           s->num_mpr);
    for (i = 0; i < s->num_mpr; i++) {
        printf("   MPR[%d] is %lu\n", i, 
               s->mprSet[i]);
    }
    
    printf("node %lu had %d MPR selectors\n", s->local_address, s->num_mpr_sel);
    
    printf("node %lu routing table\n", s->local_address);
    for (i = 0; i < s->num_routes; i++) {
        printf("   route[%d]: dest: %lu \t next %lu \t distance %d\n",
               i, s->route_table[i].destAddr,
               s->route_table[i].nextAddr, s->route_table[i].distance);
    }
    
    printf("node %lu top tuples\n", s->local_address);
    for (i = 0; i < s->num_top_set; i++) {
        printf("   top_tuple[%d] dest: %lu   last:  %lu   seq:   %d\n",
               i, s->topSet[i].destAddr, s->topSet[i].lastAddr, s->topSet[i].sequenceNumber);
    }
    
    /*
    for (i = 0; i < OLSR_MAX_NEIGHBORS; i++) {
        if (sqrt((s->lng - g_X[i]) * (s->lng - g_X[i]) +
                 (s->lat - g_Y[i]) * (s->lat - g_Y[i])) > RANGE) {
            printf("%lu and %d are out of range.\n", s->local_address, i);
        }
    }
    */
    printf("\n");
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
    TWOPT_STIME("lookahead", g_tw_lookahead, "lookahead for the simulation"),
    TWOPT_CHAR("rwalk", g_olsr_mobility, "random walk [Y/N]"),
    TWOPT_END(),
};

// Done mainly so doxygen will catch and differentiate this main
// from other mains while allowing smooth compilation.
#define olsr_main main

int olsr_main(int argc, char *argv[])
{
    int i;
    
    tw_opt_add(olsr_opts);
    tw_init(&argc, &argv);
    
    // nlp_per_pe = OLSR_MAX_NEIGHBORS;// / tw_nnodes();
   //g_tw_lookahead = SA_INTERVAL;
   g_tw_events_per_pe =  OLSR_MAX_NEIGHBORS / 2 * nlp_per_pe  + 65536;
   tw_define_lps(nlp_per_pe, sizeof(olsr_msg_data), 0);
   
    for(i = 0; i < OLSR_END_EVENT; i++)
        g_olsr_event_stats[i] = 0;
    
   for (i = 0; i < g_tw_nlp; i++) {
     tw_lp_settype(i, &olsr_lps[0]);
   }
    
    tw_run();
    
    if( g_tw_synchronization_protocol != 1 )
    {
        MPI_Reduce( g_olsr_event_stats, g_olsr_event_stats, OLSR_END_EVENT, MPI_LONG_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
    }
    
    if (tw_ismaster()) {
        for( i = 0; i < OLSR_END_EVENT; i++ )
            printf("OLSR Type %d Event Count = %llu \n", i, g_olsr_event_stats[i]);
        printf("Complete.\n");
    }
    
    tw_end();
    
    return 0;
}
