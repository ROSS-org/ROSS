#ifndef INC_ross_base_h
#define INC_ross_base_h

#include <stdint.h>
#include <stddef.h>
#include "config.h"

#if !defined(DBL_MAX)
#include <float.h>
#endif

#ifdef __GNUC__
#  define NORETURN __attribute__((__noreturn__))
#else
#  define NORETURN
#  ifndef __attribute__
#    define __attribute__(x)
#  endif
#endif

#ifdef ROSS_INTERNAL
#undef malloc
#undef calloc
#undef realloc
#undef strdup
#undef free

#  define malloc(a) must_use_tw_calloc_not_malloc
#  define calloc(a,b) must_use_tw_calloc_not_calloc
#  define realloc(a,b) must_use_tw_calloc_not_realloc
#  define strdup(b) must_use_tw_calloc_not_strdup
#  define free(b) must_not_use_free
#endif

/* tw_peid -- Processing Element "PE" id */
typedef unsigned long tw_peid;

/* tw_stime -- Simulation time value for sim clock (NOT wall!) */
typedef double tw_stime;
#define MPI_TYPE_TW_STIME   MPI_DOUBLE
#define TW_STIME_CRT(x)     (x)
#define TW_STIME_DBL(x)     (x)
#define TW_STIME_CMP(x, y)  (((x) < (y)) ? -1 : ((x) > (y)))
#define TW_STIME_ADD(x, y)  ((x) + (y))
#define TW_STIME_MAX        DBL_MAX

/* tw_lpid -- Logical Process "LP" id */
//typedef unsigned long long tw_lpid;
typedef uint64_t tw_lpid;

#endif /* INC_ross_base_h */
