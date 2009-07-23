#include <tlm.h>

void
read_vector(unsigned int ** v, char * name, const xmlNodePtr node)
{
	xmlNodePtr	 e;
	unsigned int	*V;

	*v = tw_calloc(TW_LOC, name, sizeof(**v), g_tlm_spatial_dim);
	V = *v;

	for(e = node; e; e = e->next)
	{
		if(0 == strcmp((char *) e->name, "x"))
			V[0] = atoi((char *) e->children->content);
		if(0 == strcmp((char *) e->name, "y"))
			V[1] = atoi((char *) e->children->content);
		if(0 == strcmp((char *) e->name, "z"))
			V[2] = atoi((char *) e->children->content);
	}
}

void
tlm_xml(tlm_state * state, const xmlNodePtr node, tw_lp * lp)
{
	xmlNodePtr	 e;

	tw_lpid		 nlp;
	int		 i;

	if(0 == strcmp(xml_getprop(node, "dimensions"), "dimensions"))
		g_tlm_spatial_dim = atoi(xml_getprop(node, "dimensions"));

	for(e = node->children; e; e = e->next)
	{
		if(0 == strcmp((char *) e->name, "grid"))
			read_vector(&g_tlm_spatial_grid, "grid", e->children);

		if(0 == strcmp((char *) e->name, "spacing"))
			read_vector(&g_tlm_spatial_d, "spacing", e->children);
	}

	if(!g_tlm_spatial_grid)
		tw_error(TW_LOC, "TLM spatial grid not specified!");

	if(!g_tlm_spatial_d)
		tw_error(TW_LOC, "TLM spatial grid spacing not specified!");

	if(g_tlm_spatial_dim == 3)
		g_tlm_z_max = g_tlm_spatial_grid[2] * g_tlm_spatial_d[2];

	for(i = 0, nlp = 1; i < g_tlm_spatial_dim; i++)
		nlp *= g_tlm_spatial_grid[i];

	g_rn_env_nlps = nlp;
}
