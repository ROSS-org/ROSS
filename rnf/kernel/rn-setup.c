#include <rossnet.h>

#define RN_VERIFY_SETUP 0

void
rn_setup_layers(rn_lp_state * state, int cur_stream, rn_layer * layer, 
		rn_lptype * types, xmlNodePtr x_layer, tw_lp * me)
{
	xmlNodePtr		 layer_xml;
	rn_lptype		*t;
	rn_lptype		*type_set = NULL;

	tw_lp			*layer_lp;

	int			 i;

	/* 
	 * Got the XML for the layer, now setup layer, create LP state,
	 * and call layer XML, INIT functions
	 */
	layer_lp = &layer->lp;
	layer_lp->gid = me->gid;
	layer_lp->id = me->id;
	layer_lp->pe = me->pe;
	layer_lp->kp = me->kp;

	layer_lp->state_qh = NULL;

	for(t = types, i = 0; t && t->init; t = &types[++i])
	{
		if(strcmp(xml_getprop(x_layer, "name"), t->sname) == 0)
		{
			//tw_lp_settype(layer_lp, t->name);
			memcpy(&layer_lp->type, &t->init, sizeof(tw_lptype));
			type_set = rn_lp_gettype(layer_lp);

#if RN_VERIFY_SETUP
			printf("\t\ttype: %s\n", t->sname);
#endif
			break;
		}
	}

	if(NULL == type_set)
		tw_error(TW_LOC, "No type for LP %d, layer: %s \n", 
				me->id, xml_getprop(x_layer, "name"));

	tw_state_alloc(layer_lp, 1);

/*
	layer_lp->cur_state = layer_lp->state_qh;
	layer_lp->state_qh = layer_lp->state_qh->next;
*/

	layer_xml = x_layer;
	x_layer = x_layer->next;
	while(x_layer && 0 != strcmp((char *) x_layer->name, "layer"))
		x_layer = x_layer->next;

#if RN_VERIFY_SETUP
	printf("\t\tinit: %ld - %s\n", 
			(long int) layer, t->sname);
#endif

	if(x_layer)
		rn_setup_layers(state, cur_stream, ++layer, types, x_layer, me);

	state->cur_lp = layer_lp;
	state->cur_stream = cur_stream;

	/*
	 * Layer LP XML function call
	 */
	if(type_set->xml_init)
		(*(rn_xml_init_f) type_set->xml_init) (layer_lp->cur_state, layer_xml, layer_lp);
	else if (NULL != t->xml_init)
		tw_error(TW_LOC, "No XML function for lp %d!\n", me->id);
}

void
rn_setup_mobility(rn_lp_state * state, rn_lptype * types, tw_lp * lp)
{
	xmlNodePtr		x_temp;
	xmlNodePtr		x_mobility;
	xmlNodePtr		x_layer;

	rn_machine	*m;
	rn_stream	*mobility;
	rn_layer	*layer;

	int		 cur_mobility;
	int		 cur_layer;

	m = rn_getmachine(lp->gid);
	state->nmobility = 0;

	for(x_mobility = m->xml->children; x_mobility; x_mobility = x_mobility->next)
	{
		if(0 != strcmp((char *) x_mobility->name, "mobility"))
			continue;

		state->nmobility++;
	}

	if(!state->nmobility)
		return;

	state->mobility = tw_calloc(TW_LOC, "Local mobility array", 
				    sizeof(*state->mobility), state->nmobility);

	mobility = state->mobility;
	cur_layer = 0;
	cur_mobility = 0;

	for(x_mobility = m->xml->children; x_mobility; x_mobility = x_mobility->next)
	{
		if(0 != strcmp((char *) x_mobility->name, "mobility"))
			continue;

		/*
		 * Setup the mobility layers
		 */
		for(x_temp = x_mobility->children; x_temp; x_temp = x_temp->next)
		{
			if(0 != strcmp((char *) x_temp->name, "layer"))
				continue;

			mobility->nlayers++;
		}

		mobility->layers = tw_calloc(TW_LOC, "Local layer array",
					     sizeof(*mobility->layers), mobility->nlayers);

		layer = mobility->layers;
		x_layer = x_mobility->children;

		while(0 != strcmp((char *) x_layer->name, "layer"))
			x_layer = x_layer->next;

		rn_setup_layers(state, cur_mobility, layer, types, x_layer, lp);

		mobility = mobility + 1;
		cur_mobility++;
	}
}

