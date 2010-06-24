#ifndef INC_tw_opts_h
#define INC_tw_opts_h

typedef enum tw_opttype tw_opttype;
enum tw_opttype
{
	TWOPTTYPE_GROUP = 1,
	TWOPTTYPE_ULONG,   /* value must be an "unsigned long*" */
	TWOPTTYPE_UINT,    /* value must be an "unsigned int*" */
	TWOPTTYPE_STIME,   /* value must be an "tw_stime*" */
	TWOPTTYPE_CHAR,    /* value must be a char * */
	TWOPTTYPE_SHOWHELP
};

typedef struct tw_optdef tw_optdef;
struct tw_optdef
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
