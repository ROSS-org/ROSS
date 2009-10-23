#include <rossnet.h>
#include <rn_config.h>

void
init_stream(rn_lp_state * state, rn_stream * stream, int i, tw_lp * lp)
{
	tw_lp			*layer_lp;

	rn_layer		*layer;

	int			 j;

#if RN_VERIFY_SETUP
	printf("\tstream port: %d, nlayers %d \n", 
		stream->port, stream->nlayers);
#endif

	/*
	 * Setup the stream layers
	 */
	layer = &stream->layers[stream->nlayers - 1];
	for(j = stream->nlayers - 1; j >= 0; layer = &stream->layers[--j])
	{
		layer_lp = &layer->lp;

		state->cur_lp = layer_lp;
		state->cur_stream = i;
		stream->cur_layer = j;

		lp->rng = layer_lp->rng;

		/*
		 * Layer LP INIT function call
		 */
		if(layer_lp->type.init)
			(*(init_f)layer_lp->type.init) (layer_lp->cur_state, lp);
		else
			tw_error(TW_LOC, "No init routine: LP %d!", lp->gid);
	}
}

/**
 * This function calls the INIT routines of the star model layers
 */
void
rn_init_streams(rn_lp_state * state, tw_lp * lp)
{
	rn_stream		*stream;

	int			 i;

#if RN_VERIFY_SETUP
	printf("INIT star model for node: %lld, nstreams %d\n",
		lp->gid, state->nstreams);
#endif

	stream = state->streams;
	for(i = 0; i < state->nstreams; stream = &state->streams[++i])
		init_stream(state, stream, i, lp);

	stream = state->mobility;
	for(i = 0; i < state->nmobility; stream = &state->mobility[++i])
		init_stream(state, stream, i, lp);
}

void
rn_init_environment()
{
	xmlNodePtr	 node;

	rn_lptype	*t;

	int		 i;

	if(!g_rn_environment)
		return;

	node = g_rn_environment->children;

	// support for multiple environment elements?
	for(node = g_rn_environment->children; node; node = node->next)
	{
		if(strcmp((char *) node->name, "layer") == 0)
		{
			for(t = g_rn_lptypes, i = 0; t; t = &g_rn_lptypes[++i])
			{
				if(strcmp(xml_getprop(node, "name"), t->sname) == 0)
				{
					if(t->xml_init)
						(*(rn_xml_init_f) t->xml_init) (NULL, node, NULL);

					break;
				}
			}
		}
	}
}
