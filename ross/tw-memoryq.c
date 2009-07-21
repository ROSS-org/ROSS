#include <ross.h>

#define ROSS_DEBUG 0

tw_memoryq	*
tw_memoryq_init()
{
	tw_memoryq	*q;

	q = tw_calloc(TW_LOC, "Memory Queue", sizeof(tw_memoryq), 1);

	if(!q)
		tw_error(TW_LOC, "Error allocating queue!");

	return q;
}

void
tw_memoryq_debug(tw_memoryq * q)
{
	tw_memory	*next;
	tw_memory	*last;

	int		 cnt;

#if !ROSS_DEBUG
	tw_error(TW_LOC, "This function for DEBUGGING ONLY!");
#endif

	cnt = 0;
	next = q->head;
	last = NULL;

	while(next)
	{
		cnt++;

		if(next->data != (void *)(((char *)next) + sizeof(tw_memory)))
			tw_error(TW_LOC, "Bad data pointer!");

		if(next->prev != last)
			tw_error(TW_LOC, "Prev pointer not correct!");

		last = next;

		next = next->next;
	}

	if(q->tail != last)
		tw_error(TW_LOC, "Tail pointer not correct!");

	if(cnt != q->size)
		tw_error(TW_LOC, "Size not correct!");	
}

 void
tw_memoryq_push(tw_memoryq * q, tw_memory * buf)
{
#if ROSS_DEBUG
	tw_memoryq_debug(q);
#endif

	buf->next = q->head;
	buf->prev = NULL;

	if(q->head)
		q->head->prev = buf;

	q->head = buf;

	if(!q->tail)
	{
		buf->next = NULL;
		q->tail = buf;
	}

	q->size++;

#if ROSS_DEBUG
	tw_memoryq_debug(q);
#endif
}

 void
tw_memoryq_push_list(tw_memoryq * q, tw_memory * h, tw_memory * t, int cnt)
{
#if ROSS_DEBUG
	tw_memoryq_debug(q);
#endif

	t->next = q->head;
	h->prev = NULL;

	if(q->head)
	{
		q->head->prev = t;
		t->next = q->head;
	}

	q->head = h;
	q->head->prev = NULL;

	if(!q->tail)
		q->tail = t;

	q->size += cnt;

#if ROSS_DEBUG
	tw_memoryq_debug(q);
#endif
}

 tw_memory *
tw_memoryq_pop(tw_memoryq * q)
{
	tw_memory	*buf;

#if ROSS_DEBUG
	tw_memoryq_debug(q);
#endif

	if(!q->head)
		return NULL;

	buf = q->head;

	if(q->head->next)
	{
		q->head = q->head->next;
		q->head->prev = NULL;
	} else
		q->head = q->tail = NULL;

	q->size--;

	buf->next = buf->prev = NULL;

#if ROSS_DEBUG
	tw_memoryq_debug(q);
#endif

	return buf;
}

 tw_memory *
tw_memoryq_pop_list(tw_memoryq * q)
{
	tw_memory	*b;

#if ROSS_DEBUG
	tw_memoryq_debug(q);
#endif

	q->size = 0;

	b = q->head;
	q->head = q->tail = NULL;

#if ROSS_DEBUG
	tw_memoryq_debug(q);
#endif

	return b;
}

/*
 * The purpose of this function is to be able to remove some
 * part of a list.. could be all of list, from head to some inner
 * buffer, or from some inner buffer to tail.  I only care about the
 * last case.. 
 */
 void
tw_memoryq_splice(tw_memoryq * q, tw_memory * h, tw_memory * t, int cnt)
{
#if ROSS_DEBUG
	tw_memoryq_debug(q);
#endif

	if(h == q->head && t == q->tail)
	{
		q->size = 0;
		q->head = q->tail = NULL;
		return;
	}

	if(h == q->head)
		q->head = t->next;
	else
		h->prev->next = t->next;

	if(t == q->tail)
		q->tail = h->prev;
	else
		t->next->prev = h->prev;

	q->size -= cnt;

#if ROSS_DEBUG
	tw_memoryq_debug(q);
#endif
}

 void
tw_memoryq_delete_any(tw_memoryq * q, tw_memory * buf)
{
	tw_memory	*next;

#if ROSS_DEBUG
	tw_memoryq_debug(q);
#endif

	next = q->head;
	while(next)
	{
		if(buf == next)
			break;

		next = next->next;
	}

	if(buf != next)
	{
		printf("Buffer not in this queue!! \n");
		return;
	}

#if ROSS_DEBUG
	tw_memoryq_debug(q);
#endif

	if(buf->prev != NULL)
		buf->prev->next = buf->next;
	else
		q->head = buf->next;

	if(buf->next != NULL)
		buf->next->prev = buf->prev;
	else
		q->tail = buf->prev;

	buf->next = buf->prev = NULL;

	q->size--;

#if ROSS_DEBUG
	tw_memoryq_debug(q);
#endif
}
