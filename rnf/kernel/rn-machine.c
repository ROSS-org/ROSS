#include <rossnet.h>

rn_machine     *
rn_getmachine(tw_lpid id)
{
	return &g_rn_machines[id];
}

void
rn_setmachine(rn_machine * m)
{
	g_rn_machines[m->id] = *m;
}

rn_subnet	*
rn_getsubnet(rn_machine * m)
{
	return m->subnet;
}

rn_area		*
rn_getarea(rn_machine * m)
{
	return m->subnet->area;
}

rn_as		*
rn_getas(rn_machine * m)
{
	return m->subnet->area->as;
}

void
rn_machine_print(rn_machine * m)
{
	int	i;

	rn_link	*l;


	printf("\tnLinks: %d \n", m->nlinks);

	for(i = 0; i < m->nlinks; i++)
	{
		l = &m->link[i];

		if(NULL == l->wire)
			rn_link_setwire(l);

#if DYNAMIC_LINKS
		if(l->status == 0)
			printf("\t\tLink status: down \n");
		else
			printf("\t\tLink status: up\n");
#endif

		printf("\t\tLink to: %lld \n", 
			l->addr);
		printf("\t\tLink delay: %f \n", 
			l->delay);
		printf("\t\tLink bandwidth: %lf \n", 
			l->bandwidth);
		printf("\n");
	}
}
