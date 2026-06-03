#ifndef INC_test_lp_minimal_h
#define INC_test_lp_minimal_h

#include <ross.h>

/* Minimal no-op LP for subsystem driver tests. Drivers that need any
 * actual behavior register their own tw_lptype; this one is just the
 * smallest registration that lets the engine boot. */

typedef struct test_lp_minimal_state {
    int unused;
} test_lp_minimal_state;

typedef struct test_lp_minimal_message {
    int unused;
} test_lp_minimal_message;

extern tw_lptype test_lp_minimal_type[];

#endif
