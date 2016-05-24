#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

/* Copied and modified from http://pine.cs.yale.edu/pinewiki/C/AvlTree google cache */

#include "stat_tree.h"
long g_tw_min_bin = 0;
long g_tw_max_bin = 0;
tw_clock stat_write_cycle_counter = 0;
tw_clock stat_comp_cycle_counter = 0;


/* find min bin in tree */
stat_node *stat_find_min(stat_node *t)
{
    if (t == AVL_EMPTY)
        return NULL;

    if (t->child[0])
        return stat_find_min(t->child[0]);
    else 
        return t;
}

/* find max bin in tree */
stat_node *stat_find_max(stat_node *t)
{
    if (t == AVL_EMPTY)
        return NULL;

    if (t->child[1])
        return stat_find_max(t->child[1]);
    else 
        return t;
}

/* recursive function for creating a tree for stats collection. 
 * takes in the current node, uses in order traversal.
 */
//TODO need to handle small special cases
static long create_tree(stat_node *current_node, tw_stat key, int height)
{
    if (height > 1)
    {
        current_node->child[0] = tw_calloc(TW_LOC, "statistics collection (tree)", sizeof(struct stat_node), 1);
        current_node->child[1] = tw_calloc(TW_LOC, "statistics collection (tree)", sizeof(struct stat_node), 1);
        int k1 = create_tree(current_node->child[0], key, height-1);
        current_node->key = k1 + g_tw_time_interval;
        current_node->height = height;
        create_tree(current_node->child[1], (current_node->key) + g_tw_time_interval, height-1);
        stat_node *maxptr = stat_find_max(current_node);
        return maxptr->key;
    }
    else if (height == 1)
    {
        current_node->key = key;
        current_node->height = height;
        return key;
    }
    return -1;
}

/* initialize the tree with some number of bins.
 * currently about 10% of bins expected based on end time and time interval
 * returns pointer to the root
 */
stat_node *stat_init_tree(tw_stat start)
{
    tw_clock start_cycle_time = tw_clock_read();
    
    // TODO need to do some stuff to make sure too many bins aren't created
    int height = 8;
    //tw_stat end = num_bins * g_tw_time_interval;
    stat_node *root = tw_calloc(TW_LOC, "statistics collection (tree)", sizeof(struct stat_node), 3);

    root->child[0] = root + 1;
    root->child[1] = root + 2;
    long k = create_tree(root->child[0], start, height-1);
    root->key = k + g_tw_time_interval;
    root->height = height;
    create_tree(root->child[1], (root->key) + g_tw_time_interval, height-1);
    stat_comp_cycle_counter += tw_clock_read() - start_cycle_time;
    return root;
}

/* return nonzero if key is present in tree */
// TODO need to make sure that original caller adds bins if it gets a return value == 0
stat_node *stat_increment(stat_node *t, long time_stamp, int stat_type, stat_node *root, int amount)
{
    // check that there is a bin for this time stamp
    if (time_stamp > g_tw_max_bin + g_tw_time_interval)
    {
        //printf("adding nodes to the tree\n");
        root = stat_add_nodes(root);
        stat_node *tmp = stat_find_max(root);
        g_tw_max_bin = tmp->key;
    }

    tw_clock start_cycle_time = tw_clock_read();
    // find bin this time stamp belongs to
    int key = floor(time_stamp / g_tw_time_interval) * g_tw_time_interval;
    if (t == AVL_EMPTY) {
        return root;
    }
    
    if (key == t->key) 
    {
        t->bin.stats[stat_type] += amount;
    }

    if (key < t->key)
    {
        if (t->child[0])
        {
            stat_increment(t->child[0], time_stamp, stat_type, root, amount);
        }
    }
    else if (key > t->key)
    {
        if (t->child[1])
        {
            stat_increment(t->child[1], time_stamp, stat_type, root, amount);
        }
    }
    stat_write_cycle_counter += tw_clock_read() - start_cycle_time;
    return root;
}

/* recompute height of a node */
static void fix_node_height(stat_node *t)
{
    assert(t != AVL_EMPTY);
    
    t->height = 1 + ROSS_MAX(stat_get_height(t->child[0]), stat_get_height(t->child[1]));
}

static void fix_all_heights(stat_node *root)
{
    if (root != AVL_EMPTY)
    {
        if (root->child[0])
            fix_all_heights(root->child[0]);
        if (root->child[1])
            fix_all_heights(root->child[1]);
        fix_node_height(root);
    }
}

/* rotate child[d] to root */
/* assumes child[d] exists */
/* Picture:
 *
 *     y            x
 *    / \   <==>   / \
 *   x   C        A   y
 *  / \              / \
 * A   B            B   C
 *
 */
