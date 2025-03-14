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
Copyright (c) 2013, Rensselaer Polytechnic Institute
All rights reserved.

Redistribution and  use in  source and binary  forms, with  or without
modification, are permitted provided that the following conditions are
met:

  Redistributions  of  source code  must  retain  the above  copyright
  notice, this list of conditions and the following disclaimer.

  Redistributions in  binary form  must reproduce the  above copyright
  notice, this list of conditions  and the following disclaimer in the
  documentation and/or other materials provided with the distribution.

  Neither the  name of Rensselaer Polytechnic Institute  nor the names
  of  its contributors  may be  used  to endorse  or promote  products
  derived   from  this   software  without   specific   prior  written
  permission.

THIS SOFTWARE  IS PROVIDED BY  THE COPYRIGHT HOLDERS  AND CONTRIBUTORS
"AS  IS" AND  ANY EXPRESS  OR IMPLIED  WARRANTIES, INCLUDING,  BUT NOT
LIMITED TO, THE IMPLIED  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE  ARE DISCLAIMED. IN NO EVENT  SHALL THE COPYRIGHT
HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL,  EXEMPLARY,  OR  CONSEQUENTIAL  DAMAGES (INCLUDING,  BUT  NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE  GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS  INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF  LIABILITY, WHETHER IN  CONTRACT, STRICT LIABILITY,  OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING  IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

/*******************************************************************
 * The location of this include is important, as it is outside of  *
 * the __cplusplus check.  This is required as the mpi header will *
 * mess up and complain if we force it into an extern "C" context. *
 *******************************************************************/
#include <mpi.h>

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************
 *
 * Include ``standard'' headers that most of ROSS will require.
 *
 ********************************************************************/

#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>

#ifdef USE_BGPM
#include<bgpm.h>
#endif

#include "ross-base.h"
#include "ross-clock.h"
#include "ross-gvt.h"
#include "ross-inline.h"
#include "ross-kernel-inline.h"
#include "ross-types.h"
#include "ross-extern.h"
#include "ross-random.h"

#include "instrumentation/st-instrumentation.h"
#include "check-revent/crv-state.h"

// Optional headers not needed by any other headers (above)
#ifdef USE_DAMARIS
#include "damaris/core/damaris.h"
#endif


#ifdef __cplusplus
}
#endif

#endif
