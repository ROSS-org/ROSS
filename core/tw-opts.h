#ifndef INC_tw_opts_h
#define INC_tw_opts_h

enum tw_opttype
{
	TWOPTTYPE_GROUP = 1,
	TWOPTTYPE_ULONG,       /**< value must be an "unsigned long*"      */
	TWOPTTYPE_ULONGLONG,   /**< value must be an "unsigned long long*" */
	TWOPTTYPE_UINT,        /**< value must be an "unsigned int*"       */
	TWOPTTYPE_STIME,       /**< value must be a  "tw_stime*"           */
        TWOPTTYPE_DOUBLE,      /**< value must be a  "double *"            */
	TWOPTTYPE_CHAR,        /**< value must be a  "char *"              */
        TWOPTTYPE_FLAG,        /**< value must be an "unsigned int*"       */
	TWOPTTYPE_SHOWHELP
};
typedef enum tw_opttype tw_opttype;

typedef struct tw_optdef tw_optdef;
struct tw_optdef
{
	tw_opttype type;
	const char *name;
	const char *help;
	void *value;
};

#define TWOPT_GROUP(h)         { TWOPTTYPE_GROUP,    NULL, (h), NULL }
#define TWOPT_ULONG(n,v,h)     { TWOPTTYPE_ULONG,     (n), (h), &(v) }
#define TWOPT_ULONGLONG(n,v,h) { TWOPTTYPE_ULONGLONG, (n), (h), &(v) }
#define TWOPT_UINT(n,v,h)      { TWOPTTYPE_UINT,      (n), (h), &(v) }
#define TWOPT_STIME(n,v,h)     { TWOPTTYPE_STIME,     (n), (h), &(v) }
#define TWOPT_DOUBLE(n,v,h)    { TWOPTTYPE_DOUBLE,    (n), (h), &(v) }
#define TWOPT_CHAR(n,v,h)      { TWOPTTYPE_CHAR,      (n), (h), &(v) }
#define TWOPT_FLAG(n,v,h)      { TWOPTTYPE_FLAG,      (n), (h), &(v) }
#define TWOPT_END()            { (tw_opttype)0,     NULL, NULL, NULL }

/** Remove options from the command line arguments. */
extern void tw_opt_parse(int *argc, char ***argv);
/** Add an opt group */
extern void tw_opt_add(const tw_optdef *options);
/** Set an option at runtime. Command line take precedence */
extern void tw_opt_set(const char *option, const char *value);
/** Pretty-print the option descriptions (for --help) */
extern void tw_opt_print(void);
/** Pretty-print the option descriptions and current values */
extern void tw_opt_settings(FILE *f);

#endif