static stat_node *statRotate(stat_node *root, int d)
{
    stat_node *oldRoot;
    stat_node *newRoot;
    stat_node *oldMiddle;
    
    oldRoot = root;
    newRoot = oldRoot->child[d];
    oldMiddle = newRoot->child[!d];
    
    oldRoot->child[d] = oldMiddle;
    newRoot->child[!d] = oldRoot;
    root = newRoot;
    
    /* update heights */
    fix_node_height((root)->child[!d]);   /* old root */
    fix_node_height(root);                /* new root */
    return root;
}

/* rebalance at node if necessary */
/* also fixes height */
// TODO works correctly, but could probably be better done
// because of the situation, the right subtree usually has more nodes after deletions
// potentially rebalance more often would help solve this
static stat_node *statRebalance(stat_node *t)
{
    fix_all_heights(t);
    int d;
    
    if (t != AVL_EMPTY) {
        for (d = 0; d < 2; d++) {
            /* maybe child[d] is now too tall */
            if (stat_get_height((t)->child[d]) > stat_get_height((t)->child[!d]) + 1) {
                /* imbalanced! */
                /* how to fix it? */
                /* need to look for taller grandchild of child[d] */
                if (stat_get_height((t)->child[d]->child[d]) > stat_get_height((t)->child[d]->child[!d])) {
                    /* same direction grandchild wins, do single rotation */
                    t = statRotate(t, d);
                }
                else {
                    /* opposite direction grandchild moves up, do double rotation */
                    stat_node *temp = statRotate((t)->child[d], !d);
                    t->child[d] = temp;
                    t = statRotate(t, d);
                }
                
                //return;   /* statRotate called fix_node_height */
            }
        }
        
        /* update height */
        fix_all_heights(t);
    }
    return t;
}

void insert_node(stat_node *root, stat_node *t)
{
    if(t == AVL_EMPTY) {
        return;
    } 
    if (t->key < root->key) {
        /* do the insert in subtree */
        if (root->child[0])
            insert_node(root->child[0], t);
        else
            root->child[0] = t;
    
    }
    else if (t->key > root->key){
        if (root->child[1])
            insert_node(root->child[1], t);
        else
            root->child[1] = t;
        
    }
    //    statRebalance(t);

    return;
}


/* determined that more bins should be added */
stat_node *stat_add_nodes(stat_node *root)
{
    tw_clock start_cycle_time = tw_clock_read();;
    stat_node *old_root = root;
    stat_node *max_node = stat_delete_max(root, NULL);
    tw_stat start = max_node->key + g_tw_time_interval;
    stat_write_cycle_counter += tw_clock_read() - start_cycle_time;

    stat_node *subtree = stat_init_tree(start);

    if (!old_root->child[0] && !old_root->child[1])
    {
        insert_node(subtree, old_root);
        root = subtree;
    }
    else
    {
        max_node->child[0] = old_root;
        // connect subtree to tree
        max_node->child[1] = subtree;
        root = max_node;
    }

    fix_all_heights(root);
    root = statRebalance(root);
    return root;
}

/* return height of an AVL tree */
int stat_get_height(stat_node *t)
{
    if (t != AVL_EMPTY) {
        return t->height;
    }
    else {
        return 0;
    }
}

/* assert height fields are correct throughout tree */
void stat_sanity_check(stat_node *root)
{
    int i;
    
    if (root != AVL_EMPTY) {
        for (i = 0; i < 2; i++) {
            stat_sanity_check(root->child[i]);
        }
        
        assert(root->height == 1 + ROSS_MAX(stat_get_height(root->child[0]), stat_get_height(root->child[1])));
    }
}

static stat_node *write_bins(FILE *log, stat_node *t, tw_stime gvt, stat_node *parent, stat_node *root, int *flag)
{
    if (t != AVL_EMPTY)
    {
        if (t->child[0])
            root = write_bins(log, t->child[0], gvt, t, root, flag);
        if (t->key + g_tw_time_interval <= gvt && t->key <= g_tw_ts_end)
        { // write in order
            char buffer[2048] = {0};
            char tmp[32] = {0};
            sprintf(tmp, "%ld,%ld,", g_tw_mynode, t->key);
            strcat(buffer, tmp);
            int i;
            for (i = 0; i < NUM_INTERVAL_STATS; i++)
            {
                sprintf(tmp, "%llu", t->bin.stats[i]);
                strcat(buffer, tmp);
                if (i != NUM_INTERVAL_STATS-1)
                    sprintf(tmp, ",");
                else
                    sprintf(tmp, "\n");
                strcat(buffer, tmp);
            }

            tw_clock start_cycle_time = tw_clock_read();;
            MPI_File_write(interval_file, buffer, strlen(buffer), MPI_CHAR, MPI_STATUS_IGNORE);
            stat_write_cycle_counter += tw_clock_read() - start_cycle_time;
        }
        if (t->child[1] && *flag)
            root = write_bins(log, t->child[1], gvt, t, root, flag);
        
        if (t->key + g_tw_time_interval <= gvt)
        {
            // can delete this node now
            root = stat_delete(t, t->key, parent, root);
            //free(t);
            //if (parent != root)
            //    statRebalance(parent);
            *flag = 1;
        }
        else // no need to continue checking nodes
            *flag = 0;
    }

    return root;
}

