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

#include <stdlib.h>
#include <stdio.h>

struct var
{
	const char *name;
	const char *value;
	const char *help;
};

const struct var values[] = {
	{ "cc", CC, "C compiler used by ROSS" },
	{ "cflags", CFLAGS, "C flags used by ROSS" },
	{ "ldflags", LDFLAGS, "Linker flags used by ROSS" },
	{ NULL, NULL }
};

static int
usage(void)
{
	unsigned i;
	fputs("usage: ross-config option\n", stderr);
	fputc('\n', stderr);
	fputs("Recognized options:\n", stderr);
	for (i = 0; values[i].name; i++)
		fprintf(stderr,
			"    --%-15s %s\n",
			values[i].name,
			values[i].help);
	return 1;
}

int
main(int argc, char **argv)
{
	const char *name;
	unsigned i;

	if (argc != 2)
		return usage();

	name = argv[1];
	if (!strncmp(name, "--", 2))
		name += 2;
	if (!*name)
		return usage();

	for (i = 0; values[i].name; i++) {
		if (!strcmp(name, values[i].name)) {
			fputs(values[i].value, stdout);
			fputc('\n', stdout);
			return 0;
		}
	}

	return usage();
}
