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

#include <ross.h>

void
tw_printf(const char *file, int line, const char *fmt, ...)
{
	va_list	ap;

	va_start(ap, fmt);
	fprintf(stdout, "%s:%i: ", file, line);
	vfprintf(stdout, fmt, ap);
	fprintf(stdout, "\n");
	fflush(stdout);
	va_end(ap);
}

void
tw_error(const char *file, int line, const char *fmt, ...)
{
	va_list	ap;

	va_start(ap, fmt);
	fprintf(stdout, "error: %s:%i: ", file, line);
	vfprintf(stdout, fmt, ap);
	fprintf(stdout, "\n");
	fflush(stdout);
	fflush(stdout);
	va_end(ap);

	tw_net_abort();
}

void
tw_exit(int rv)
{
	tw_net_stop();
	exit(rv);
}

struct mem_pool
{
	struct mem_pool *next_pool;
	char *next_free;
	char *end_free;
};
static struct mem_pool *main_pool;

#ifdef ARCH_BLUE_GENE
static tw_mutex pool_lock;
#else
static tw_mutex pool_lock = TW_MUTEX_INIT;
#endif /* BLUE_GENE */

static const size_t pool_size = 512 * 1024 - sizeof(struct mem_pool);
static const size_t pool_align = max(sizeof(double),sizeof(void*));
static size_t total_allocated;
static unsigned malloc_calls;
static void* my_malloc(size_t len);

void
tw_calloc_stats(
	size_t *bytes_alloc,
	size_t *bytes_wasted)
{
	struct mem_pool *p;

	*bytes_alloc = total_allocated;
	*bytes_wasted = malloc_calls * (sizeof(void*) + sizeof(size_t));

	for (p = main_pool; p; p = p->next_pool)
		*bytes_wasted += p->end_free - p->next_free;
}

static void*
pool_alloc(size_t len)
{
	struct mem_pool *p;
	void *r;

#if 1
	if (len & (pool_align - 1))
		len += pool_align - (len & (pool_align - 1));
#endif

	tw_mutex_lock(&pool_lock);
	for (p = main_pool; p; p = p->next_pool)
		if ((p->end_free - p->next_free >= len))
			break;

	if (!p) {
		if (len >= pool_size) {
			r = my_malloc(len);
			goto ret;
		}

		p = my_malloc(sizeof(struct mem_pool) + pool_size);
		if (!p) {
			r = NULL;
			goto ret;
		}

		p->next_pool = main_pool;
		p->next_free = (char*)(p + 1);
		p->end_free = p->next_free + pool_size;
		main_pool = p;
	}

	r = p->next_free;
	p->next_free += len;

ret:
	if (r)
		total_allocated += len;
	tw_mutex_unlock(&pool_lock);
	return r;
}

void*
tw_calloc(
	const char *file,
	int line,
	const char *for_who,
	size_t e_sz,
	size_t n)
{
	void *r;

#if 0
	if(n & (pool_align - 1))
		n += pool_align - (n & (pool_align - 1));
#endif

	e_sz *= n;
	if (!e_sz)
		return NULL;

	r = pool_alloc(e_sz);
	if (!r)
		tw_error(
			file, line,
			"Cannot allocate %lu bytes for %u %s"
			" (need total of %lu KiB)",
			(unsigned long)e_sz,
			n,
			for_who,
			(unsigned long)((total_allocated + e_sz) / 1024));
	memset(r, 0, e_sz);
	return r;
}

#undef malloc
static void*
my_malloc(size_t len)
{
	malloc_calls++;
	return malloc(len);
}

#undef realloc
void*
tw_unsafe_realloc(
	const char *file,
	int line,
	const char *for_who,
	void *addr,
	size_t len)
{
	malloc_calls++;
	total_allocated += len;
	addr = realloc(addr, len);
	if (!addr)
		tw_error(
			file, line,
			"Cannot allocate %lu bytes for %s",
			(unsigned long)len,
			for_who);
	return addr;
}
