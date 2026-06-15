#ifndef INC_test_assert_h
#define INC_test_assert_h

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

/*
 * ROSS_TEST_ASSERT(expr, fmt, ...)
 *
 * Single-line failure messages that interpolate runtime values, so a
 * ctest failure log tells you *what* went wrong without needing to
 * rerun under gdb. The failed expression is captured as a string via
 * #expr so the message and the source-level condition stay in sync.
 *
 * MPI_Abort rather than plain abort() so multi-rank drivers terminate
 * all ranks instead of leaving the non-faulting ones to time out. For
 * single-rank drivers it has the same effect as abort(). Safe to call
 * any time after tw_init (which calls MPI_Init); never call before.
 *
 * fflush before abort because stderr is block-buffered when ctest
 * redirects it, and an aborting process doesn't flush on the way out.
 */
#define ROSS_TEST_ASSERT(expr, ...) do {                                       \
    if (!(expr)) {                                                             \
        fprintf(stderr, "[ROSS_TEST] FAIL %s:%d (%s): ",                       \
            __FILE__, __LINE__, #expr);                                        \
        fprintf(stderr, __VA_ARGS__);                                          \
        fprintf(stderr, "\n");                                                 \
        fflush(stderr);                                                        \
        MPI_Abort(MPI_COMM_WORLD, 1);                                          \
        abort();                                                               \
    }                                                                          \
} while (0)

#endif
