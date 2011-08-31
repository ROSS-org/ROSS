#include <ross.h>

static void     rehash(olsr_hash * hash_t, int pe);
static int find_empty(olsr_tuple ** hash_t, olsr_tuple * event, int hash_size);
static int find_entry(olsr_tuple ** hash_t, olsr_tuple * event, int hash_size, int pe);
static void     insert(olsr_tuple ** hash_t, olsr_tuple * event, int hash_size);
static olsr_tuple **allocate_table(int hash_size);
static int hash_(olsr_tupleid event_id, int hash_size);
static int      next_prime(int ptst);
static int      is_prime(int ptst);
olsr_tuple *hash_search(olsr_tuple ** hash_t, olsr_tuple *evt, int size);

void     hash_print(olsr_hash * h);

static unsigned int ncpu = 1;
unsigned int g_olsr_hash_size = 31;

int
hash_(olsr_tupleid event_id, int hash_size)
{
	return event_id % hash_size;
}

void           *
olsr_hash_create()
{
	olsr_hash        *h;
	unsigned int             pi;

	ncpu = tw_nnodes();
	h = (olsr_hash *) tw_calloc(TW_LOC, "olsr_hash", sizeof(olsr_hash), 1);

	if (!h)
		tw_error(TW_LOC, "Cannot allocate olsr_hash.");

	h->num_stored = (int *) tw_calloc(TW_LOC, "olsr_hash", sizeof(int) * ncpu, 1);
	h->hash_sizes = (unsigned int *) tw_calloc(TW_LOC, "olsr_hash", sizeof(int) * ncpu, 1);
	h->incoming = (olsr_tuple ***) tw_calloc(TW_LOC, "olsr_hash", sizeof(olsr_tuple *)* ncpu, 1);

	if(!is_prime(g_olsr_hash_size))
		g_olsr_hash_size = next_prime(g_olsr_hash_size);

	for (pi = 0; pi < ncpu; pi++)
	{
		h->num_stored[pi] = 0;
		h->hash_sizes[pi] = g_olsr_hash_size;
		h->incoming[pi] = allocate_table(h->hash_sizes[pi]);
	}

	return (void *) h;
}

void
olsr_hash_insert(void *h, olsr_tuple * event, int pe)
{
	olsr_hash        *hash_t;

	hash_t = (olsr_hash *) h;

	insert(hash_t->incoming[pe], event, hash_t->hash_sizes[pe]);

	(hash_t->num_stored[pe])++;
	if (hash_t->num_stored[pe] > floor(hash_t->hash_sizes[pe] * MAX_FRACTION))
	{
		rehash(hash_t, pe);
	}
}

void
insert(olsr_tuple ** hash_t, olsr_tuple * event, int hash_size)
{
	int             key = 0;

	key = find_empty(hash_t, event, hash_size);
	hash_t[key] = event;
}

void
rehash(olsr_hash * hash_t, int pe)
{
	int             old_size;
	int             old_stored;
	int             i;
	olsr_tuple      **old_list;

	old_stored = hash_t->num_stored[pe];
	old_list = hash_t->incoming[pe];
	old_size = hash_t->hash_sizes[pe];

	hash_t->num_stored[pe] = 0;
	hash_t->hash_sizes[pe] = next_prime(hash_t->hash_sizes[pe]);
	hash_t->incoming[pe] = allocate_table(hash_t->hash_sizes[pe]);

	for (i = 0; i < old_size; i++)
	{
		if (old_list[i] != NULL)
		{
			insert(hash_t->incoming[pe], old_list[i], hash_t->hash_sizes[pe]);
			(hash_t->num_stored[pe])++;
		}
	}

	if(old_stored != hash_t->num_stored[pe])
		tw_error(TW_LOC, "Did not rehash properly!");

#if VERIFY_HASH_QUAD
	printf("\nHASH TABLE RESIZED: old size = %d, new size = %d \n\n", old_size,
		   hash_t->hash_sizes[pe]);
#endif
}

