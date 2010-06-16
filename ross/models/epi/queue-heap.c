#include <epi.h>

typedef double KEY_TYPE;
#define KEY(e) (( ((epi_agent *) e)+1)->ts_next)

typedef struct CHeap CHeap;

struct CHeap
{
  long nelems;
  int curr_max;
  tw_memory **elems; /* Array [0..curr_max] of tw_memory * */
};

#define SWAP(heap,x,y,t) { \
    t = heap->elems[x]; \
    heap->elems[x] = heap->elems[y]; \
    heap->elems[y] = t; \
    heap->elems[x]->heap_index = x; \
    heap->elems[y]->heap_index = y; \
    }

/*---------------------------------------------------------------------------*/
inline tw_memory * HeapPeekTop( CHeap *h )
{
  return (h->nelems <= 0) ? 0 : h->elems[0];
}

/*---------------------------------------------------------------------------*/
void sift_down( CHeap *h, int i )
{
  int n = h->nelems, k = i, j, c1, c2;
  tw_memory * temp;

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
  tw_memory * temp;

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
void pq_enqueue(  void *pq, tw_memory * e )
{
  CHeap *h = (CHeap *)pq;

  if( h->nelems >= h->curr_max )
    {
      h->elems = (tw_memory **)
	realloc(h->elems, sizeof(tw_memory *)*(h->curr_max + 50));
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
      tw_memory * e = h->elems[0];
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
void pq_delete_any(void *pq, tw_memory ** q)
{
  CHeap *h = (CHeap *)(pq);
  tw_memory * victim = *q;
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
	  tw_memory * successor = h->elems[h->nelems];
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
  h->elems = (tw_memory **)malloc(sizeof(tw_memory *)*h->curr_max);
  if( !h->elems ) { fprintf( stderr, "Failed to create heap\n" ); exit(1); }

/*  printf( "Initialized Heap. Allocated %ld bytes\n", sizeof(tw_memory *)*h->curr_max ); */
  fflush(stdout);

  return (void *)h;
}

/*---------------------------------------------------------------------------*/
tw_stime pq_minimum(void *pq)
{
  tw_memory * e = HeapPeekTop( (CHeap*)(pq) );
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

void
pq_cleanup(void * h)
{
	tw_memory	*b;
	tw_stime	 last;

	int	 sz = pq_get_size(h);
	int	 i;

	last = pq_minimum(h);

	for(i = 0; i < sz; i++)
	{
		b = pq_next(h, i);

		while(last > KEY(b))
		{
			pq_delete_any(h, &b);
			pq_enqueue(h, b);

			b = pq_next(h, i);
		}
	}
}

/*---------------------------------------------------------------------------*/
void *
pq_next(void * h, int next)
{
	return ((CHeap *) h)->elems[next];
}

unsigned int
pq_get_size( void *pq )
{
  return ( ((CHeap *)pq)->nelems );
}