/* call after GVT; write bins < GVT to file and delete */
stat_node *gvt_write_bins(FILE *log, stat_node *t, tw_stime gvt)
{
    if (g_tw_mynode == g_tw_masternode)
    {
        //printf("\n\nprinting tree at gvt: %f\n", gvt);
        //stat_print_keys(t);
        long counter = g_tw_min_bin;
        debug_nodes(t, &counter, gvt);
    }
    tw_clock start_cycle_time = tw_clock_read();
    int flag = 1;
    t = write_bins(log, t, gvt, NULL, t, &flag);
    t = statRebalance(t);
    stat_write_cycle_counter += tw_clock_read() - start_cycle_time;
    
    stat_node *tmp = stat_find_min(t);
    g_tw_min_bin = tmp->key;
    tmp = stat_find_max(t);
    g_tw_max_bin = tmp->key;
    return t;
}

/* free a tree */
void stat_destroy(stat_node *t)
{
    if (t != AVL_EMPTY) {
        stat_destroy(t->child[0]);
        t->child[0] = AVL_EMPTY;
        stat_destroy(t->child[1]);
        t->child[1] = AVL_EMPTY;
        stat_free(t);
    }
}

void stat_free(stat_node *t)
{
    (t)->child[0] = AVL_EMPTY;
    (t)->child[1] = AVL_EMPTY;
    (t)->key = -1;
    (t)->height = 0;
}

/* print all elements of the tree in order */
void stat_print_keys(stat_node *t)
{
    if (t != AVL_EMPTY) {
        stat_print_keys(t->child[0]);
        printf("%ld\t%d\n", t->key, t->height);
        stat_print_keys(t->child[1]);
    }
}

/* delete and return minimum value in a tree */
// Need to make sure to rebalance after calling
stat_node *stat_delete_min(stat_node *t, stat_node *parent)
{
    stat_node *oldroot;
    stat_node *min_node;

    assert(t != AVL_EMPTY);

    if((t)->child[0] == AVL_EMPTY) {
        /* root is min value */
        min_node = t;
        if (parent != AVL_EMPTY)
        {
            if (t->key < parent->key)
                parent->child[0] = t->child[1];
            else
                parent->child[1] = t->child[1];
        }
        //free(oldroot);
    } else {
        /* min value is in left subtree */
        min_node = stat_delete_min((t)->child[0], t);
    }

    return min_node;
}

/* delete and return max node in a tree */
// Need to make sure to rebalance after calling
stat_node *stat_delete_max(stat_node *t, stat_node *parent)
{
    stat_node *max_node;

    assert(t != AVL_EMPTY);

    if((t)->child[1] == AVL_EMPTY) {
        /* root is max value */
        max_node = t;
        if (parent)
            parent->child[1] = NULL;
    } else {
        /* max value is in right subtree */
        max_node = stat_delete_max((t)->child[1], t);
    }

    return max_node;
}

/* delete the given value */
// Need to make sure to rebalance after calling
stat_node *stat_delete(stat_node *t, long key, stat_node *parent, stat_node *root)
{
    stat_node *oldroot;
    stat_node *newroot = NULL;

    if(t == AVL_EMPTY) {
        return root;
    } else if((t)->key == key) {
        /* do we have a right child? */
        if((t)->child[1] != AVL_EMPTY) {
            /* give root min value in right subtree */
            oldroot = t;
            stat_node *temp = stat_delete_min(oldroot->child[1], oldroot);
            if (parent != AVL_EMPTY){
                if (oldroot->key < parent->key)
                    parent->child[0] = temp; 
                else
                    parent->child[1] = temp;
            }
            else
                newroot = temp; 
            temp->child[1] = oldroot->child[1];
            //free(oldroot);
        } else {
            /* splice out root */
            if(parent != AVL_EMPTY){
                if (t->key < parent->key)
                    parent->child[0] = NULL;
                else 
                    parent->child[1] = NULL;
            }
            else
                newroot = t;
            oldroot = (t);
            t = (t)->child[0];
            
            //free(oldroot);
        }
    } else {
        stat_delete((t)->child[key > (t)->key], key, t, root);
    }
    if (!newroot)
        newroot = root;
    return newroot;
}

/* run through tree to check that it is correct */
void debug_nodes(stat_node *root, long *counter, tw_stime lvt)
{
    if (root == AVL_EMPTY)
        return;
    if (root->child[0] != AVL_EMPTY)
        debug_nodes(root->child[0], counter, lvt);
    if (root->key != *counter)
        printf("tree is incorrect at virtual time: %f\n", lvt);
    *counter += g_tw_time_interval;
    if (root->child[1] != AVL_EMPTY)
        debug_nodes(root->child[1], counter, lvt);

} 
