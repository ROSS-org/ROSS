#include <ross.h>

/* Copied and modified from http://pine.cs.yale.edu/pinewiki/C/AvlTree google cache */

/* implementation of an AVL tree with explicit heights */

struct avlNode {
  struct avlNode *child[2];    /* left and right */
  tw_event *key;
  int height;
  struct avlNode *next;        /* for ROSS weird linked-list memory */
};

/* empty avl tree is just a null pointer */

#define AVL_EMPTY (0)

/* free a tree */
void avlDestroy(AvlTree t);

/* return the height of a tree */
int avlGetHeight(AvlTree t);

/* return nonzero if key is present in tree */
int avlSearch(AvlTree t, tw_event *key);

/* insert a new element into a tree */
/* note *t is actual tree */
void avlInsert(AvlTree *t, tw_event *key);

/* run sanity checks on tree (for debugging) */
/* assert will fail if heights are wrong */
void avlSanityCheck(AvlTree t);

/* print all keys of the tree in order */
void avlPrintKeys(AvlTree t);

/* delete and return minimum value in a tree */
tw_event * avlDeleteMin(AvlTree *t);

tw_event * avlDelete(AvlTree *t, tw_event *key);

AvlTree avl_alloc(void);

void avl_free(AvlTree t);
