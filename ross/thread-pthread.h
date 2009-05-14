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

#ifndef INC_thread_pthread_h
#define	INC_thread_pthread_h

#include <pthread.h>

DEF(struct, tw_mutex)
{
	pthread_mutex_t	lock;
};
#define TW_MUTEX_INIT { PTHREAD_MUTEX_INITIALIZER }

DEF(struct, tw_barrier)
{
	pthread_mutex_t lock;
	tw_peid n_clients;
	tw_peid n_waiting;
	tw_volatile int phase;
};

INLINE(void)
tw_mutex_lock(tw_mutex *lck)
{
	while(pthread_mutex_trylock(&lck->lock) == EBUSY);
}

INLINE(void)
tw_mutex_unlock(tw_mutex *lck)
{
	pthread_mutex_unlock(&lck->lock);
}

INLINE(void)
tw_barrier_sync(tw_barrier * b)
{
	tw_volatile int my_phase;
	unsigned int	usecs = 0;

	pthread_mutex_lock(&(b->lock));

	my_phase = b->phase;
	if (++b->n_waiting == b->n_clients)
	{
		b->n_waiting = 0;
		b->phase = 1 - my_phase;
	}

	pthread_mutex_unlock(&(b->lock));

	while (b->phase == my_phase)
		usleep(usecs);
}

#endif
