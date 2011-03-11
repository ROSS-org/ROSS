#ifndef INC_ross_h
#define INC_ross_h

/** @mainpage Rensselaer's Optimistic Simulation System (ROSS)
    @section intro_sec Introduction
    
    ROSS is an acronym for Rensselaer's Optimistic Simulation System. It is a
    parallel discrete-event simulator that executes on shared-memory
    multiprocessor systems. ROSS is geared for running large-scale simulation
    models (i.e., 100K to even 1 million object models).  The synchronization
    mechanism is based on Time Warp. Time Warp is an optimistic
    synchronization mechanism develop by Jefferson and Sowizral [10, 11] used
    in the parallelization of discrete-event simulation. The distributed
    simulator consists of a collection of logical processes or LPs, each
    modeling a distinct component of the system being modeled, e.g., a server
    in a queuing network. LPs communicate by exchanging timestamped event
    messages, e.g., denoting the arrival of a new job at that server.

    The Time Warp mechanism uses a detection-and-recovery protocol to
    synchronize the computation. Any time an LP determines that it has
    processed events out of timestamp order, it "rolls back" those events, and
    re-executes them. For a detailed discussion of Time Warp as well as other
    parallel simulation protocols we refer the reader to [8]

    ROSS was modeled after a Time Warp simulator called GTW or Georgia Tech
    Time Warp[7]. ROSS helped to demonstrate that Time Warp simulators can be
    run efficiently both in terms of speed and memory usage relative to a
    high-performance sequential simulator.

    To achieve high parallel performance, ROSS uses a technique call Reverse
    Computation. Here, the roll back mechanism in the optimistic simulator is
    realized not by classic state-saving, but by literally allowing to the
    greatest possible extent events to be reverse. Thus, as models are
    developed for parallel execution, both the forward and reverse execution
    code must be written. Currently, both are done by hand. We are
    investigating automatic methods that are able to generate the reverse
    execution code using only the forward execution code as input. For more
    information on ROSS and Reverse Computation we refer the interested reader
    to [4, 5]. Both of these text are provided as additional reading in the
    ROSS distribution.

@section license_sec License
    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
  
    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
  
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.
  
    3. All advertising materials mentioning features or use of this software
       must display the following acknowledgement:
  
        This product includes software developed by Mark Anderson, David Bauer,
        Dr. Christopher D. Carothers, Justin LaPre, and Shawn Pearce of the
        Department of Computer Science at Rensselaer Polytechnic
        Institute.
 
    4. Neither the name of the University nor of the developers may be used
       to endorse or promote products derived from this software without
       specific prior written permission.
  
    5. The use or inclusion of this software or its documentation in
       any commercial product or distribution of this software to any
       other party without specific, written prior permission is
       prohibited.
 
    THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS
    IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
    FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
    REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
    HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
    STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
    ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
    OF THE POSSIBILITY OF SUCH DAMAGE.
 
    Copyright (c) 1999-2010 Rensselaer Polytechnic Institute.
    All rights reserved.
*/

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) ( sizeof((a)) / sizeof((a)[0]) )
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

#include "ross_config.h"

#ifdef HAVE_ERRNO_H
#	include <errno.h>
#endif

#ifdef HAVE_SYS_TYPES_H
#	include <sys/types.h>
#endif

#ifdef HAVE_MATH_H
#	include <math.h>
#endif

#ifdef HAVE_LIMITS_H
#	include <limits.h>
#endif

#ifdef HAVE_STDLIB_H
#	include <stdlib.h>
#endif

#ifdef HAVE_UNISTD_H
#	include <unistd.h>
#endif

#ifdef HAVE_STRING_H
#	include <string.h>
#endif

#ifdef HAVE_STDIO_H
#	include <stdio.h>
#endif

#ifdef HAVE_STDARG_H
#	include <stdarg.h>
#endif

#ifdef HAVE_STDINT_H
#	include <stdint.h>
#endif

#ifdef HAVE_FLOAT_H
#	include <float.h>
#endif

#ifdef HAVE_SYS_TIME_H
#	include <sys/time.h>
#endif

#ifdef HAVE_SYS_TYPES_H
#	include <time.h>
#endif

#ifdef HAVE_BGLPERSONALITY_H
#	include <bglpersonality.h> 
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


typedef uint64_t tw_peid;   	/**< tw_peid -- Processing Element "PE" id */
typedef   double tw_stime;  	/**< tw_stime -- Simulation time value for sim clock (NOT wall!) */
typedef uint64_t tw_clock;
typedef uint64_t tw_lpid;		/**< tw_lpid -- Numerical Value of Logical Processor */
typedef uint32_t tw_eventid;
typedef  int32_t tw_node;

#include "ross-random.h"
#include "rand-clcg4.h"

#include "tw-timing.h"
#include "ross-types.h"
#include "tw-timer.h"
#include "tw-opts.h"
#include "ross-network.h"
#include "ross-gvt.h"
#include "ross-extern.h"
#include "ross-kernel-inline.h"
#include "hash-quadratic.h"

#include "network-mpi.h"

#ifdef ROSS_GVT_7oclock
#  include "gvt-7oclock.h"
#endif
#ifdef ROSS_GVT_mpi_allreduce
#  include <mpi.h>
#  include "gvt-mpi_allreduce.h"
#endif

#ifdef ROSS_MEMORY
#  include "tw-memoryq.h"
#  include "tw-memory.h"
#endif

#include "tw-eventq.h"
#include "ross-inline.h"

#endif
