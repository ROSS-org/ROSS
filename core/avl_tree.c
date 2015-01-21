#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

/* Copied and modified from http://pine.cs.yale.edu/pinewiki/C/AvlTree google cache */

#include "avl_tree.h"

/* implementation of an AVL tree with explicit heights */

tw_pe *pe;

/* free a tree */
void
avlDestroy(AvlTree t)
{
    if (t != AVL_EMPTY) {
        avlDestroy(t->child[0]);
        t->child[0] = AVL_EMPTY;
        avlDestroy(t->child[1]);
        t->child[1] = AVL_EMPTY;
        avl_free(t);
    }
}

/* return height of an AVL tree */
int
avlGetHeight(AvlTree t)
{
    if (t != AVL_EMPTY) {
        return t->height;
    }
    else {
        return 0;
    }
}

/* return nonzero if key is present in tree */
int
avlSearch(AvlTree t, tw_event *key)
{
    if (t == AVL_EMPTY) {
        return 0;
    }
    
    if (key->recv_ts == t->key->recv_ts) {
        // Timestamp is the same
        if (key->event_id == t->key->event_id) {
            // Event ID is the same
            if (key->send_pe == t->key->send_pe) {
                // send_pe is the same
                return 1;
            }
            else {
                // send_pe is different
                return avlSearch(t->child[key->send_pe > t->key->send_pe], key);
            }
        }
        else {
            // Event ID is different
            return avlSearch(t->child[key->event_id > t->key->event_id], key);
        }
    }
    else {
        // Timestamp is different
        return avlSearch(t->child[key->recv_ts > t->key->recv_ts], key);
    }
}

/* assert height fields are correct throughout tree */
void
avlSanityCheck(AvlTree root)
{
    int i;
    
    if (root != AVL_EMPTY) {
        for (i = 0; i < 2; i++) {
            avlSanityCheck(root->child[i]);
        }
        
        assert(root->height == 1 + ROSS_MAX(avlGetHeight(root->child[0]), avlGetHeight(root->child[1])));
    }
}

