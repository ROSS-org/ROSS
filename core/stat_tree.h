
/* Copied and modified from http://pine.cs.yale.edu/pinewiki/C/AvlTree google cache */

/* implementation of an AVL tree with explicit heights */
/****
 * This is just to be able to run without ross
 ****/
#include <math.h>
#include <ross.h>

#define NUM_INTERVAL_STATS 11
/***
 * end
 *****/

// TODO set up enum for accessing correct location in array
struct stats_bin {
/*  
    0  FORWARD_EV,
    1  REVERSE_EV,
    2  NUM_GVT,
    3  NUM_ALLREDUCE,
    4  EVENTS_ABORTED,
    5  PE_EVENT_TIES,
    6  REMOTE_SEND,
    7  REMOTE_RECV,
    8  RB_PRIMARY,
    9 RB_SECONDARY,
    10 FC_ATTEMPTS
    */
    tw_stat stats[NUM_INTERVAL_STATS];
};

struct stat_node {
  struct stat_node *child[2];    /* left and right */
  long key;                    /* beginning time for this bin */
  int height;
  stats_bin bin;
};

/* empty stat tree is just a null pointer */

#define AVL_EMPTY (0)

stat_node *stat_find_min(stat_node *t);
stat_node *stat_find_max(stat_node *t);
stat_node *stat_init_tree(tw_stat start);

stat_node *stat_increment(stat_node *t, long time_stamp, int stat_type, stat_node *root, int amount);

stat_node *stat_add_nodes(stat_node *root);

/* return the height of a tree */
int stat_get_height(stat_node *t);

/* run sanity checks on tree (for debugging) */
/* assert will fail if heights are wrong */
void stat_sanity_check(stat_node *t);

stat_node *gvt_write_bins(FILE *log, stat_node *t, tw_stime gvt);

/* free a tree */
void stat_destroy(stat_node *t);

void stat_free(stat_node *t);

/* print all keys of the tree in order */
void stat_print_keys(stat_node *t);

/* delete and return minimum value in a tree */
stat_node *stat_delete_min(stat_node *t, stat_node *parent);
stat_node *stat_delete_max(stat_node *t, stat_node *parent);

// Should only need to delete at GVT
// could be 1 bin or many, so write necessary bins to file, delete those bins,
// then rebalance the tree
stat_node *stat_delete(stat_node *t, long key, stat_node *parent, stat_node *root);
void debug_nodes(stat_node *root, long *counter, tw_stime lvt);
