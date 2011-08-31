#ifndef INC_hash_quadratic_h
#define INC_hash_quadratic_h

#define MAX_FRACTION 0.50

extern unsigned int	 g_olsr_hash_size;

typedef struct olsr_hash olsr_hash;

struct olsr_hash
{
	olsr_tuple     ***incoming;
	int            *num_stored;
	unsigned int   *hash_sizes;
};


/*
 * hash-quadratic.c
 */
extern void*     olsr_hash_create();
extern void      olsr_hash_insert(void *h, olsr_tuple * event, int pe);
extern olsr_tuple* olsr_hash_remove(void *h, olsr_tuple * event, int pe);

#endif