/**
 * This function sets up the star model for the different layers
 */
void
rn_setup_streams(rn_lp_state * state, rn_lptype * types, tw_lp * lp)
{
	xmlNodePtr		node;
	xmlNodePtr		x_temp;
	xmlNodePtr		x_stream;
	xmlNodePtr		x_layer;

	int			cur_stream;

	rn_layer		*layer;
	rn_stream		*stream;
	rn_machine		*m;

	m = rn_getmachine(lp->gid);
	state->nstreams = 0;

#if RN_VERIFY_SETUP
	printf("SETUP star model for node: %lld \n", lp->gid);
#endif

	for(x_stream = m->xml->children; x_stream; x_stream = x_stream->next)
	{
		if(0 != strcmp((char *) x_stream->name, "stream"))
			continue;

		state->nstreams++;
	}

	if(!state->nstreams)
		return;

	state->streams = tw_calloc(TW_LOC, "Local stream array", 
				   sizeof(*stream), state->nstreams);

	if(!state->streams)
		tw_error(TW_LOC, "Did not get any streams for machine %d!", lp->gid);

	node = m->xml;
	stream = state->streams;
	cur_stream = 0;
	for(x_stream = node->children; x_stream; x_stream = x_stream->next)
	{
		if(0 != strcmp((char *) x_stream->name, "stream"))
			continue;

		stream->port = atoi(xml_getprop(x_stream, "port"));

#if RN_VERIFY_SETUP
		printf("\tport: %d \n", stream->port);
#endif

		/*
		 * Setup the stream layers
		 */
		for(x_temp = x_stream->children; x_temp; x_temp = x_temp->next)
		{
			if(0 != strcmp((char *) x_temp->name, "layer"))
				continue;

			stream->nlayers++;
		}

		stream->layers = tw_calloc(TW_LOC, "Local layer array",
					   sizeof(*stream->layers), stream->nlayers);

		layer = stream->layers;
		x_layer = x_stream->children;

#if RN_VERIFY_SETUP
		printf("\tlayers: %d\n", stream->nlayers);
#endif

		while(0 != strcmp((char *) x_layer->name, "layer"))
			x_layer = x_layer->next;

		rn_setup_layers(state, cur_stream, layer, types, x_layer, lp);

		stream = stream + 1;
		cur_stream++;
	}
}

/*
 * This function should handle the link status changes
 * that occur by sending a link status change event for each link on
 * this LP which is going to change.
 */
void
rn_setup_links(rn_lp_state * state, tw_lp * lp)
{
#if DYNAMIC_LINKS
	tw_event	*e;

	rn_message	*msg;
	rn_machine	*m;

	int		 i;

	m = rn_getmachine(lp->gid);

	for(i = 0; i < m->nlinks; i++)
	{
		if(m->link[i].next_status == 0 || m->link[i].next_status == INT_MAX)
			continue;

#if 1
		printf("%lld: Setting link status event for time %lf, now %lf \n", 
				lp->gid, m->link[i].next_status, tw_now(lp));
#endif

		g_rn_nlink_status++;
		e = tw_event_new(lp, m->link[i].next_status, lp);
		msg = tw_event_data(e);

		msg->port = i;
		msg->src = lp->gid;
		msg->dst = lp->gid;

		if(m->link[i].status == rn_link_up)
			msg->type = LINK_STATUS_DOWN;
		else
			msg->type = LINK_STATUS_UP;

		tw_event_send(e);
	}
#endif
}

/**
 * If the global variable g_rn_tools_dir is set, look for the xml
 * topology, link topology, and routing table configuration in that
 * directory.  Should not interfere with NetDMF configuration regardless
 * of the value of g_rn_tools_dir
 */
void
rn_setup()
{
	if(0 != strcmp(g_rn_tools_dir, ""))
	{
		char * dir = g_rn_tools_dir;
		sprintf(g_rn_xml_topology, "tools/%s/%s.xml", dir, dir);
		sprintf(g_rn_rt_table, "tools/%s/%s.rt", dir, dir);
		sprintf(g_rn_xml_link_topology, "tools/%s/links.xml", dir);
		//sprintf(g_rn_xml_model, "tools/%s/model.xml", dir);

		if(tw_ismaster())
		{
			printf("Opening config file: %s \n", g_rn_xml_topology);
			printf("Opening link file: %s \n", g_rn_xml_link_topology);
			printf("Opening routing table file: %s \n", g_rn_rt_table);
		}
	}
}
