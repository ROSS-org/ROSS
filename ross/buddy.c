#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <ross.h>
#include "buddy.h"

/**
 * @file buddy.c
 * @brief Buddy-system memory allocator implementation
 */

#define BUDDY_BLOCK_ORDER 6 /**< @brief Minimum block order */

static void *buddy_base_address = 0;

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

int dump_buddy_table(buddy_list_bucket_t *buddy_master)
{
    buddy_list_t *blt;
    buddy_list_bucket_t *blbt = buddy_master;

    while (1) {
        int counter = 0;

        printf("BLBT %p:\n", blbt);
        printf("  Count: %d\n", blbt->count);
        printf("  Order: %d\n", blbt->order);
        if (blbt->is_valid == VALID)
            printf("  Valid: YES\n");
        else
            printf("  Valid: NO\n");

        printf("    Pointer         Use            Size\n");
        LIST_FOREACH(blt, &blbt->ptr, next_freelist) {
            counter++;
            if (blt->use == FREE)
                printf("    %11p%8s%16d\n", blt, "FREE", blt->size);
            else
                printf("    %11p%8s%16d\n", blt, "USED", blt->size);
            assert(next_power2(blt->size) == (1 << blbt->order));
        }
        printf("\n");
        assert(counter == blbt->count && "Count is incorrect!");

        blbt++;
        if (blbt->is_valid == INVALID)
            break;
    }

    return 1;
}

/**
 * See if we can merge.  If we can, see if we can merge again.
 */
int buddy_try_merge(buddy_list_t *blt)
{
    int merge_count = 0;

    assert(sizeof(unsigned long) >= sizeof(buddy_list_t*));

    while (1) {
        unsigned long pointer_as_long = (unsigned long)blt;
        unsigned int size = blt->size + sizeof(buddy_list_t);
        buddy_list_bucket_t *blbt = g_tw_buddy_master;
        // Find the bucket we need
        while (size > (1 << blbt->order)) {
            blbt++;
        }
        // We need to normalize for the "buddy formula" to work
        pointer_as_long -= (unsigned long)buddy_base_address;
        pointer_as_long ^= size;
        pointer_as_long += (unsigned long)buddy_base_address;

        // Our buddy has to meet some criteria
        buddy_list_t *possible_buddy = (buddy_list_t*)pointer_as_long;
        if (possible_buddy->use == FREE &&
            possible_buddy->size == (size - sizeof(buddy_list_t))) {

            if (merge_count) {
                // If we've already merged at least once, then it's already
                // in its bucket and we must remove it
                blbt->count--;
                LIST_REMOVE(blt, next_freelist);
            }

            assert(blbt->count && "bucket containing buddy has zero elements");
            blbt->count--;
            LIST_REMOVE(possible_buddy, next_freelist);
            blbt++;
            blbt->count++;

            buddy_list_t *smallest_address = (blt < possible_buddy) ? blt : possible_buddy;
            smallest_address->size = 2 * size - sizeof(buddy_list_t);
            smallest_address->use = FREE;
            LIST_INSERT_HEAD(&blbt->ptr, smallest_address, next_freelist);
            memset(smallest_address+1, 0, smallest_address->size);
            blt = smallest_address;
            merge_count++;
        }
        else {
            break;
        }
    }

    return merge_count;
}

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
    buddy_list_bucket_t *blbt = g_tw_buddy_master;
    while (size > (1 << blbt->order)) {
        blbt++;
    }

    if (blt->use != USED) {
        buddy_list_t *iter;
        tw_printf(TW_LOC, "warning: double free buddy");
        // If it's free, it should be in the correct bucket so let's see
        LIST_FOREACH(iter, &blbt->ptr, next_freelist) {
            if (blt == iter) {
                // Found it, great.
                return;
            }
        }
        assert(0 && "buddy with FREE status not in freelist");
    }

    int initial_count = blbt->count;

    // If there are no entries here, we can't have a buddy
    if (blbt->count == 0) {
        blt->size = size - sizeof(buddy_list_t);
        blt->use = FREE;
        blbt->count++;
        LIST_INSERT_HEAD(&blbt->ptr, blt, next_freelist);
        memset(blt+1, 0, blt->size);
        assert(blbt->count == initial_count + 1);
        return;
    }

    if (buddy_try_merge(blt)) {
        assert(initial_count > blbt->count);
        return;
    }

    // Otherwise, just add it to the list
    blt->size = size - sizeof(buddy_list_t);
    blt->use = FREE;
    blbt->count++;
    LIST_INSERT_HEAD(&blbt->ptr, blt, next_freelist);
    memset(blt+1, 0, blt->size);
    assert(blbt->count == initial_count + 1);
}

