#ifndef INC_gvt_mpi_allreduce_h
#define INC_gvt_mpi_allreduce_h

static double gvt_print_interval = 0.01;
static double percent_complete = 0.0;

static inline int
tw_gvt_inprogress(tw_pe * pe)
{
	return pe->gvt_status;
}

static inline void
gvt_print(tw_stime gvt)
{
	if(gvt_print_interval == 1.0)
		return;

	if(percent_complete == 0.0)
	{
		percent_complete = gvt_print_interval;
		return;
	}

    printf("GVT #%d: simulation %d%% complete, max event queue size %u (",
               g_tw_gvt_done,
               (int) ROSS_MIN(100, floor(100 * (gvt/g_tw_ts_end))),
               tw_pq_max_size(g_tw_pe->pq));

    if (gvt == DBL_MAX)
        printf("GVT = %s", "MAX");
    else
        printf("GVT = %.4f", gvt);

    printf(").\n");

#ifdef AVL_TREE
    printf("AVL tree size: %d\n", g_tw_pe->avl_tree_size);
#endif

	percent_complete += gvt_print_interval;
}

extern tw_stat st_get_allreduce_count();

#endif
