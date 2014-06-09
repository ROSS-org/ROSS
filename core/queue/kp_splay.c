/*
 * queue-splay.c :- splay tree for priority queue
 * 
 * THIS IMPLEMENTATION IS ADAPTED FROM THE DASSF 
 * C++ IMPLEMENTATION.
 * THEIR COPYRIGHT IS ATTACHED BELOW
 */

/*
 * Copyright (c) 1998-2002 Dartmouth College
 *
 * Permission is hereby granted, free of charge, to any individual or 
 * institution obtaining a copy of this software and associated 
 * documentation files (the "Software"), to use, copy, modify, and 
 * distribute without restriction, provided that this copyright and 
 * permission notice is maintained, intact, in all copies and supporting 
 * documentation.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL DARTMOUTH COLLEGE BE LIABLE FOR ANY CLAIM, DAMAGES 
 * OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR 
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE 
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <ross.h>

#define UP(t)		((t)->up)
#define UPUP(t)		((t)->up->up)
#define LEFT(t)		((t)->next)
#define RIGHT(t)	((t)->prev)
#define KEY(t)          ((t)->recv_ts)
#define KPKEY(t) (((tw_event *)(t)->pq->least)->recv_ts)

struct tw_pq {
	tw_kp *root;
	tw_kp *least;
	unsigned int nitems;
	unsigned int max_size;
};

struct tw_eventpq
{
	tw_event       *root;
	tw_event       *least;
	unsigned int    nitems;
	unsigned int    max_size;
};
typedef tw_eventpq splay_tree;

#define ROTATE_R(n,p,g) \
  if((LEFT(p) = RIGHT(n))) UP(RIGHT(n)) = p;  RIGHT(n) = p; \
  UP(n) = g;  UP(p) = n;

#define ROTATE_L(n,p,g) \
  if((RIGHT(p) = LEFT(n))) UP(LEFT(n)) = p;  LEFT(n) = p; \
  UP(n) = g;  UP(p) = n;

tw_eventpq *
tw_eventpq_create(void)
{
    splay_tree *st = (splay_tree *) tw_calloc(TW_LOC, "splay tree queue", sizeof(splay_tree), 1);

    st->root = 0;
    st->least = 0;
    st->nitems = 0;

    return st;
}

// KP Version
tw_pq * tw_pq_create (void) {
	tw_pq *st = (tw_pq *) tw_calloc(TW_LOC, "KP splay tree queue", sizeof(tw_pq), 1);

	st->root = 0;
	st->least = 0;
	st->nitems = 0;

	return st;
}

static void
event_splay(tw_event * node)
{
	register tw_event *n = node, *g, *p;
	register tw_event *x, *z;

	for (; (p = UP(n));)
	{
		if (n == LEFT(p))
		{
			if (!((g = UPUP(n))))
			{
				ROTATE_R(n, p, g);
			} else if (p == LEFT(g))
			{
				if ((z = UP(g)))
				{
					if (g == LEFT(z))
						LEFT(z) = n;
					else
						RIGHT(z) = n;
				}
				UP(n) = z;
				if ((x = LEFT(p) = RIGHT(n)))
					UP(x) = p;
				RIGHT(n) = p;
				UP(p) = n;
				if ((x = LEFT(g) = RIGHT(p)))
					UP(x) = g;
				RIGHT(p) = g;
				UP(g) = p;
			} else
			{
				if ((z = UP(g)))
				{
					if (g == LEFT(z))
						LEFT(z) = n;
					else
						RIGHT(z) = n;
				}
				UP(n) = z;
				if ((x = LEFT(p) = RIGHT(n)))
					RIGHT(n) = UP(x) = p;
				else
					RIGHT(n) = p;
				if ((x = RIGHT(g) = LEFT(n)))
					LEFT(n) = UP(x) = g;
				else
					LEFT(n) = g;
				UP(g) = UP(p) = n;
			}
		} else
		{
			if (!((g = UPUP(n))))
			{
				ROTATE_L(n, p, g);
			} else if (p == RIGHT(g))
			{
				if ((z = UP(g)))
				{
					if (g == RIGHT(z))
						RIGHT(z) = n;
					else
						LEFT(z) = n;
				}
				UP(n) = z;
				if ((x = RIGHT(p) = LEFT(n)))
					UP(x) = p;
				LEFT(n) = p;
				UP(p) = n;
				if ((x = RIGHT(g) = LEFT(p)))
					UP(x) = g;
				LEFT(p) = g;
				UP(g) = p;
			} else
			{
				if ((z = UP(g)))
				{
					if (g == RIGHT(z))
						RIGHT(z) = n;
					else
						LEFT(z) = n;
				}
				UP(n) = z;
				if ((x = RIGHT(p) = LEFT(n)))
					LEFT(n) = UP(x) = p;
				else
					LEFT(n) = p;
				if ((x = LEFT(g) = RIGHT(n)))
					RIGHT(n) = UP(x) = g;
				else
					RIGHT(n) = g;
				UP(g) = UP(p) = n;
			}
		}
	}
}

//KP Version 
static void kp_splay (tw_kp *node) {
	register tw_kp *n = node, *g, *p;
	register tw_kp *x, *z;

	for (; (p = UP(n));)
	{
		if (n == LEFT(p))
		{
			if (!((g = UPUP(n))))
			{
				ROTATE_R(n, p, g);
			} else if (p == LEFT(g))
			{
				if ((z = UP(g)))
				{
					if (g == LEFT(z))
						LEFT(z) = n;
					else
						RIGHT(z) = n;
				}
				UP(n) = z;
				if ((x = LEFT(p) = RIGHT(n)))
					UP(x) = p;
				RIGHT(n) = p;
				UP(p) = n;
				if ((x = LEFT(g) = RIGHT(p)))
					UP(x) = g;
				RIGHT(p) = g;
				UP(g) = p;
			} else
			{
				if ((z = UP(g)))
				{
					if (g == LEFT(z))
						LEFT(z) = n;
					else
						RIGHT(z) = n;
				}
				UP(n) = z;
				if ((x = LEFT(p) = RIGHT(n)))
					RIGHT(n) = UP(x) = p;
				else
					RIGHT(n) = p;
				if ((x = RIGHT(g) = LEFT(n)))
					LEFT(n) = UP(x) = g;
				else
					LEFT(n) = g;
				UP(g) = UP(p) = n;
			}
		} else
		{
			if (!((g = UPUP(n))))
			{
				ROTATE_L(n, p, g);
			} else if (p == RIGHT(g))
			{
				if ((z = UP(g)))
				{
					if (g == RIGHT(z))
						RIGHT(z) = n;
					else
						LEFT(z) = n;
				}
				UP(n) = z;
				if ((x = RIGHT(p) = LEFT(n)))
					UP(x) = p;
				LEFT(n) = p;
				UP(p) = n;
				if ((x = RIGHT(g) = LEFT(p)))
					UP(x) = g;
				LEFT(p) = g;
				UP(g) = p;
			} else
			{
				if ((z = UP(g)))
				{
					if (g == RIGHT(z))
						RIGHT(z) = n;
					else
						LEFT(z) = n;
				}
				UP(n) = z;
				if ((x = RIGHT(p) = LEFT(n)))
					LEFT(n) = UP(x) = p;
				else
					LEFT(n) = p;
				if ((x = LEFT(g) = RIGHT(n)))
					RIGHT(n) = UP(x) = g;
				else
					RIGHT(n) = g;
				UP(g) = UP(p) = n;
			}
		}
	}
}

void
tw_eventpq_enqueue(splay_tree *st, tw_event * e)
{
	tw_event       *n = st->root;

	st->nitems++;
	if (st->nitems > st->max_size)
		st->max_size = st->nitems;

	e->state.owner = TW_pe_pq;

	RIGHT(e) = LEFT(e) = 0;
	if (n)
	{
		for (;;)
		{
			if (KEY(n) <= KEY(e))
			{
				if (RIGHT(n))
					n = RIGHT(n);
				else
				{
					RIGHT(n) = e;
					UP(e) = n;
					break;
				}
			} else
			{
				if (LEFT(n))
					n = LEFT(n);
				else
				{
					if (st->least == n)
						st->least = e;
					LEFT(n) = e;
					UP(e) = n;
					break;
				}
			}
		}
		event_splay(e);
		st->root = e;
	} else
	{
		st->root = st->least = e;
		UP(e) = 0;
	}
}

// KP Version
void tw_kp_pq_enqueue (tw_pq *st, tw_kp *e) {
	tw_kp *n = st->root;

	st->nitems++;
	if (st->nitems > st->max_size) {
		st->max_size = st->nitems;
	}

	// e->state.owner = TW_pe_pq; //TODO: needed for kp??

	RIGHT(e) = LEFT(e) = 0;
	if (n)
	{
		for (;;)
		{
			if (KPKEY(n) <= KPKEY(e))
			{
				if (RIGHT(n))
					n = RIGHT(n);
				else
				{
					RIGHT(n) = e;
					UP(e) = n;
					break;
				}
			} else
			{
				if (LEFT(n))
					n = LEFT(n);
				else
				{
					if (st->least == n)
						st->least = e;
					LEFT(n) = e;
					UP(e) = n;
					break;
				}
			}
		}
		kp_splay(e);
		st->root = e;
	} else
	{
		st->root = st->least = e;
		UP(e) = 0;
	}

}

tw_event       *
tw_eventpq_dequeue(splay_tree *st)
{
	tw_event       *r = st->least;
	tw_event       *tmp, *p;

	if (st->nitems == 0)
		return (tw_event *) NULL;

	st->nitems--;

	if ((p = UP(st->least)))
	{
		if ((tmp = RIGHT(st->least)))
		{
			LEFT(p) = tmp;
			UP(tmp) = p;
			for (; LEFT(tmp); tmp = LEFT(tmp));
			st->least = tmp;
		} else
		{
			st->least = UP(st->least);
			LEFT(st->least) = 0;
		}
	} else
	{
		if ((st->root = RIGHT(st->least)))
		{
			UP(st->root) = 0;
			for (tmp = st->root; LEFT(tmp); tmp = LEFT(tmp));
			st->least = tmp;
		} else
			st->least = 0;
	}

	LEFT(r) = NULL;
	RIGHT(r) = NULL;
	UP(r) = NULL;
	r->state.owner = 0;

	return r;
}

// KP Version
tw_kp * tw_kp_pq_dequeue (tw_pq *st) {
	tw_kp *r = st->least;
	tw_kp *tmp, *p;

	if (st->nitems == 0) {
		return (tw_kp *) NULL;
	}

	st->nitems--;

	if ((p = UP(st->least)))
	{
		if ((tmp = RIGHT(st->least)))
		{
			LEFT(p) = tmp;
			UP(tmp) = p;
			for (; LEFT(tmp); tmp = LEFT(tmp));
			st->least = tmp;
		} else
		{
			st->least = UP(st->least);
			LEFT(st->least) = 0;
		}
	} else
	{
		if ((st->root = RIGHT(st->least)))
		{
			UP(st->root) = 0;
			for (tmp = st->root; LEFT(tmp); tmp = LEFT(tmp));
			st->least = tmp;
		} else
			st->least = 0;
	}

	LEFT(r) = NULL;
	RIGHT(r) = NULL;
	UP(r) = NULL;
	// r->state.owner = 0; //TODO: needed for kp??

	return r;
}

void
tw_eventpq_delete_any(splay_tree *st, tw_event * r)
{
	tw_event       *n, *p;
	tw_event       *tmp;

	r->state.owner = 0;

	if (r == st->least)
	{
		tw_eventpq_dequeue(st);
		return;
	}

	if (st->nitems == 0)
	{
		tw_error(TW_LOC,
				 "tw_eventpq_delete_any: attempt to delete from empty queue \n");
	}

	st->nitems--;

	if ((n = LEFT(r)))
	{
		if ((tmp = RIGHT(r)))
		{
			UP(n) = 0;
			for (; RIGHT(n); n = RIGHT(n));
			event_splay(n);
			RIGHT(n) = tmp;
			UP(tmp) = n;
		}
		UP(n) = UP(r);
	} else if ((n = RIGHT(r)))
	{
		UP(n) = UP(r);
	}

	if ((p = UP(r)))
	{
		if (r == LEFT(p))
			LEFT(p) = n;
		else
			RIGHT(p) = n;
		if (n)
		{
			event_splay(p);
			st->root = p;
		}
	} else
		st->root = n;

	LEFT(r) = NULL;
	RIGHT(r) = NULL;
	UP(r) = NULL;
}

// KP Version
void tw_kp_pq_delete_any (tw_pq *st, tw_kp *r) {
	tw_kp *n, *p;
	tw_kp *tmp;

	// r->state.owner = 0; //TODO: needed for kp??

	if (r == st->least)
	{
		tw_kp_pq_dequeue(st);
		return;
	}

	if (st->nitems == 0)
	{
		tw_error(TW_LOC, "tw_pq_delete_any: attempt to delete from empty queue \n");
	}

	st->nitems--;

	if ((n = LEFT(r)))
	{
		if ((tmp = RIGHT(r)))
		{
			UP(n) = 0;
			for (; RIGHT(n); n = RIGHT(n));
			kp_splay(n);
			RIGHT(n) = tmp;
			UP(tmp) = n;
		}
		UP(n) = UP(r);
	} else if ((n = RIGHT(r)))
	{
		UP(n) = UP(r);
	}

	if ((p = UP(r)))
	{
		if (r == LEFT(p))
			LEFT(p) = n;
		else
			RIGHT(p) = n;
		if (n)
		{
			kp_splay(p);
			st->root = p;
		}
	} else
		st->root = n;

	LEFT(r) = NULL;
	RIGHT(r) = NULL;
	UP(r) = NULL;
}

double
tw_eventpq_minimum(splay_tree *pq)
{
	return ((pq->least ? pq->least->recv_ts : DBL_MAX));
}

double tw_pq_minimum (tw_pq *pq) {
	if (pq->least) {
		return ((pq->least->pq->least ? pq->least->pq->least->recv_ts : DBL_MAX));
	}
	return DBL_MAX;
}	

unsigned int
tw_eventpq_get_size(splay_tree *st)
{
	return (st->nitems);
}

unsigned int tw_pq_get_size(tw_pq *st) {
	return (st->nitems);
}


unsigned int
tw_eventpq_max_size(splay_tree *pq)
{
	return (pq->max_size);
}

unsigned int tw_pq_max_size(tw_pq *pq) {
	return (pq->max_size);
}

// API Version
void tw_pq_enqueue (tw_pq *st, tw_event *e) {
	tw_kp *kp = e->dest_lp->kp;
	
	// remove kp from pe->pq if it is there
	if (kp->pq->nitems > 0) {
		tw_kp_pq_delete_any(st, kp);
	}

	// enqueue event, then kp
	tw_eventpq_enqueue(kp->pq, e);
	tw_kp_pq_enqueue(st, kp);
}

// API Version
tw_event * tw_pq_dequeue (tw_pq *st) {

	// Base case
	if (st->nitems == 0) {
		return (tw_event *) NULL;
	}

	// find the correct KP to dequeue from
	tw_kp *kp = tw_kp_pq_dequeue(st);

	// dequeue the event
	tw_event *e = tw_eventpq_dequeue(kp->pq);

	// re-enqueue the kp
	if (kp->pq->nitems > 0) {
		tw_kp_pq_enqueue(st, kp);
	}

	return e;
}

// API Version
// NOTE: delete_any assumes that event is in the pq
void tw_pq_delete_any (tw_pq *st, tw_event *r) {
	tw_kp *kp = r->dest_lp->kp;

	tw_kp_pq_delete_any(st, kp);
	tw_eventpq_delete_any(kp->pq, r);
	if (kp->pq->nitems > 0) {
		tw_kp_pq_enqueue(st, kp);
	}
}