/**
 * This function assumes that a block of the specified order exists.
 * @param bucket The bucket containing a block we intend to split.
 */
static
void buddy_split(buddy_list_bucket_t *bucket)
{
    assert(bucket->count && "Bucket contains no entries!");

    // Remove an entry from this bucket and adjust the count
    buddy_list_t *blt = LIST_FIRST(&bucket->ptr);
    assert(blt && "LIST_FIRST returned NULL");
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

    // new_blt->next_freelist = NULL;
    new_blt->use = FREE;
    new_blt->size = (1 << bucket->order) - sizeof(buddy_list_t);

    assert(blt != new_blt);

    LIST_INSERT_HEAD(&bucket->ptr, new_blt, next_freelist);
    LIST_INSERT_HEAD(&bucket->ptr, blt, next_freelist);
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
    buddy_list_bucket_t *blbt = g_tw_buddy_master;
    while (size > (1 << blbt->order)) {
        blbt++;
        if (blbt->is_valid == INVALID) {
            // Error: we're out of bound for valid BLBTs
            tw_error(TW_LOC, "Increase buddy_size");
        }
    }

    if (blbt->count == 0) {
        unsigned split_count = 0;

        // If there are none, keep moving up to larger sizes
        while (blbt->count == 0) {
            blbt++;
            if (blbt->is_valid == INVALID) {
                // Error: we're out of bound for valid BLBTs
                tw_error(TW_LOC, "Increase buddy_size");
            }
            split_count++;
        }

        while (split_count--) {
            buddy_split(blbt--);
        }
    }

    if (LIST_EMPTY(&blbt->ptr)) {
        // This is bad -- they should have allocated more memory
        tw_error(TW_LOC, "Increase buddy_size");
    }
    buddy_list_t *blt = LIST_FIRST(&blbt->ptr);
    assert(blt && "LIST_FIRST returned NULL");
    LIST_REMOVE(blt, next_freelist);
    blt->use = USED;
    blbt->count--;
    ret = (char *)blt;
    ret += sizeof(buddy_list_t);
    return ret;
}

/**
 * Pass in the power of two e.g., passing 5 will yield 2^5 = 32.
 * @param power_of_two The largest "order" this table will support.
 */
buddy_list_bucket_t * create_buddy_table(unsigned int power_of_two)
{
    int i;
    int size;
    int list_count;
    // void *memory;
    buddy_list_bucket_t *bsystem;

    // Don't create anything smaller than this
    if (power_of_two < BUDDY_BLOCK_ORDER) {
        power_of_two = BUDDY_BLOCK_ORDER;
    }

    list_count = power_of_two - BUDDY_BLOCK_ORDER + 1;

    bsystem = tw_calloc(TW_LOC, "buddy system", list_count + 1, sizeof(buddy_list_bucket_t));
    if (bsystem == NULL) {
        return NULL;
    }

    for (i = 0; i < list_count; i++) {
        bsystem[i].count = 0;
        bsystem[i].order = i + BUDDY_BLOCK_ORDER;
        bsystem[i].is_valid = VALID;
        LIST_INIT(&(bsystem[i].ptr));
    }
    bsystem[i].is_valid = INVALID;

    // Allocate the memory
    size = 1 << power_of_two;
    buddy_base_address = tw_calloc(TW_LOC, "buddy system", 1, size);
    if (buddy_base_address == NULL) {
        free(bsystem);
        return NULL;
    }

    // Set up the primordial buddy block (2^power_of_two)
    buddy_list_t *primordial = buddy_base_address;
    primordial->use       = FREE;
    primordial->size      = (1 << power_of_two) - sizeof(buddy_list_t);

    bsystem[list_count - 1].count = 1;
    LIST_INSERT_HEAD(&bsystem[list_count - 1].ptr, primordial, next_freelist);

    return bsystem;
}
