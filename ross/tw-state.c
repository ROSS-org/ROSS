#include <ross.h>
#include <assert.h>
#include "lz4.h"

/**
 * Make a snapshot of the LP state and store it into the delta buffer
 */
void
tw_snapshot(tw_lp *lp, size_t state_sz)
{
    memcpy(lp->pe->delta_buffer[0], lp->cur_state, state_sz);
}

/**
 * Create the delta from the current state and the snapshot.
 * Compress it.
 * @return The size of the compressed data placed in delta_buffer[1].
 */
long
tw_snapshot_delta(tw_lp *lp, size_t state_sz)
{
    long i;
    int ret_size = 0;
    unsigned char *current_state = lp->cur_state;
    unsigned char *snapshot = lp->pe->delta_buffer[0];

    for (i = 0; i < state_sz; i++) {
        snapshot[i] = current_state[i] - snapshot[i];
    }

    ret_size = LZ4_compress((char*)snapshot, (char*)lp->pe->delta_buffer[1], state_sz);
    if (ret_size < 0) {
        tw_error(TW_LOC, "LZ4_compress error");
    }

    lp->pe->cur_event->delta_buddy = buddy_alloc(ret_size);
    assert(lp->pe->cur_event->delta_buddy);
    lp->pe->cur_event->delta_size = ret_size;
    memcpy(lp->pe->cur_event->delta_buddy, lp->pe->delta_buffer[1], ret_size);

    return ret_size;
}

/**
 * Restore the state of lp to the (decompressed) data held in buffer
 */
void
tw_snapshot_restore(tw_lp *lp, size_t state_sz, void *buffer, size_t delta_size)
{
    int ret = LZ4_decompress_safe(buffer, lp->cur_state, delta_size, state_sz);
    if (ret < 0) {
        tw_error(TW_LOC, "LZ4_decompress_fast error");
    }
}

void
tw_state_save(tw_lp *lp, tw_event *cevent)
{
#if 0
  tw_lp_state	*n;
  tw_lp_state	*o;

  o = (tw_lp_state *) lp->cur_state;
  n = lp->state_qh;
  if (!n)
    {
      tw_state_alloc(lp, g_tw_sv_growcnt);
      n = lp->state_qh;
    }
  lp->state_qh = n->next;

  cevent->lp_state = o;
  lp->cur_state = n;

  if (lp->type.statecp)
    (*lp->type.statecp)(n, o);
  else
    memcpy(n, o, lp->type.state_sz);
#endif
}

void
tw_state_rollback(tw_lp *lp, tw_event *revent)
{
#if 0
  tw_lp_state	*o;

  if (lp->type.revent)
    {
#endif
	lp->pe->cur_event = revent;
	lp->kp->last_time = revent->recv_ts;

	(*lp->type.revent)(
		lp->cur_state,
		&revent->cv,
		tw_event_data(revent),
		lp);

        if (revent->delta_buddy) {
            buddy_free(revent->delta_buddy);
            revent->delta_buddy = 0;
        }
#if 0
    } else
      {
	o = (tw_lp_state *) lp->cur_state;
	o->next = lp->state_qh;
	lp->state_qh = o;

	lp->cur_state = revent->lp_state;
      }
#endif
}

void
tw_state_alloc(tw_lp *lp, int nvect)
{
#if 0
  tw_lp_state	*h;
  tw_lp_state	*c;

  h = tw_calloc(TW_LOC, "state vectors", lp->type.state_sz, nvect);
  c = h;
  while(nvect--)
    {
      if (nvect == 0)
	{
	  c->next = NULL;
	} else
	  {
	    c->next = (tw_lp_state*)( ((char*)c) + lp->type.state_sz );
	    c = c->next;
	  }
    }

  lp->state_qh = h;
#endif
	if(!lp->cur_state)
		lp->cur_state = tw_calloc(TW_LOC, "state vectors", lp->type.state_sz, 1);
}
