#include "buddy.h"

/**
 * From http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
 * Finds the next power of 2 or, if v is a power of 2, return that
 */
unsigned int
next_power2(unsigned int v)
{
    // We're not allocating chunks smaller than 32 bytes
    if (v < 32) {
        return 32;
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
