#ifndef BUDDY_H
#define BUDDY_H

#include <sys/queue.h>

/**
 * @file buddy.h
 * @brief Buddy-system memory allocator
 */

typedef enum purpose { FREE, USED } purpose_t;

/**
 * Metadata about this particular block
 * (and stored at the beginning of this block).
 * One per allocated block of memory
 */
typedef struct buddy_list
{
    purpose_t use;
    unsigned int size;
    LIST_ENTRY(buddy_list) next_freelist;
} buddy_list_t;

typedef enum valid { VALID, INVALID } valid_t;

/**
 * Bucket of 2^order sized free memory blocks.
 */
typedef struct buddy_list_bucket
{
    LIST_HEAD(buddy_list_head, buddy_list) ptr;
    unsigned int count;
    unsigned int order;
    valid_t is_valid;
} buddy_list_bucket_t;

buddy_list_bucket_t * create_buddy_table(unsigned int power_of_two);
void *buddy_alloc(unsigned size, buddy_list_bucket_t *buddy_master);
void buddy_free(void *ptr, buddy_list_bucket_t *buddy_master);

#endif /* BUDDY_H */
