#include "wifi-mac.h"


void
rm_pe_init(tw_pe * pe)
{
#if PLOT
	rm_pe	*rpe = tw_pe_data(pe);

	int	 n = *tw_net_onnode(pe->id);
	char	 fn[1024];

	rpe = pe->data = tw_calloc(TW_LOC, "PE State", sizeof(rm_pe), 1);

	sprintf(fn, "n%05d-p%05d-wave.log", n, pe->id);

	if(NULL == (rpe->wave_log = fopen(fn, "w")))
		tw_error(TW_LOC, "Unable to open: %s\n", fn);
	
	sprintf(fn, "n%05d-p%05d-move.log", n, pe->id);

	if(NULL == (rpe->move_log = fopen(fn, "w")))
		tw_error(TW_LOC, "Unable to open: %s\n", fn);
#endif
	
	//printf("%d: in pe init: %ld\n", (int) * tw_net_onnode(me->id), nlp_per_pe);
}

void
rm_pe_post_init(tw_pe * pe)
{
}

void
rm_pe_gvt(tw_pe * pe)
{
}

void
rm_pe_final(tw_pe * pe)
{
#if PLOT
	rm_pe	*rpe = tw_pe_data(pe);
	fclose(rpe->wave_log);
	fclose(rpe->move_log);
#endif
}
