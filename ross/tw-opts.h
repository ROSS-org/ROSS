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

#ifndef INC_tw_opts_h
#define INC_tw_opts_h

enum tw_opttype_tag;
typedef enum tw_opttype_tag tw_opttype;
enum tw_opttype_tag
{
	TWOPTTYPE_GROUP = 1,
	TWOPTTYPE_ULONG,   /* value must be an "unsigned long*" */
	TWOPTTYPE_UINT,    /* value must be an "unsigned int*" */
	TWOPTTYPE_STIME,   /* value must be an "tw_stime*" */
	TWOPTTYPE_CHAR,    /* value must be a char * */
	TWOPTTYPE_SHOWHELP
};

struct tw_optdef_tag;
typedef struct tw_optdef_tag tw_optdef;
struct tw_optdef_tag
{
	tw_opttype type;
	const char *name;
	const char *help;
	void *value;
};

#define TWOPT_GROUP(h)     { TWOPTTYPE_GROUP, NULL, (h), NULL }
#define TWOPT_ULONG(n,v,h) { TWOPTTYPE_ULONG, (n), (h), &(v) }
#define TWOPT_UINT(n,v,h)  { TWOPTTYPE_UINT,  (n), (h), &(v) }
#define TWOPT_STIME(n,v,h) { TWOPTTYPE_STIME, (n), (h), &(v) }
#define TWOPT_CHAR(n,v,h)  { TWOPTTYPE_CHAR,  (n), (h), &(v) }
#define TWOPT_END()        { 0, NULL, NULL, NULL }

/** Remove options from the command line arguments. */
extern void tw_opt_parse(int *argc, char ***argv);
extern void tw_opt_add(const tw_optdef *options);
extern void tw_opt_print(void);

#endif
