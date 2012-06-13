#ifndef CODESIOKERNELPARSER_H
#define CODESIOKERNELPARSER_H

#include "codesparser.h"
#include "CodesIOKernelContext.h"

YYLTYPE *CodesIOKernel_get_lloc  (yyscan_t yyscanner);
int CodesIOKernel_lex_init (yyscan_t* scanner);
int CodesIOKernel_lex(YYSTYPE * lvalp, YYLTYPE * llocp, void * scanner);
//YY_BUFFER_STATE CodesIOKernel__scan_string (yyconst char *yy_str ,yyscan_t yyscanner);

#endif
