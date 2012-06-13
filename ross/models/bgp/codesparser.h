
/* A Bison parser, made by GNU Bison 2.4.1.  */

/* Skeleton interface for Bison's Yacc-like parsers in C
   
      Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.
   
   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     INTEGER = 258,
     VARIABLE = 259,
     WHILE = 260,
     IF = 261,
     PRINT = 262,
     WRITE = 263,
     WRITEAT = 264,
     WRITE_ALL = 265,
     WRITEAT_ALL = 266,
     READ = 267,
     READAT = 268,
     READ_ALL = 269,
     READAT_ALL = 270,
     SYNC = 271,
     SLEEP = 272,
     OPEN = 273,
     CLOSE = 274,
     DELETE = 275,
     FLUSH = 276,
     SEEK = 277,
     EXIT = 278,
     IFX = 279,
     ELSE = 280,
     NE = 281,
     EQ = 282,
     LE = 283,
     GE = 284,
     GETNUMGROUPS = 285,
     GETGROUPID = 286,
     GETCURTIME = 287,
     GETGROUPSIZE = 288,
     GETGROUPRANK = 289,
     UMINUS = 290
   };
#endif
/* Tokens.  */
#define INTEGER 258
#define VARIABLE 259
#define WHILE 260
#define IF 261
#define PRINT 262
#define WRITE 263
#define WRITEAT 264
#define WRITE_ALL 265
#define WRITEAT_ALL 266
#define READ 267
#define READAT 268
#define READ_ALL 269
#define READAT_ALL 270
#define SYNC 271
#define SLEEP 272
#define OPEN 273
#define CLOSE 274
#define DELETE 275
#define FLUSH 276
#define SEEK 277
#define EXIT 278
#define IFX 279
#define ELSE 280
#define NE 281
#define EQ 282
#define LE 283
#define GE 284
#define GETNUMGROUPS 285
#define GETGROUPID 286
#define GETCURTIME 287
#define GETGROUPSIZE 288
#define GETGROUPRANK 289
#define UMINUS 290




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 1676 of yacc.c  */
#line 36 "src/common/iokernellang/codesparser.y"

    int64_t iValue;                 /* integer value */
    int sIndex;              /* symbol table index */
    nodeType *nPtr;             /* node pointer */



/* Line 1676 of yacc.c  */
#line 130 "src/common/iokernellang/codesparser.h"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif



#if ! defined YYLTYPE && ! defined YYLTYPE_IS_DECLARED
typedef struct YYLTYPE
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
} YYLTYPE;
# define yyltype YYLTYPE /* obsolescent; will be withdrawn */
# define YYLTYPE_IS_DECLARED 1
# define YYLTYPE_IS_TRIVIAL 1
#endif



#ifndef YYPUSH_DECLS
#  define YYPUSH_DECLS
struct CodesIOKernel_pstate;
typedef struct CodesIOKernel_pstate CodesIOKernel_pstate;
enum { YYPUSH_MORE = 4 };
#if defined __STDC__ || defined __cplusplus
int CodesIOKernel_push_parse (CodesIOKernel_pstate *yyps, int yypushed_char, YYSTYPE const *yypushed_val, YYLTYPE const *yypushed_loc, CodesIOKernelContext * context);
#else
int CodesIOKernel_push_parse ();
#endif

#if defined __STDC__ || defined __cplusplus
CodesIOKernel_pstate * CodesIOKernel_pstate_new (void);
#else
CodesIOKernel_pstate * CodesIOKernel_pstate_new ();
#endif
#if defined __STDC__ || defined __cplusplus
void CodesIOKernel_pstate_delete (CodesIOKernel_pstate *yyps);
#else
void CodesIOKernel_pstate_delete ();
#endif
#endif

