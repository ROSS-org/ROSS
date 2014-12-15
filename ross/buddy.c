#include "buddy.h"
#include <stdlib.h>

#define BUDDY_BLOCK_ORDER 5 /**< @brief Minimum block order */

/**
 * Pass in the power of two e.g., passing 5 will yield 2^5 = 32
 * The smallest order we'll create will be 32 so this would yield one list
 */
buddy_list_t * create_buddy_table(unsigned int power_of_two)
{
    int i;
    int list_count;
    buddy_list_t *bsystem;
    
    if (power_of_two < BUDDY_BLOCK_ORDER) {
        power_of_two = BUDDY_BLOCK_ORDER;
    }
    
    list_count = power_of_two - 5 + 1;
    
    bsystem = calloc(list_count, sizeof(buddy_list_t));
    
    for (i = 0; i < list_count; i++) {
        bsystem[i].order = i + BUDDY_BLOCK_ORDER;
    }
    
    return bsystem;
}

/**
 * From http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
 * Finds the next power of 2 or, if v is a power of 2, return that
 */
unsigned int
next_power2(unsigned int v)
{
    // We're not allocating chunks smaller than BUDDY_MIN_SZ bytes
    if (v < (2 << BUDDY_BLOCK_ORDER)) {
        return (2 << BUDDY_BLOCK_ORDER);
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
