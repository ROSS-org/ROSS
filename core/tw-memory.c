#include <ross.h>

size_t
tw_memory_allocate(tw_memoryq * q)
{
	tw_memory	*head;
	tw_memory	*tail;

	void           *d;
	size_t		mem_sz;
	size_t		mem_len;

	size_t          align;
	size_t		cnt;
	
	align = max(sizeof(double), sizeof(void *));
        mem_len = sizeof(tw_memory) + q->d_size;

        if(mem_len & (align - 1))
                mem_len += align - (mem_len & (align - 1));

	cnt = align = q->start_size + ceil(q->start_size * q->grow);
	mem_sz = mem_len * cnt;

	q->size += cnt;
	q->start_size += ceil(q->start_size * q->grow);

#if ROSS_VERIFY_MEMORY
	printf("Allocating %d bytes in memory subsystem...", mem_sz);
#endif

	d = head = tail = tw_calloc(TW_LOC, "Memory Queue", mem_sz, 1);

	if (!d)
		tw_error(TW_LOC, "\nCannot allocate %u buffers! \n", q->size);
#if ROSS_VERIFY_MEMORY
	else
		printf("Allocated %ld buffers\n", cnt);
#endif

	while (--cnt)
	{
		tail->next = (tw_memory *) (((char *)tail) + mem_len);
		tail->prev = (tw_memory *) (((char *)tail) - mem_len);
		tail = tail->next;
	}

	head->prev = tail->next = NULL;

	if(q->head == NULL)
	{
		q->head = head;
		q->tail = tail;
	} else
	{
		q->tail->next = head;
		head->prev = q->tail;
		q->tail = tail;
	}

#if ROSS_VERIFY_MEMORY
	printf("successful! \n");
#endif

	return align;
}
