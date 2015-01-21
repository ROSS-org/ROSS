#ifndef BUDDY_H
#define BUDDY_H

#include <sys/queue.h>

/**
 * @file buddy.h
 * @brief Buddy-system memory allocator
 */

typedef enum purpose { FREE, USED } purpose_t;

#define BUDDY_ALIGN_PREF (32 - 2 * sizeof(void*) - sizeof(uint32_t) - sizeof(purpose_t))

/**
 * Metadata about this particular block
 * (and stored at the beginning of this block).
 * One per allocated block of memory.
 * Should be 32 bytes to not screw up alignment.
 */
typedef struct buddy_list
{
    // Should be two pointers
    LIST_ENTRY(buddy_list) next_freelist;
    uint32_t size;
    purpose_t use;
    char padding[BUDDY_ALIGN_PREF];
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
void *buddy_alloc(unsigned size);
void buddy_free(void *ptr);

#endif /* BUDDY_H */
