#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "buddy.h"

#define BUDDY_BLOCK_ORDER 5 /**< @brief Minimum block order */

static buddy_list_t *buddy_list_head = 0;
buddy_list_bucket_t *buddy_master = 0;

/**
 * Request a valid buddy_list_t (BLT) from our list of available BLTs
 */
buddy_list_t *buddy_alloc(void)
{
    buddy_list_t *head = buddy_list_head;
    buddy_list_head = head->next_free;

    if (buddy_list_head == NULL) {
        printf("buddy_list_head is invalid!");
        exit(-1);
    }

    head->next_free = NULL;

    return head;
}

/**
 * This function assumes that a block of the specified order exists
 */
int buddy_split(buddy_list_bucket_t *bucket)
{
    assert(bucket->count && "Bucket contains no entries!");

    // Remove an entry from this bucket and adjust the count
    buddy_list_t *blt = bucket->ptr;
    bucket->count--;
    bucket->ptr = bucket->ptr->next_free;
    blt->next_free = NULL;

    // Add two to the lower order bucket
    bucket--;
    bucket->count += 2;

    // Update the BLTs
    blt->size /= 2;
    blt->use = FREE;
    buddy_list_t *new_buddy = buddy_alloc();
    new_buddy->next_free = (void *)blt->next_free + blt->size;
    new_buddy->size = blt->size;
    new_buddy->use = FREE;

    return 0;
}

/**
 * Find the smallest block that will contain size and return it.
 * This may involve breaking up larger blocks
 */
void *request_buddy_block(unsigned size)
{
    void *ret = 0; // Return value

    // We'll prepend a BLT before each allocation so add that now
    size += sizeof(buddy_list_t);
    size = next_power2(size);

    // Find the bucket we need
    buddy_list_bucket_t *blbt = buddy_master;
    while (size > (1 << blbt->order)) {
        printf("%d > %d\n", size, 1 << blbt->order);
        blbt++;
    }

    printf("target: %d-sized block\n", 1 << blbt->order);

    if (blbt->count) {
        buddy_list_t *blt = blbt->ptr;
        // There's one available.  Grab it
        blbt->count--;
        blbt->ptr = blbt->ptr->next_free;
        blt->next_free = NULL;
        ret = blt;
        ret += sizeof(buddy_list_t);
        return ret;
    }

    // If there are none, keep moving up to larger sizes
    while (!blbt->count) {
        blbt++;
    }

    return ret;
}

/**
 * Pass in the power of two e.g., passing 5 will yield 2^5 = 32
 * The smallest order we'll create will be 32 so this would yield one list
 */
buddy_list_bucket_t * create_buddy_table(unsigned int power_of_two)
{
    int i;
    int list_count;
    buddy_list_bucket_t *bsystem;

    if (power_of_two < BUDDY_BLOCK_ORDER) {
        power_of_two = BUDDY_BLOCK_ORDER;
    }

    list_count = power_of_two - BUDDY_BLOCK_ORDER + 1;

    bsystem = calloc(list_count, sizeof(buddy_list_bucket_t));

    for (i = 0; i < list_count; i++) {
        bsystem[i].count = 0;
        bsystem[i].order = i + BUDDY_BLOCK_ORDER;
        bsystem[i].ptr   = NULL;
    }

    // Allocate the memory
    int size = 1 << power_of_two;
    printf("Allocating %d bytes\n", size);
    void *memory = calloc(1, size);
    printf("memory is %p\n", memory);

    // Allocate memory metadata
    // We can guarantee it is divisible by 2^BUDDY_BLOCK_ORDER
    size /= (1 << BUDDY_BLOCK_ORDER);
    buddy_list_head = calloc(size, sizeof(buddy_list_t));

    for (i = 0; i < size - 1; i++) {
        buddy_list_head[i].next_free = &buddy_list_head[i + 1];
        buddy_list_head[i].use = FREE;
    }
    buddy_list_head[i].next_free = NULL;
    buddy_list_head[i].use = FREE;

    // Set up the primordial buddy block (2^power_of_two)
    buddy_list_t *primordial = buddy_alloc();
    primordial->use       = FREE;
    primordial->next_free = memory;
    primordial->size      = (1 << power_of_two);

    bsystem[list_count - 1].count = 1;
    bsystem[list_count - 1].ptr   = primordial;

    return bsystem;
}

/**
 * From http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
 * Finds the next power of 2 or, if v is a power of 2, return that
 */
unsigned int
next_power2(unsigned int v)
{
    // We're not allocating chunks smaller than 2^BUDDY_BLOCK_ORDER bytes
    if (v < (1 << BUDDY_BLOCK_ORDER)) {
        return (1 << BUDDY_BLOCK_ORDER);
    }

    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;

    return v;
}
