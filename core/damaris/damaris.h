#include <ross.h>

extern void st_set_damaris_parameters(int num_lp);
extern void st_ross_damaris_init();
extern void st_ross_damaris_finalize();
extern void st_expose_data_damaris(tw_pe *me, tw_stime gvt, tw_statistics *s, int inst_type);
extern void reset_block_counter();
extern void damaris_error(const char *file, int line, int err, char *variable);
