#ifndef CODES_H
#define CODES_H

#include "CodesIOKernelContext.h"

typedef enum { typeCon, typeId, typeOpr } nodeEnum;

/* constants */
typedef struct {
    int64_t value;                  /* value of constant */
} conNodeType;

/* identifiers */
typedef struct {
    int i;                      /* subscript to sym array */
} idNodeType;

/* operators */
typedef struct {
    int oper;                   /* operator */
    int nops;                   /* number of operands */
    struct nodeTypeTag *op[1];  /* operands (expandable) */
} oprNodeType;

typedef struct nodeTypeTag {
    nodeEnum type;              /* type of node */

    /* Ning's additions for the bgp storage model */
    /* XXX does this belong here ? */
    int GroupRank;
    int GroupSize;

    /* union must be last entry in nodeType */
    /* because operNodeType may dynamically increase */
    union {
        conNodeType con;        /* constants */
        idNodeType id;          /* identifiers */
        oprNodeType opr;        /* operators */
    };
} nodeType;

#ifndef YY_TYPEDEF_YY_SCANNER_T
#define YY_TYPEDEF_YY_SCANNER_T
typedef void* yyscan_t;
#endif

typedef union codesYYType {
    int64_t iValue;                 /* integer value */
    int sIndex;              /* symbol table index */
    nodeType *nPtr;             /* node pointer */
} codesYYType;
#define YYSTYPE codesYYType 

typedef struct codesParserParam
{
        yyscan_t scanner;
        void * nPtr;
}codesParserParam;

// the parameter name (of the reentrant 'yyparse' function)
// data is a pointer to a 'codesParserParam' structure
//#define YYPARSE_PARAM data

// the argument for the 'yylex' function
//#define YYLEX_PARAM   ((codesParserParam *)data)->scanner

extern int64_t * sym;
extern int64_t * var;
extern int * inst_ready;

extern int * group_rank;
extern int * group_size;

#endif
