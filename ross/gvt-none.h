#include <ross.h>

void tw_gvt_stats(FILE * F);
const tw_optdef	* tw_gvt_setup(void);
void tw_gvt_start(void);
void tw_gvt_force_update(tw_pe *me);
void tw_gvt_step1(tw_pe *me);
void tw_gvt_step2(tw_pe *me);
