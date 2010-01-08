#ifndef INC_ross_h
#define INC_ross_h

/*
 * ROSS: Rensselaer's Optimistic Simulation System.
 * Copyright (c) 1999-2003 Rensselaer Polytechnic Instutitute.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *
 *      This product includes software developed by David Bauer,
 *      Dr. Christopher D.  Carothers, and Shawn Pearce of the
 *      Department of Computer Science at Rensselaer Polytechnic
 *      Institute.
 *
 * 4. Neither the name of the University nor of the developers may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * 5. The use or inclusion of this software or its documentation in
 *    any commercial product or distribution of this software to any
 *    other party without specific, written prior permission is
 *    prohibited.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS
 * IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *
 */

#ifndef INLINE
#define	INLINE(x) static inline x
#endif

#ifndef FWD
#define	FWD(t,n) t n##_tag;typedef t n##_tag n
#endif
#ifndef DEF
#define	DEF(t,n) t n##_tag
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) ( sizeof((a)) / sizeof((a)[0]) )
#endif

#ifndef ROSS_THREAD_none
#  define ROSS_byte_per_bit
#endif
#ifdef ROSS_byte_per_bit
#  define BIT_GROUP(grp) grp
#  define BIT_GROUP_ITEM(name, sz) unsigned char name;
#else
#  define BIT_GROUP(grp) unsigned grp __bitgroupend##__LINE__:1;
#  define BIT_GROUP_ITEM(name, sz) name:sz,
#endif

#ifdef __GNUC__
#  define NORETURN __attribute__((__noreturn__))
#else
#  define NORETURN
#  ifndef __attribute__
#    define __attribute__(x)
#  endif
#endif

/*********************************************************************
 *
 * Include ``standard'' headers that most of ROSS will require.
 *
 ********************************************************************/

#include <errno.h>
#include <sys/types.h>
#include <math.h>
#include <limits.h> 
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>

#if !defined(DBL_MAX)
#include <float.h>
#endif

#include <sys/time.h>
#include <time.h>

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
typedef unsigned int tw_peid;

/* tw_stime -- Simulation time value for sim clock (NOT wall!) */
typedef double tw_stime;

/* tw_lpid -- Logical Process "LP" id */
#if defined(ARCH_bgl) || defined(ARCH_bgp)
typedef unsigned long long tw_lpid;
#else
typedef unsigned long long tw_lpid;
//typedef uintptr_t tw_lpid;
#endif

#include "ross-random.h"

#ifdef ROSS_RAND_clcg4
#  include "rand-clcg4.h"
#endif

#ifdef ROSS_CLOCK_i386
#  include "clock-i386.h"
#endif
#ifdef ROSS_CLOCK_amd64
#  include "clock-amd64.h"
#endif
#ifdef ROSS_CLOCK_ia64
#  include "clock-ia64.h"
#endif
#ifdef ROSS_CLOCK_ppc
#  include "clock-ppc.h"
#endif
#ifdef ROSS_CLOCK_bgl
#  include "clock-bgl.h"
#endif
#ifdef ROSS_CLOCK_none
#  include "clock-none.h"
#endif

#ifdef ROSS_NETWORK_none
#  include "network-none1.h"
#endif
#ifdef ROSS_NETWORK_mpi
#  include "network-mpi1.h"
#endif
#ifdef ROSS_NETWORK_tcp
#  include "network-tcp1.h"
#endif

#include "tw-timing.h"
#include "ross-types.h"
#include "tw-timer.h"
#include "tw-opts.h"
#include "ross-network.h"
#include "ross-gvt.h"
#include "ross-extern.h"
#include "ross-kernel-inline.h"
#include "hash-quadratic.h"

#ifdef ROSS_NETWORK_none
#  include "network-none2.h"
#endif
#ifdef ROSS_NETWORK_mpi
#  include "network-mpi2.h"
#endif
#ifdef ROSS_NETWORK_tcp
#  include "network-tcp2.h"
#  include "socket-tcp.h"
#  include "hash-quadratic.h"
#endif

#ifdef ROSS_GVT_none
#  include "gvt-none.h"
#endif
#ifdef ROSS_GVT_7oclock
#  include "gvt-7oclock.h"
#endif
#ifdef ROSS_GVT_mpi_allreduce
#  include "mpi.h"
#  include "gvt-mpi_allreduce.h"
#endif

#ifdef ROSS_MEMORY
#  include "tw-memoryq.h"
#  include "tw-memory.h"
#endif

#include "tw-eventq.h"
#include "ross-inline.h"

#endif

