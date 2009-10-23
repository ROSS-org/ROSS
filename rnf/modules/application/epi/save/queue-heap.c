/*
 * ROSS: Rensselaer's Optimistic Simulation System.
 * Copyright (c) 1999-2003 Rensselaer Polytechnic Instutitute.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *
 *      This product includes software developed by David Bauer,
 *      Dr. Christopher D.  Carothers, and Shawn Pearce of the
 *      Department of Computer Science at Rensselaer Polytechnic
 *      Institute.
 *
 * 4. Neither the name of the University nor of the developers may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * 5. The use or inclusion of this software or its documentation in
 *    any commercial product or distribution of this software to any
 *    other party without specific, written prior permission is
 *    prohibited.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS
 * IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *
 */

/**********************************************************************
 * Additional Contributions and Acknowledgements
 *   Kalyan Perumalla - Ga Tech
 *
 *   This implementation is an adaption of the implementation done
 *   by Kalyan for Ga Tech Time Warp
 **********************************************************************/

#include <epi.h>

typedef double KEY_TYPE;
#define KEY(e) (e->ts_next)

FWD(struct, CHeap);

DEF(struct, CHeap)
{
  long nelems;
  int curr_max;
  epi_agent **elems; /* Array [0..curr_max] of epi_agent * */
};

#define SWAP(heap,x,y,t) { \
    t = heap->elems[x]; \
    heap->elems[x] = heap->elems[y]; \
    heap->elems[y] = t; \
    heap->elems[x]->heap_index = x; \
    heap->elems[y]->heap_index = y; \
    }

/*---------------------------------------------------------------------------*/
inline epi_agent * HeapPeekTop( CHeap *h )
{
  return (h->nelems <= 0) ? 0 : h->elems[0];
}

/*---------------------------------------------------------------------------*/
void sift_down( CHeap *h, int i )
{
  int n = h->nelems, k = i, j, c1, c2;
  epi_agent * temp;

  if( n <= 1 ) return;

  /* Stops when neither child is "strictly less than" parent */
  do{
    j = k;
    c1 = c2 = 2*k+1;
    c2++;
    if( c1 < n && KEY(h->elems[c1]) < KEY(h->elems[k]) ) k = c1;
    if( c2 < n && KEY(h->elems[c2]) < KEY(h->elems[k]) ) k = c2;
    SWAP( h, j, k, temp );
  }while( j != k );
}

/*---------------------------------------------------------------------------*/
void percolate_up( CHeap *h, int i )
{
  int n = h->nelems, k = i, j, p;
  epi_agent * temp;

  if( n <= 1 ) return;

  /* Stops when parent is "less than or equal to" child */
  do
    {
      j = k;
      if( (p = (k+1)/2) )
	{
	  --p;
	  if( KEY(h->elems[k]) < KEY(h->elems[p]) ) k = p;
	}
      SWAP( h, j, k, temp );
    }while( j != k );
}

/*---------------------------------------------------------------------------*/
void pq_enqueue(  void *pq, epi_agent * e )
{
  CHeap *h = (CHeap *)pq;

  if( h->nelems >= h->curr_max )
    {
      h->elems = (epi_agent **)
	realloc(h->elems, sizeof(epi_agent *)*(h->curr_max + 50));
      if( !h->elems ) {fprintf(stderr,"Can't expand heap!\n"); exit(1);}
      //else {fprintf(stderr,"Expanded heap to %d!\n", h->curr_max + 50);}
      h->curr_max += 50;
    }

  e->heap_index = h->nelems;
  h->elems[h->nelems++] = e;
  percolate_up( h, h->nelems-1 );

  e->next = NULL;
  e->prev = NULL;
}

/*---------------------------------------------------------------------------*/
void *
pq_dequeue( void *pq )
{
  CHeap *h = (CHeap *)pq;

  if( h->nelems <= 0 )
    return 0;
  else
    {
      epi_agent * e = h->elems[0];
      h->elems[0] = h->elems[--h->nelems];
      h->elems[0]->heap_index = 0;
      sift_down( h, 0 );
      return e;
    }
}

/*---------------------------------------------------------------------------*/
void DumpBucket( void *pq, FILE *fp )
{
  int i;
  CHeap *h = (CHeap *)(pq);
  fprintf( fp, "[ " );
  for( i = 0; i < h->nelems; i++ )
    {
      fprintf( fp, "%s", ( i && i % 10 == 0 ) ? "\n\t" : "" );
      fprintf( fp, "%s%lf", (i ? ", ":""), KEY(h->elems[i]) );
    }
  fprintf( fp, " ]\n" );
  fflush( fp );
}
  
/*---------------------------------------------------------------------------*/
void pq_delete_any(void *pq, epi_agent ** q)
{
  CHeap *h = (CHeap *)(pq);
  epi_agent * victim = *q;
  int i = victim->heap_index;

  if( !(0 <= i && i < h->nelems) || (h->elems[i]->heap_index != i) )
    {
      fprintf( stderr, "Fatal: Bad node in FEL!\n" ); exit(2);
    }
  else
    {

      h->nelems--;
      if( h->nelems > 0 )
	{
	  epi_agent * successor = h->elems[h->nelems];
	  h->elems[i] = successor;
	  successor->heap_index = i;
	  if( KEY(successor) <= KEY(victim) ) percolate_up( h, i );
	  else sift_down( h, i );
	}
    }
}

/*---------------------------------------------------------------------------*/
void * pq_create(void)
{
  CHeap *h = (CHeap *)malloc(sizeof(CHeap));
  h->nelems = 0;
  h->curr_max = (10);
  h->elems = (epi_agent **)malloc(sizeof(epi_agent *)*h->curr_max);
  if( !h->elems ) { fprintf( stderr, "Failed to create heap\n" ); exit(1); }

/*  printf( "Initialized Heap. Allocated %ld bytes\n", sizeof(epi_agent *)*h->curr_max ); */
  fflush(stdout);

  return (void *)h;
}

/*---------------------------------------------------------------------------*/
tw_stime pq_minimum(void *pq)
{
  epi_agent * e = HeapPeekTop( (CHeap*)(pq) );
  double retval = e ? KEY(e) : HUGE_VAL;
  return retval;
}

/*---------------------------------------------------------------------------*/
void *
pq_top(void *pq)
{
  return HeapPeekTop( (CHeap*)(pq) );
}

/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
void *
pq_next(void * h, int next)
{
	return ((CHeap *) h)->elems[next];
}

unsigned int pq_get_size( void *pq )
{
  return ( ((CHeap *)pq)->nelems );
}
