#include <ospf.h>

	/*
	 * This code tells me the event rate during different times 
	 * during the OSPF protocol (flooding, building RTs, etc)
	 */
void
ospf_router_debug_rate(tw_lp * lp, int * next)
{
#if 0
	if(lp->id == 0 && lp->kp->s_nevent_processed > *next)
	{
		tw_wall_now(&g_tw_pe[0].end_time);
		tw_wall_sub(&g_tw_pe[0].run_time, &g_tw_pe[0].end_time,
				&g_tw_pe[0].start_time);

		printf("Events processed: %ld \n", 
				lp->kp->s_nevent_processed);

		printf("Event rate: %11.2f (events/sec)\n", 
			(double) lp->kp->s_nevent_processed / 
			tw_wall_to_double(&g_tw_pe[0].run_time));

		*next += 1000000;
	}
#endif
}

int x = 0;

void
ospf_router_debug_routing(ospf_state * state, int d, tw_lp * lp)
{
#if 0
	FILE	*f;
	char	 file[1024];

	sprintf(file, "ospf/logs/r%ld-%d.log", lp->id, x++);
	f = fopen(file, "w");

	fprintf(f, "src\t\tdst\n");
	for(d = 0; d < g_rn_nrouters; d++)
	{
		if(state->ft[d] == 0xff)
			fprintf(f, "%d\t\tno route\n", d);
		else if(lp->id == d)
			fprintf(f, "%d\t\t self d\n", d);
		else
			fprintf(f, "%d\t\t%d\n", d, state->ft[d]);
	}

	fclose(f);
#endif
}
