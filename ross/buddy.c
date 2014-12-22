#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "buddy.h"

/**
 * @file buddy.c
 * @brief Buddy-system memory allocator implementation
 */

#define BUDDY_BLOCK_ORDER 6 /**< @brief Minimum block order */

buddy_list_bucket_t *buddy_master = 0;

static void *buddy_base_address = 0;

/**
 * Free the given pointer (and coalesce it with its buddy if possible).
 * @param ptr The pointer to free.
 */
void buddy_free(void *ptr)
{
    buddy_list_t *blt = ptr;
    blt--;

    // Now blt is is pointing to the correct address
    unsigned int size = blt->size + sizeof(buddy_list_t);

    // Find the bucket we need
    buddy_list_bucket_t *blbt = buddy_master;
    while (size > (1 << blbt->order)) {
        printf("%d > %d\n", size, 1 << blbt->order);
        blbt++;
    }

    // If there are no entries here, we can't have a buddy
    if (!blbt->count) {
        LIST_INSERT_HEAD(&blbt->ptr, blt, next_freelist);
        memset(blt, 0, size);
        blt->size = size;
        blt->use = FREE;
        return;
    }

    assert(sizeof(unsigned long) >= sizeof(buddy_list_t*));
    unsigned long pointer_as_long = (unsigned long)blt;
    // We need to normalize for the "buddy formula" to work
    pointer_as_long -= (unsigned long)buddy_base_address;
    pointer_as_long ^= size;
    pointer_as_long += (unsigned long)buddy_base_address;
    printf("BLT: %p\tsize: %d\t\tXOR: %lx\n", blt, size, pointer_as_long);

    // Our buddy has to meet some criteria
    buddy_list_t *possible_buddy = (buddy_list_t*)pointer_as_long;
    if (possible_buddy->use == FREE &&
        possible_buddy->size == (size - sizeof(buddy_list_t))) {
        blbt->count--;
        LIST_REMOVE(possible_buddy, next_freelist);
        blbt++;
        blbt->count++;
        buddy_list_t *smallest_address = (blt < possible_buddy) ? blt : possible_buddy;
        printf("smallest_address: %p\tblt: %p\tpossible_buddy: %p\n", smallest_address, blt, possible_buddy);
        LIST_INSERT_HEAD(&blbt->ptr, smallest_address, next_freelist);
        memset(smallest_address, 0, 2 * size);
        smallest_address->size = 2 * size;
        smallest_address->use = FREE;
        return;
    }

    // Otherwise, just add it
    LIST_INSERT_HEAD(&blbt->ptr, blt, next_freelist);
    memset(blt, 0, size);
    blt->size = size;
    blt->use = FREE;
}

/**
 * This function assumes that a block of the specified order exists.
 * @param bucket The bucket containing a block we intend to split.
 */
void buddy_split(buddy_list_bucket_t *bucket)
{
    assert(bucket->count && "Bucket contains no entries!");

    // Remove an entry from this bucket and adjust the count
    buddy_list_t *blt = LIST_FIRST(&bucket->ptr);
    bucket->count--;
    LIST_REMOVE(blt, next_freelist);

    // Add two to the lower order bucket
    bucket--;
    bucket->count += 2;

    // Update the BLTs
    blt->use = FREE;
    blt->size = (1 << bucket->order) - sizeof(buddy_list_t);

    void *address = ((char *)blt) + (1 << bucket->order);
    buddy_list_t *new_blt = address;
    printf("address of new_blt is %p\n", new_blt);

    // new_blt->next_freelist = NULL;
    new_blt->use = FREE;
    new_blt->size = (1 << bucket->order) - sizeof(buddy_list_t);

    LIST_INSERT_HEAD(&bucket->ptr, new_blt, next_freelist);
    LIST_INSERT_HEAD(&bucket->ptr, blt, next_freelist);
}

/**
 * Finds the next power of 2 or, if v is a power of 2, return that.
 * From http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
 * @param v Find a power of 2 >= v.
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

/**
 * Find the smallest block that will contain size and return it.
 * Note this returns the memory allocated and usable, not the entire buffer.
 * This may involve breaking up larger blocks.
 * @param size The size of the data this allocation must be able to hold.
 */
void *buddy_alloc(unsigned size)
{
    char *ret = 0; // Return value

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

    if (blbt->count == 0) {
        unsigned split_count = 0;

        // If there are none, keep moving up to larger sizes
        while (!blbt->count) {
            blbt++;
            split_count++;
        }

        while (split_count--) {
            buddy_split(blbt--);
        }
    }

    if (LIST_EMPTY(&blbt->ptr)) {
        // This is bad -- they should have allocated more memory
        return ret;
    }
    buddy_list_t *blt = LIST_FIRST(&blbt->ptr);
    LIST_REMOVE(blt, next_freelist);
    blt->use = USED;
    blbt->count--;
    ret = (char *)blt;
    ret += sizeof(buddy_list_t);
    return ret;
}

/**
 * Pass in the power of two e.g., passing 5 will yield 2^5 = 32.
 * The smallest order we'll create will be 2^BUDDY_BLOCK_ORDER.
 * @param power_of_two The largest "order" this table will support.
 */
buddy_list_bucket_t * create_buddy_table(unsigned int power_of_two)
{
    int i;
    int size;
    int list_count;
    // void *memory;
    buddy_list_bucket_t *bsystem;

    if (power_of_two < BUDDY_BLOCK_ORDER) {
        power_of_two = BUDDY_BLOCK_ORDER;
    }

    list_count = power_of_two - BUDDY_BLOCK_ORDER + 1;

    bsystem = calloc(list_count, sizeof(buddy_list_bucket_t));

    for (i = 0; i < list_count; i++) {
        bsystem[i].count = 0;
        bsystem[i].order = i + BUDDY_BLOCK_ORDER;
        LIST_INIT(&(bsystem[i].ptr));
    }

    // Allocate the memory
    size = 1 << power_of_two;
    printf("Allocating %d bytes\n", size);
    buddy_base_address = calloc(1, size);
    printf("memory is %p\n", buddy_base_address);

    // Set up the primordial buddy block (2^power_of_two)
    buddy_list_t *primordial = buddy_base_address;
    primordial->use       = FREE;
    primordial->size      = (1 << power_of_two) - sizeof(buddy_list_t);

    bsystem[list_count - 1].count = 1;
    LIST_INSERT_HEAD(&bsystem[list_count - 1].ptr, primordial, next_freelist);

    return bsystem;
}