/* recompute height of a node */
static void
avlFixHeight(AvlTree t)
{
    assert(t != AVL_EMPTY);
    
    t->height = 1 + ROSS_MAX(avlGetHeight(t->child[0]), avlGetHeight(t->child[1]));
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
static void
avlRotate(AvlTree *root, int d)
{
    AvlTree oldRoot;
    AvlTree newRoot;
    AvlTree oldMiddle;
    
    oldRoot = *root;
    newRoot = oldRoot->child[d];
    oldMiddle = newRoot->child[!d];
    
    oldRoot->child[d] = oldMiddle;
    newRoot->child[!d] = oldRoot;
    *root = newRoot;
    
    /* update heights */
    avlFixHeight((*root)->child[!d]);   /* old root */
    avlFixHeight(*root);                /* new root */
}


/* rebalance at node if necessary */
/* also fixes height */
static void
avlRebalance(AvlTree *t)
{
    int d;
    
    if (*t != AVL_EMPTY) {
        for (d = 0; d < 2; d++) {
            /* maybe child[d] is now too tall */
            if (avlGetHeight((*t)->child[d]) > avlGetHeight((*t)->child[!d]) + 1) {
                /* imbalanced! */
                /* how to fix it? */
                /* need to look for taller grandchild of child[d] */
                if (avlGetHeight((*t)->child[d]->child[d]) > avlGetHeight((*t)->child[d]->child[!d])) {
                    /* same direction grandchild wins, do single rotation */
                    avlRotate(t, d);
                }
                else {
                    /* opposite direction grandchild moves up, do double rotation */
                    avlRotate(&(*t)->child[d], !d);
                    avlRotate(t, d);
                }
                
                return;   /* avlRotate called avlFixHeight */
            }
        }
        
        /* update height */
        avlFixHeight(*t);
    }
}

/* insert into tree */
/* this may replace root, which is why we pass
 * in a AvlTree * */
void
avlInsert(AvlTree *t, tw_event *key)
{
    /* insertion procedure */
    if (*t == AVL_EMPTY) {
        /* new t */
        *t = avl_alloc();
        if (*t == NULL) {
            tw_error(TW_LOC, "Out of AVL tree nodes!");
        }
        
        (*t)->child[0] = AVL_EMPTY;
        (*t)->child[1] = AVL_EMPTY;
        
        (*t)->key = key;
        
        (*t)->height = 1;
        
        /* done */
        return;
    }
    
    if (key->recv_ts == (*t)->key->recv_ts) {
        // We have a timestamp tie, check the event ID
        if (key->event_id == (*t)->key->event_id) {
            // We have a event ID tie, check the send_pe
            if (key->send_pe == (*t)->key->send_pe) {
                // This shouldn't happen but we'll allow it
                tw_printf(TW_LOC, "The events are identical!!!\n");
            }
            avlInsert(&(*t)->child[key->send_pe > (*t)->key->send_pe], key);
            avlRebalance(t);
        }
        else {
            // Event IDs are different
            avlInsert(&(*t)->child[key->event_id > (*t)->key->event_id], key);
            avlRebalance(t);
        }
        return;
    }
    else {
        // Timestamps are different
        avlInsert(&(*t)->child[key->recv_ts > (*t)->key->recv_ts], key);
        avlRebalance(t);
    }
}


/* print all elements of the tree in order */
void
avlPrintKeys(AvlTree t)
{
    if (t != AVL_EMPTY) {
        avlPrintKeys(t->child[0]);
        //printf("%f\n", t->key->recv_ts);
        avlPrintKeys(t->child[1]);
    }
}


/* delete and return minimum value in a tree */
tw_event *
avlDeleteMin(AvlTree *t)
{
    AvlTree oldroot;
    tw_event *event_with_lowest_ts = NULL;
    
    assert(t != AVL_EMPTY);
    
    if ((*t)->child[0] == AVL_EMPTY) {
        /* root is min value */
        oldroot = *t;
        event_with_lowest_ts = oldroot->key;
        *t = oldroot->child[1];
        avl_free(oldroot);
    }
    else {
        /* min value is in left subtree */
        event_with_lowest_ts = avlDeleteMin(&(*t)->child[0]);
    }
    
    avlRebalance(t);
    return event_with_lowest_ts;
}

/* delete the given value */
tw_event *
avlDelete(AvlTree *t, tw_event *key)
{
    tw_event *target = NULL;
    AvlTree oldroot;
    
    if (*t == AVL_EMPTY) {
        tw_error(TW_LOC, "We never look for non-existent events!");
        return target;
    }
    
    if (key->recv_ts == (*t)->key->recv_ts) {
        // We have a timestamp tie, check the event ID
        if (key->event_id == (*t)->key->event_id) {
            // We have a event ID tie, check the send_pe
            if (key->send_pe == (*t)->key->send_pe) {
                // This is actually the one we want to delete
                target = (*t)->key;
                /* do we have a right child? */
                if ((*t)->child[1] != AVL_EMPTY) {
                    /* give root min value in right subtree */
                    (*t)->key = avlDeleteMin(&(*t)->child[1]);
                }
                else {
                    /* splice out root */
                    oldroot = (*t);
                    *t = (*t)->child[0];
                    avl_free(oldroot);
                }
            }
            else {
                // Timestamp and event IDs are the same, but different send_pe
                target = avlDelete(&(*t)->child[key->send_pe > (*t)->key->send_pe], key);
            }
        }
        else {
            // Timestamps are the same but event IDs differ
            target = avlDelete(&(*t)->child[key->event_id > (*t)->key->event_id], key);
        }
    }
    else {
        // Timestamps are different
        target = avlDelete(&(*t)->child[key->recv_ts > (*t)->key->recv_ts], key);
    }
    
    avlRebalance(t);
    
    return target;
}

AvlTree avl_alloc(void)
{
    AvlTree head = g_tw_pe[0]->avl_list_head;
    g_tw_pe[0]->avl_list_head = head->next;
    
    if (g_tw_pe[0]->avl_list_head == NULL) {
        tw_error(TW_LOC, "avl_list_head is invalid!");
    }
    
    head->next = NULL;
    
    return head;
}

void avl_free(AvlTree t)
{
    (t)->child[0] = AVL_EMPTY;
    (t)->child[1] = AVL_EMPTY;
    (t)->next = NULL;
    (t)->key = NULL;
    (t)->height = 0;
    (t)->next = g_tw_pe[0]->avl_list_head;
    g_tw_pe[0]->avl_list_head = t;
}
