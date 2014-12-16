#ifndef BUDDY_H
#define BUDDY_H

typedef enum purpose { FREE, SPLIT, USED } purpose_t;

/**
 * One per allocated block of memory
 */
typedef struct buddy_list
{
    purpose_t use;
    unsigned int size;
    struct buddy_list *next_free;
} buddy_list_t;

/**
 * One bucket of 2^order sized free memory blocks
 */
typedef struct buddy_list_bucket
{
    buddy_list_t *ptr;
    unsigned int count;
    unsigned int order;
} buddy_list_bucket_t;

extern buddy_list_bucket_t *buddy_master;

buddy_list_bucket_t * create_buddy_table(unsigned int power_of_two);
unsigned int next_power2(unsigned int v);
void *request_buddy_block(unsigned size);

#endif /* BUDDY_H */
