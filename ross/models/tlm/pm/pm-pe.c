#include <pm.h>

void pm_pe_init(tw_pe * pe) {}
void pm_pe_post_init(tw_pe * pe) { }
void pm_pe_gvt(tw_pe * pe) { }

void
pm_pe_final(tw_pe * pe)
{
#if 0
	pm_statistics	 copy;

	if(!g_tw_mynode)
		printf("In PM_PE.c\n");

	// reduce RM statistics
	if(MPI_Reduce(
			&(g_pm_stats->s_move_ev),
			&(copy.s_move_ev),
			6,
			MPI_LONG_LONG,
			MPI_SUM,
			g_tw_masternode,
			MPI_COMM_WORLD) != MPI_SUCCESS)
		tw_error(TW_LOC, "Unable to reduce statistics: rm\n");

	memcpy(g_pm_stats, &copy, sizeof(*g_rm_stats));
#endif
}
