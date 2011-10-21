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
    
    //s->num_tuples = 0;
    s->num_neigh  = 0;
    s->num_two_hop = 0;
    s->num_mpr = 0;
    s->num_mpr_sel = 0;
    s->num_top_set = 0;
    s->local_address = lp->gid;
    s->lng = tw_rand_unif(lp->rng) * GRID_MAX;
    s->lat = tw_rand_unif(lp->rng) * GRID_MAX;
    
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
    t->num_mpr_sel = 0;
    tw_event_send(e);
}

#define RANGE 40.0

static inline int out_of_radio_range(node_state *s, olsr_msg_data *m)
{
    const double range = RANGE;
    
    double sender_lng = m->lng;
    double sender_lat = m->lat;
    double receiver_lng = s->lng;
    double receiver_lat = s->lat;
    
    double dist = (sender_lng - receiver_lng) * (sender_lng - receiver_lng);
    dist += (sender_lat - receiver_lat) * (sender_lat - receiver_lat);
    
    dist = sqrt(dist);
    
    if (dist > range) {
        return 1;
    }
    
    return 0;
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
        g_mpr_two_hop[index_to_remove] = g_mpr_two_hop[g_num_two_hop-1];
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
        
        s->topSet[index_to_remove] = s->topSet[s->num_top_set-1];
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
            RT_entry *destAddrEntry = Lookup(s, s->topSet[i].destAddr);
            RT_entry *lastAddrEntry = Lookup(s, s->topSet[i].lastAddr);
            if (!destAddrEntry && lastAddrEntry && lastAddrEntry->distance == h) {
                s->route_table[s->num_routes].destAddr = s->topSet[i].destAddr;
                s->route_table[s->num_routes].nextAddr = lastAddrEntry->nextAddr;
                s->route_table[s->num_routes].distance = h + 1;
                added = 1;
            }
        }
        
        if (!added) {
            break;
        }
    }
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
    
    switch(m->type) {
        case HELLO_TX:
            ts = tw_rand_exponential(lp->rng, HELLO_DELTA);
            
            cur_lp = g_tw_lp[0];
            
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
            
        case HELLO_RX:
            
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
                    
                    // Make sure they're all unique!
                    mpr_set_uniq(s);
                    
                    // take note of all the 2-hop neighbors reachable by the newly elected MPR
                    for (j = 0; j < g_num_two_hop; j++) {
                        if (g_mpr_two_hop[j].neighborMainAddr == g_mpr_two_hop[i].neighborMainAddr) {
                            //coveredTwoHopNeighbors.insert (otherTwoHopNeigh->twoHopNeighborAddr);
                            // We don't do that, we use bitfields.  Make sure
                            // our assumptions are correct then create a mask
                            assert(g_mpr_two_hop[j].neighborMainAddr < OLSR_MAX_NEIGHBORS);
                            BITSET(g_covered, g_mpr_two_hop[j].twoHopNeighborAddr);
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
                if (BITTEST(g_covered, g_mpr_two_hop[i].twoHopNeighborAddr)) {
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
                    assert(g_mpr_one_hop[i].neighborMainAddr < OLSR_MAX_NEIGHBORS);
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
                    
                    // Make sure they're all unique!
                    mpr_set_uniq(s);
                    
                    // take note of all the 2-hop neighbors reachable by the newly elected MPR
                    for (j = 0; j < g_num_two_hop; j++) {
                        if (g_mpr_two_hop[j].neighborMainAddr == g_mpr_neigh_to_add.neighborMainAddr) {
                            //coveredTwoHopNeighbors.insert (otherTwoHopNeigh->twoHopNeighborAddr);
                            // We don't do that, we use bitfields.  Make sure
                            // our assumptions are correct then create a mask
                            assert(g_mpr_two_hop[j].neighborMainAddr < OLSR_MAX_NEIGHBORS);
                            BITSET(g_covered, g_mpr_two_hop[j].twoHopNeighborAddr);
                        }
                    }
                }
                
                // Remove the nodes from N2 which are now covered by a node in the MPR set.
                for (i = 0; i < g_num_two_hop; i++) {
                    if (BITTEST(g_covered, g_mpr_two_hop[i].twoHopNeighborAddr)) {
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
                    }
                }
            }
            
            // END MPR SELECTOR SET
            
            break;
            
        case TC_TX:
            // Might want to rename HELLO_DELTA...
            ts = tw_rand_exponential(lp->rng, HELLO_DELTA);
            
            cur_lp = g_tw_lp[0];
            
            e = tw_event_new(cur_lp->gid, ts, lp);
            msg = tw_event_data(e);
            msg->type = TC_RX;
            msg->ttl = 255;
            msg->originator = m->originator;
            msg->sender = s->local_address;
            msg->lng = s->lng;
            msg->lat = s->lat;
            msg->target = 0;
            t = &msg->mt.t;
            t->ansn = s->ansn;
            t->num_mpr_sel = s->num_mpr_sel;
            for (j = 0; j < s->num_mpr_sel; j++) {
                t->neighborAddresses[j] = s->mprSelSet[j].mainAddr;
            }
            if (s->num_mpr_sel > 0) {
                tw_event_send(e);
            }
            //tw_event_send(e);
            
            e = tw_event_new(lp->gid, HELLO_INTERVAL, lp);
            msg = tw_event_data(e);
            msg->type = TC_TX;
            msg->originator = s->local_address;
            msg->lng = s->lng;
            msg->lat = s->lat;
            t = &msg->mt.t;
            t->num_mpr_sel = 0;
            tw_event_send(e);
            
            break;
            
        case TC_RX:
            
            // 1. When a packet arrives at the router, the router evaluates the TTL.
            // 2a. If the TTL=0, the router drops the packet, and sends an ICMP TTL Exceeded message to the source.
            // 2b. If the TTL>0, then the router decrements the TTL by 1, and then forwards the packet.
            // From http://www.dslreports.com/forum/r25655787-When-is-TTL-Decremented-by-a-Router-
            
            if (m->ttl == 0)
                return;
            
            m->ttl--;
            
            // Copy the message we just received; we can't add data to
            // a message sent by another node
            
            if (m->target < g_tw_nlp - 1) {
                ts = tw_rand_exponential(lp->rng, HELLO_DELTA);
                
                tw_lp *cur_lp = g_tw_lp[m->target + 1];
                
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
                t->num_mpr_sel = m->mt.t.num_mpr_sel;
                for (j = 0; j < t->num_mpr_sel; j++) {
                    t->neighborAddresses[j] = m->mt.t.neighborAddresses[j];
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
            
            // BEGIN TC PROCESSING
            
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
            
            // 4. For each of the advertised neighbor main address received in
            // the TC message:
            for (i = 0; i < m->mt.t.num_mpr_sel; i++) {
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
                    s->num_top_set++;
                    assert(s->num_top_set < OLSR_MAX_TOP_TUPLES);
                    s->topSet[s->num_top_set-1].destAddr = addr;
                    s->topSet[s->num_top_set-1].lastAddr = m->originator;
                    s->topSet[s->num_top_set-1].sequenceNumber = m->mt.t.ansn;
#warning "Correct this line - TOP_HOLD_TIME should be in the struct!"
                    s->topSet[s->num_top_set-1].expirationTime = tw_now(lp) + TOP_HOLD_TIME;
                }
            }
            
            // END TC PROCESSING
            
            
            break;
    }
    
    RoutingTableComputation(s);
}

void olsr_final(node_state *s, tw_lp *lp)
{
    int i;
    
    printf("node %lu contains %d neighbors\n", s->local_address, s->num_neigh);
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

// Done mainly so doxygen will catch and differentiate this main
// from other mains while allowing smooth compilation.
#define olsr_main main

int olsr_main(int argc, char *argv[])
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