#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

/* Copied and modified from http://pine.cs.yale.edu/pinewiki/C/AvlTree google cache */

#include "avl_tree.h"

/* implementation of an AVL tree with explicit heights */

struct avlNode {
  struct avlNode *child[2];    /* left and right */
  tw_event *key;
  int height;
};

/* free a tree */
void 
avlDestroy(AvlTree t)
{
  if (t != AVL_EMPTY) {
    avlDestroy(t->child[0]);
    avlDestroy(t->child[1]);
    free(t);
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
  else if (t->key->recv_ts == key->recv_ts) {
    return 1;
  } 
  else {
    if (t->key->recv_ts > key->recv_ts) {
      return avlSearch(t->child[1], key);
    }
    return avlSearch(t->child[0], key);
  }
}

#define Max(x,y) ((x)>(y) ? (x) : (y))

/* assert height fields are correct throughout tree */
void
avlSanityCheck(AvlTree root)
{
  int i;

  if (root != AVL_EMPTY) {
    for (i = 0; i < 2; i++) {
      avlSanityCheck(root->child[i]);
    }

    assert(root->height == 1 + Max(avlGetHeight(root->child[0]), avlGetHeight(root->child[1])));
  }
}

/* recompute height of a node */
static void
avlFixHeight(AvlTree t)
{
  assert(t != AVL_EMPTY);

  t->height = 1 + Max(avlGetHeight(t->child[0]), avlGetHeight(t->child[1]));
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
    *t = malloc(sizeof(struct avlNode));
    assert(*t);

    (*t)->child[0] = AVL_EMPTY;
    (*t)->child[1] = AVL_EMPTY;

    (*t)->key = key;

    (*t)->height = 1;

    /* done */
    return;
  }
  else if (key->recv_ts == (*t)->key->recv_ts) {
    /* nothing to do */
    return;
  } 
  else {
    /* do the insert in subtree */
    if (key->recv_ts > (*t)->key->recv_ts) {
      avlInsert(&(*t)->child[1], key);
    }
    else {
      avlInsert(&(*t)->child[0], key);
    }
    //        avlInsert(&(*t)->child[key > (*t)->key], key);

    avlRebalance(t);

    return;
  }
}


/* print all elements of the tree in order */
void
avlPrintKeys(AvlTree t)
{
  if (t != AVL_EMPTY) {
    avlPrintKeys(t->child[0]);
    printf("%f\n", t->key->recv_ts);
    avlPrintKeys(t->child[1]);
  }
}


/* delete and return minimum value in a tree */
tw_event *
avlDeleteMin(AvlTree *t)
{
  AvlTree oldroot;
  tw_event *event_with_lowest_ts;
  //double minValue;

  assert(t != AVL_EMPTY);

  if ((*t)->child[0] == AVL_EMPTY) {
    /* root is min value */
    oldroot = *t;
    event_with_lowest_ts = oldroot->key;
    *t = oldroot->child[1];
    free(oldroot);
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
  tw_event *target;
  AvlTree oldroot;
  
  /* if(*t != AVL_EMPTY) { */
  /*   assert(0 && "We never look for non-existent events!"); */
  /*   return NULL; */
  /* } */
  /* else  */
  if ((*t)->key->recv_ts == key->recv_ts) {
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
      free(oldroot);
    }
    avlRebalance(t);
    return target;
  }
  else {
    if (key->recv_ts > (*t)->key->recv_ts) {
      target = avlDelete(&(*t)->child[1], key);
      avlRebalance(t);
      return target;
    }
    target = avlDelete(&(*t)->child[0], key);
    avlRebalance(t);
    return target;
  }
  
  /* rebalance */
  avlRebalance(t);
}
