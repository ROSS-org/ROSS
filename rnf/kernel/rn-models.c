#include <rossnet.h>

/*
 * This function gives each model a chance to initialize global memory
 * prior to the simulation execution.
 *
 * At this point, ROSS is still only a single
 * thread, and rn has called tw_init, so that the LPs, KPs, PEs exist and
 * are mapped.  Also, the xml file(s) have been read and the RN main data
 * structures have been created.
 */
void
rn_models_init(rn_lptype * types, int argc, char ** argv, char ** env)
{
	rn_lptype	*t;

	int		 i;

	for(t = types, i = 0; t && t->init; t = &types[++i])
		if(*t->md_init)
			(*t->md_init) (argc, argv, env);
}

/*
 * This function gives each model a chance to complete operations after 
 * the simulation has completed.
 */
void
rn_models_final(rn_lptype * types)
{
	rn_lptype	*t;
	int		 i;

	for(t = types, i = 0; t && t->init; t = &types[++i])
		if(*t->md_final)
			(*t->md_final) ();
}
