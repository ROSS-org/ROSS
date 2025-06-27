#ifndef INC_gvt_mpi_allreduce_h
#define INC_gvt_mpi_allreduce_h

#include "ross-extern.h"
#include "ross-kernel-inline.h"
#include "ross-types.h"
#include "queue/tw-queue.h"
#include <math.h>
#include <sys/time.h>
#include <time.h>

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

    double ts = TW_STIME_DBL(gvt);

    printf("GVT #%d: simulation %d%% complete, max event queue size %u (",
               g_tw_gvt_done,
               (int) ROSS_MIN(100, floor(100 * (ts/g_tw_ts_end))),
               tw_pq_max_size(g_tw_pe->pq));

    if (ts == DBL_MAX)
        printf("GVT = %s", "MAX");
    else
        printf("GVT = %.4f", ts);

#if HAVE_CTIME
    time_t raw_time;
    struct tm * timeinfo;
    char time_str [80];
    time(&raw_time);
    timeinfo = localtime(&raw_time);
    strftime(time_str, 80, "%c", timeinfo);
    printf(") at %s.\n", time_str);
#else
    printf(").\n");
#endif

#ifdef AVL_TREE
    printf("AVL tree size: %d\n", g_tw_pe->avl_tree_size);
#endif

	percent_complete += gvt_print_interval;
}

extern tw_stat st_get_allreduce_count();

#endif