int
find_empty(olsr_tuple ** hash_t, olsr_tuple * event, int hash_size)
{
	unsigned int    i;
	int key;

	i = 0;
	key = hash_(event->event_id, hash_size);

	if(0 > key)
		tw_error(TW_LOC, "here!");

	while (hash_t[key])
	{
		key += 2 * (++i) - 1;
		if (key >= hash_size)
			key -= hash_size;
	}

	return key;
}

int
find_entry(olsr_tuple ** hash_t, olsr_tuple * event, int hash_size, int pe)
{
	unsigned int    i;
	int key;

	i = 0;
	key = hash_(event->event_id, hash_size);

	while (hash_t[key] == NULL || event->event_id != hash_t[key]->event_id)
	{
		key += 2 * (++i) - 1;
		if (key >= hash_size)
			key -= hash_size;

		if (key > hash_size)
		{
			tw_error(TW_LOC, "Cannot find event in hash table: PE %d, key %d, size %d\n", 
				pe, key, hash_size);
		}
	}

	return key;
}

olsr_tuple      **
allocate_table(int hash_size)
{
	return (olsr_tuple **) tw_calloc(TW_LOC, "olsr_hash", sizeof(olsr_tuple *) * hash_size, 1);
}

olsr_tuple       *
olsr_hash_remove(void *h, olsr_tuple * event, int pe)
{
	olsr_hash        *hash_t = (olsr_hash *) h;
	olsr_tuple       *ret_event;
	int             key;

	if(pe > tw_nnodes() - 1)
		tw_error(TW_LOC, "bad pe id");

	key = find_entry(hash_t->incoming[pe], event, hash_t->hash_sizes[pe], pe);
	ret_event = hash_t->incoming[pe][key];

	hash_t->incoming[pe][key] = NULL;
	(hash_t->num_stored[pe])--;
	
	return ret_event;
}

int
next_prime(int ptst)
{

	ptst = ptst * 2 + 1;

	if (is_prime(ptst))
	{
		// printf("%d is prime.\n", ptst);
		return ptst;
	}
	// printf("Searching forward for next prime... ");
	while (!is_prime(ptst))
		ptst += 2;

	// printf("found %d.\n",ptst);

	return ptst;
}

int
is_prime(int ptst)
{
	long            pmaxseek, a;
	int             prim_found;

	if (ptst % 2 == 0)
		return 0;

	prim_found = 1;
	pmaxseek = (long)sqrt((double)ptst) + 1;

	for (a = 3; a <= pmaxseek; a++, a++)
	{
		if (!(ptst % a))
		{
			prim_found = 0;
			break;
		}
	}

	return prim_found;
}

olsr_tuple *
hash_search(olsr_tuple ** hash_t, olsr_tuple *evt, int size)
{
	int             j, empty;
	olsr_tuple       *e;

	for (empty = 0, j = 0; j < size; j++)
	{
		e = hash_t[j];

		if (e && (e->event_id == evt->event_id))
		{
			printf("Found event in hash: %d\n", j);
			return e;
		} else
			empty++;
	}

	printf("%d: HASH has %d empty cells. \n", g_tw_mynode, empty);

	return NULL;
}

void
hash_print(olsr_hash * h)
{
	int             i, j, empty;
	unsigned int   *sizes = h->hash_sizes;
	int            *stored = h->num_stored;
	olsr_tuple      **hash_t;
	olsr_tuple       *e;

	for (i = 0; i < ncpu; i++)
	{
		printf("PE %d: \n", i);
		printf("table size: %d \n", sizes[i]);
		printf("num_stored: %d \n\n", stored[i]);

		hash_t = h->incoming[i];

		for (empty = 0, j = 0; j < sizes[i]; j++)
		{
			e = hash_t[j];

			if (e)
			{
				//printf("recv_ts = %f \n", e->recv_ts);
				//printf("%d: %ld \n\n", j, e->event_id);
			} else
				empty++;
		}
		printf("PE %d has %d empty cells. \n", i, empty);
	}
}
