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

struct tw_pq
{
	tw_event       *root;
	tw_event       *least;
	unsigned int    nitems;
	unsigned int    max_size;
};
typedef tw_pq splay_tree;

#define ROTATE_R(n,p,g) \
  if((LEFT(p) = RIGHT(n))) UP(RIGHT(n)) = p;  RIGHT(n) = p; \
  UP(n) = g;  UP(p) = n;

#define ROTATE_L(n,p,g) \
  if((RIGHT(p) = LEFT(n))) UP(LEFT(n)) = p;  LEFT(n) = p; \
  UP(n) = g;  UP(p) = n;

tw_pq *
tw_pq_create(void)
{
    splay_tree *st = (splay_tree *) tw_calloc(TW_LOC, "splay tree queue", sizeof(splay_tree), 1);

    st->root = 0;
    st->least = 0;
    st->nitems = 0;

    return st;
}

static void
splay(tw_event * node)
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

void
tw_pq_enqueue(splay_tree *st, tw_event * e)
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
		splay(e);
		st->root = e;
	} else
	{
		st->root = st->least = e;
		UP(e) = 0;
	}
}

tw_event       *
tw_pq_dequeue(splay_tree *st)
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

void
tw_pq_delete_any(splay_tree *st, tw_event * r)
{
	tw_event       *n, *p;
	tw_event       *tmp;

	r->state.owner = 0;

	if (r == st->least)
	{
		tw_pq_dequeue(st);
		return;
	}

	if (st->nitems == 0)
	{
		tw_error(TW_LOC,
				 "tw_pq_delete_any: attempt to delete from empty queue \n");
	}

	st->nitems--;

	if ((n = LEFT(r)))
	{
		if ((tmp = RIGHT(r)))
		{
			UP(n) = 0;
			for (; RIGHT(n); n = RIGHT(n));
			splay(n);
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
			splay(p);
			st->root = p;
		}
	} else
		st->root = n;

	LEFT(r) = NULL;
	RIGHT(r) = NULL;
	UP(r) = NULL;
}

double
tw_pq_minimum(splay_tree *pq)
{
	return ((pq->least ? pq->least->recv_ts : DBL_MAX));
}

unsigned int
tw_pq_get_size(splay_tree *st)
{
	return (st->nitems);
}

unsigned int
tw_pq_max_size(splay_tree *pq)
{
	return (pq->max_size);
}
