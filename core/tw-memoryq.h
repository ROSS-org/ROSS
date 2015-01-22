#ifndef INC_tw_memoryq_h
#define INC_tw_memoryq_h

static inline tw_memoryq * 
tw_memoryq_init()
{
	tw_memoryq	*q;

	q = tw_calloc(TW_LOC, "Memory Queue", sizeof(tw_memoryq), 1);

	if(!q)
		tw_error(TW_LOC, "Error allocating queue!");

	q->size = -1;

	return q;
}

static inline void 
tw_memoryq_debug(tw_memoryq * q)
{
#if ROSS_DEBUG
	tw_memory	*next;
	tw_memory	*last;

	int		 cnt;

	cnt = 0;
	next = q->head;
	last = NULL;

	while(next)
	{
		cnt++;

		if(next->prev != last)
			tw_error(TW_LOC, "Prev pointer not correct!");

		last = next;

		next = next->next;
	}

	if(q->tail != last)
		tw_error(TW_LOC, "Tail pointer not correct!");

	if(cnt != q->size)
		tw_error(TW_LOC, "Size not correct!");	
#endif
}

static inline void 
tw_memoryq_push(tw_memoryq * q, tw_memory * buf)
{
	tw_memoryq_debug(q);

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

	tw_memoryq_debug(q);
}

static inline void 
tw_memoryq_push_list(tw_memoryq * q, tw_memory * h, tw_memory * t, int cnt)
{
	tw_memoryq_debug(q);

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

	tw_memoryq_debug(q);
}

static inline tw_memory * 
tw_memoryq_pop(tw_memoryq * q)
{
	tw_memory	*buf;

	tw_memoryq_debug(q);

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

	tw_memoryq_debug(q);

	return buf;
}

static inline tw_memory * 
tw_memoryq_pop_list(tw_memoryq * q)
{
	tw_memory	*b;

	tw_memoryq_debug(q);

	q->size = 0;

	b = q->head;
	q->head = q->tail = NULL;

	tw_memoryq_debug(q);

	return b;
}

/*
 * The purpose of this function is to be able to remove some
 * part of a list.. could be all of list, from head to some inner
 * buffer, or from some inner buffer to tail.  I only care about the
 * last case.. 
 */
static inline void 
tw_memoryq_splice(tw_memoryq * q, tw_memory * h, tw_memory * t, int cnt)
{
	tw_memoryq_debug(q);

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

	tw_memoryq_debug(q);
}

static inline void 
tw_memoryq_delete_any(tw_memoryq * q, tw_memory * buf)
{
	tw_memory	*next;

	tw_memoryq_debug(q);

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

	tw_memoryq_debug(q);

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

	tw_memoryq_debug(q);
}

#endif
