/**********************************************************************
 * Additional Contributions and Acknowledgements
 *   Kalyan Perumalla - Ga Tech
 *
 *   This implementation is an adaption of the implementation done
 *   by Kalyan for Ga Tech Time Warp
 **********************************************************************/

#include <ross.h>

typedef tw_event *ELEMENT_TYPE;
typedef double KEY_TYPE;
#define KEY(e) (e->recv_ts)

struct tw_pq
{
  unsigned long nelems;
  unsigned long curr_max;
  ELEMENT_TYPE *elems; /* Array [0..curr_max] of ELEMENT_TYPE */
};

#define SWAP(heap,x,y,t) { \
    t = heap->elems[x]; \
    heap->elems[x] = heap->elems[y]; \
    heap->elems[y] = t; \
    heap->elems[x]->heap_index = x; \
    heap->elems[y]->heap_index = y; \
    }

/*---------------------------------------------------------------------------*/
void*
tw_unsafe_realloc(
  const char *file,
  int line,
  const char *for_who,
  void *addr,
  size_t len)
{
  malloc_calls++;
  total_allocated += len;
  addr = realloc(addr, len);
  if (!addr)
    tw_error(
      file, line,
      "Cannot allocate %lu bytes for %s",
      (unsigned long)len,
      for_who);
  return addr;
}

/*---------------------------------------------------------------------------*/
static inline ELEMENT_TYPE HeapPeekTop( tw_pq *h )
{
  return (h->nelems <= 0) ? 0 : h->elems[0];
}

/*---------------------------------------------------------------------------*/
static void sift_down( tw_pq *h, int i )
{
  int n = h->nelems, k = i, j, c1, c2;
  ELEMENT_TYPE temp;

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
static void percolate_up( tw_pq *h, int i )
{
  int n = h->nelems, k = i, j, p;
  ELEMENT_TYPE temp;

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
void tw_pq_enqueue(tw_pq *h, ELEMENT_TYPE e )
{
  if( h->nelems >= h->curr_max )
    {
	  const unsigned int i = 50000;
	  const unsigned int u = h->curr_max;
      h->curr_max += i;
      h->elems = tw_unsafe_realloc(
		TW_LOC,
		"heap queue elements",
		h->elems,
		sizeof(*h->elems) * h->curr_max);
	  memset(&h->elems[u], 0, sizeof(*h->elems) * i);
    }

  e->heap_index = h->nelems;
  h->elems[h->nelems++] = e;
  percolate_up( h, h->nelems-1 );

  e->state.owner = TW_pe_pq;
  e->next = NULL;
  e->prev = NULL;
}

/*---------------------------------------------------------------------------*/
ELEMENT_TYPE tw_pq_dequeue(tw_pq *h)
{
  if( h->nelems <= 0 )
    return 0;
  else
    {
      ELEMENT_TYPE e = h->elems[0];
      h->elems[0] = h->elems[--h->nelems];
      h->elems[0]->heap_index = 0;
      sift_down( h, 0 );
      e->state.owner = 0;
      return e;
    }
}

/*---------------------------------------------------------------------------*/
#if 0
static void DumpBucket( void *pq, FILE *fp )
{
  int i;
  tw_pq *h = (tw_pq *)(pq);
  fprintf( fp, "[ " );
  for( i = 0; i < h->nelems; i++ )
    {
      fprintf( fp, "%s", ( i && i % 10 == 0 ) ? "\n\t" : "" );
      fprintf( fp, "%s%lf", (i ? ", ":""), KEY(h->elems[i]) );
    }
  fprintf( fp, " ]\n" );
  fflush( fp );
}
#endif
  
/*---------------------------------------------------------------------------*/
void tw_pq_delete_any(tw_pq *h, tw_event * victim)
{
  int i = victim->heap_index;

  if( !(0 <= i && i < h->nelems) || (h->elems[i]->heap_index != i) )
    {
      fprintf( stderr, "Fatal: Bad node in FEL!\n" ); exit(2);
    }
  else
    {
      h->nelems--;
      victim->state.owner = 0;

      if( h->nelems > 0 )
	{
	  ELEMENT_TYPE successor = h->elems[h->nelems];
	  h->elems[i] = successor;
	  successor->heap_index = i;
	  if( KEY(successor) <= KEY(victim) ) percolate_up( h, i );
	  else sift_down( h, i );
	}
    }
}

/*---------------------------------------------------------------------------*/
tw_pq * tw_pq_create(void)
{
  tw_pq *h = tw_calloc(TW_LOC, "heap queue", sizeof(tw_pq), 1);
  h->nelems = 0;
  h->curr_max = (2*g_tw_events_per_pe);
  h->elems = tw_unsafe_realloc(
	TW_LOC,
	"heap queue elements",
	NULL,
	sizeof(*h->elems) * h->curr_max);
  memset(h->elems, 0, sizeof(*h->elems) * h->curr_max);

  return h;
}

/*---------------------------------------------------------------------------*/
tw_stime tw_pq_minimum(tw_pq *pq)
{
  ELEMENT_TYPE e = HeapPeekTop(pq);
  double retval = e ? KEY(e) : HUGE_VAL;
  return retval;
}

/*---------------------------------------------------------------------------*/
unsigned int tw_pq_get_size( tw_pq *pq )
{
  return ( pq->nelems );
}
