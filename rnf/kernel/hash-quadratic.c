#include <rossnet.h>

static int      rn_find_empty(rn_link ** hash_t, rn_link * event, int capacity);
static int      rn_find_entry(rn_link ** hash_t, int, int capacity);
static int      rn_hash_(int, int capacity);
static int      rn_next_prime(int ptst);
static int      rn_is_prime(int ptst);

#define RN_MAX_FRACTION 0.75

struct rn_hash_tag
{
	rn_link		**storage;
	int		  nstored;
	unsigned int	  capacity;
};

int
rn_hash_(int id, int capacity)
{
	return id % capacity;
}

rn_link      **
rn_allocate_table(int capacity)
{
	return (rn_link **) tw_calloc(TW_LOC, "", sizeof(rn_link *) * capacity, 1);
}

void           *
rn_hash_create(unsigned int sz)
{
	rn_hash			*h;
	unsigned int		 size;

	h = (rn_hash *) tw_calloc(TW_LOC, "", sizeof(rn_hash), 1);

	if (!h)
		tw_error(TW_LOC, "Cannot rn_allocate rn_hash.");

	//h->storage = (rn_link **) tw_calloc(TW_LOC, "", sizeof(rn_link *), 1);

	size = sz;
	if(!rn_is_prime(sz) || sz == 1)
		size = rn_next_prime(sz);

	while(sz > floor(size * RN_MAX_FRACTION))
		size = rn_next_prime(size);

	h->nstored = 0;
	h->capacity = size;
	h->storage = rn_allocate_table(h->capacity);

	return (void *)h;
}

void
h_insert(rn_link ** hash_t, rn_link * event, int capacity)
{
	int             key = 0;

	key = rn_find_empty(hash_t, event, capacity);
	hash_t[key] = event;
}

void
rn_hash_insert(void *h, rn_link * event)
{
	rn_hash        *hash_t;

	hash_t = (rn_hash *) h;

	h_insert(hash_t->storage, event, hash_t->capacity);
	hash_t->nstored++;

	if(hash_t->nstored > floor(hash_t->capacity * RN_MAX_FRACTION))
		tw_error(TW_LOC, "Inserted beyond RN_MAX_FRACTION!");
}

int
rn_find_empty(rn_link ** hash_t, rn_link * event, int capacity)
{
	unsigned int    i;
	int             key;

	i = 0;
	key = rn_hash_(event->addr+1, capacity);

	while (hash_t[key])
	{
		key += 2 * (++i) - 1;
		if (key >= capacity)
			key -= capacity;
	}

	return key;
}

int
rn_find_entry(rn_link ** hash_t, int id, int capacity)
{
	unsigned int    i;
	int             key;

	i = 0;
	key = rn_hash_(id+1, capacity);

	while (hash_t[key] == 0 || id != hash_t[key]->addr)
	{
		key += 2 * (++i) - 1;
		if (key >= capacity)
			key -= capacity;

		if (key >= capacity)
			tw_error(TW_LOC, "Cannot find link in hash table.");
	}

	return key;
}

rn_link       *
rn_hash_fetch(void *h, int id)
{
	rn_hash		*hash_t = (rn_hash *) h;
	rn_link		*ret_event;
	int		 key;

	key = rn_find_entry(hash_t->storage, id, hash_t->capacity);

	if(-1 == key)
		return NULL;

	ret_event = hash_t->storage[key];

#if 0
	if (!ret_event || ret_event->recv_ts != event->recv_ts || ret_event->seq_num != event->seq_num)
		tw_error(TW_LOC, "Could not find correct event in hash_table.");
#endif

	return ret_event;
}

rn_link       *
rn_hash_remove(void *h, rn_link * event)
{
	rn_hash        *hash_t = (rn_hash *) h;
	rn_link       *ret_event;
	int             key;

	key = rn_find_entry(hash_t->storage, event->addr, hash_t->capacity);
	ret_event = hash_t->storage[key];

#if 0
	if (!ret_event || ret_event->recv_ts != event->recv_ts || ret_event->seq_num != event->seq_num)
		tw_error(TW_LOC, "Could not find correct event in hash_table.");
#endif

	hash_t->storage[key] = NULL;
	hash_t->nstored--;

	return ret_event;
}

int
rn_next_prime(int ptst)
{

	ptst = ptst * 2 + 1;

	if (rn_is_prime(ptst))
	{
		// printf("%d is prime.\n", ptst);
		return ptst;
	}

	// printf("Searching forward for next prime... ");
	while (!rn_is_prime(ptst))
		ptst += 2;

	// printf("found %d.\n",ptst);

	return ptst;
}

int
rn_is_prime(int ptst)
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
