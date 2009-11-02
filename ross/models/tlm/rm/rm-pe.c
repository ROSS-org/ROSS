#include <rm.h>

#define PLOT 0

void
rm_pe_init(tw_pe * pe)
{
#if PLOT
	rm_pe	*rpe = tw_pe_data(pe);

	int	 n = *tw_net_onnode(pe->id);
	char	 fn[1024];

	rpe = pe->data = tw_calloc(TW_LOC, "PE State", sizeof(rm_pe), 1);

	sprintf(fn, "n%05d-p%05d-wave.log", n, pe->id);

	if(NULL == (rpe->wave_log = fopen(fn, "a")))
		tw_error(TW_LOC, "Unable to open: %s\n", fn);

	sprintf(fn, "n%05d-p%05d-move.log", n, pe->id);

	if(NULL == (rpe->move_log = fopen(fn, "a")))
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
	rm_statistics	 copy;

#if PLOT
	rm_pe	*rpe = tw_pe_data(pe);

	fclose(rpe->wave_log);
	fclose(rpe->move_log);
#endif

	// reduce RM statistics
	if(MPI_Reduce(
			&g_rm_stats->s_nparticles,
			&copy.s_nparticles,
			4,
			MPI_LONG_LONG,
			MPI_SUM,
			g_tw_masternode,
			MPI_COMM_WORLD) != MPI_SUCCESS)
		tw_error(TW_LOC, "Unable to reduce statistics: rm\n");

	memcpy(g_rm_stats, &copy, sizeof(*g_rm_stats));

	// call PM pe_final
	(g_rm_pe.type.final)(pe);
}
