#include <rossnet.h>

#define RN_XML_DEBUG 0
void xml_link_topology();

void
verify_topology()
{

	rn_as		*as;
	rn_area		*ar;
	rn_subnet	*sn;
	rn_machine	*m;
	//rn_link		*l;

	//int		 i;

	tw_error(TW_LOC, "Machine next pointers not set!");

	printf("\n\nPrinting Datastructure: \n\n");

	for (as = NULL; NULL != (as = rn_nextas(as));)
	{
		printf("AS %d: range: %lld - %lld, nAreas = %d\n", 
			as->id, as->low, as->high, as->nareas);

		for (ar = NULL; NULL != (ar = rn_nextarea_onas(ar, as));)
		{
			printf("\tArea %d: range: %lld - %lld, nsubnets = %d\n", 
					ar->id, ar->low, ar->high, ar->nsubnets);

			for (sn = NULL; NULL != (sn = rn_nextsn_onarea(sn, ar));)
			{
				printf("\t\tSubnet %d: range: %lld - %lld, nmachines = %d\n", 
					sn->id, sn->low, sn->high, sn->nmachines);

				for (m = NULL; NULL != (m = rn_nextmachine_onsn(m, sn));)
				{
					printf("\t\t\tMachine: %lld\n", m->id);
					printf("\t\t\tnLinks: %d \n", m->nlinks);

#if 0
					for(i = 0; i < m->nlinks; i++)
					{
						l = &m->link[i];

#if DYNAMIC_LINKS
						printf("\t\t\t\tLink status: %d \n", 
							l->status);
#endif

						printf("\t\t\t\tLink to: %d \n", 
							l->addr);
						printf("\t\t\t\tLink delay: %f \n", 
							l->delay);
						printf("\t\t\t\tLink bandwidth: %lf \n", 
							l->bandwidth);
						printf("\n");
					}
#endif
				}
			}
		}
	}
}

xmlXPathObjectPtr
xpath(char * expr, xmlXPathContextPtr ctxt)
{
	return xmlXPathEvalExpression(BAD_CAST expr, ctxt);
}

/*
 * This function inits the parser and gets the total number of LPs to create
 */
void
rn_xml_init()
{
#if RN_XML_DEBUG
	printf("Reading in XML topology file: %s\n\n", g_rn_xml_topology);
#endif

	xmlXPathInit();
	document_network = xmlReadFile(g_rn_xml_topology, NULL , 0);

	if(!document_network)
		tw_error(TW_LOC, "Parsing of network topology file %s failed!", 
			g_rn_xml_topology);

	ctxt = xmlXPathNewContext(document_network);

	if(!ctxt)
		tw_error(TW_LOC, "Could not create network topology XML context!");

	/* read in the XML network topology */
	rn_xml_topology();

	if(g_rn_xml_link_topology != NULL)
	{
		document_links = xmlReadFile(g_rn_xml_link_topology, NULL ,0);

		if(!document_links)
			tw_error(TW_LOC, "Parsing of link topology file %s failed!", 
				g_rn_xml_link_topology);

		ctxt_links = xmlXPathNewContext(document_links);

		if(!ctxt_links)
			tw_error(TW_LOC, "Unable to create model XML context!");

		/* read in the XML link topology */
		rn_xml_link_topology();
		xmlXPathFreeContext(ctxt_links);
	}

	if(NULL != g_rn_xml_model && 0 != strcmp(g_rn_xml_model, ""))
	{
		document_model = xmlReadFile(g_rn_xml_model, NULL, 0);

		if(!document_model)
			tw_error(TW_LOC, "Parsing of model file %s failed!", 
				g_rn_xml_model);

		ctxt_model = xmlXPathNewContext(document_model);

		if(!ctxt_model)
			tw_error(TW_LOC, "Unable to create model XML context!");

		rn_xml_model();
		xmlXPathFreeContext(ctxt_model);
	}
}

void
rn_xml_end()
{
	if(ctxt)
		xmlXPathFreeContext(ctxt);

	if(document_network)
		xmlFreeDoc(document_network);

	if(document_links)
		xmlFreeDoc(document_links);

	if(document_model)
		xmlFreeDoc(document_model);

	xmlCleanupParser();
}

void
rn_xml_link_topology()
{
	xmlXPathObjectPtr	obj;

	xmlNodePtr		node;

	int			src;
	int			dst;

	rn_machine		*d;
	rn_machine		*s;

	rn_link			*s_l;
	rn_link			*d_l;

	/* Statuses */
	obj = xpath("//status", ctxt_links);

	if(0 == obj->nodesetval->nodeNr)
		return;

	for(node = *obj->nodesetval->nodeTab; node != NULL; node = node->next)
	{
		if(0 != strcmp((char *) node->name, "status"))
			continue;

		src = atoi(xml_getprop(node, "src"));
		dst = atoi(xml_getprop(node, "addr"));

		s = rn_getmachine(src);
		d = rn_getmachine(dst);

		/*
		 * Should check link type and change the status approp
		 */
		s_l = rn_getlink(s, d->id);
		d_l = rn_getlink(d, s->id);

		if(!s_l || !d_l)
		{
			printf("Bad link specified in links.xml -- ignoring! \n");
			continue;
		}

#if DYNAMIC_LINKS
		s_l->next_status = d_l->next_status = atoi(xml_getprop(node, "change"));
		s_l->last_status = d_l->last_status = 0;
#endif
	}

	xmlXPathFreeObject(obj);
}

char *
xml_getprop(xmlNode * node, char * prop)
{
	xmlAttrPtr next = node->properties;

	while(next)
	{
		if(0 == strcmp(prop, (char *) next->name))
			return (char *) next->children->content;

		next = next->next;
	}

	return "";
}

/* 
 * This function creates and setups the XML link topology
 */
void
xml_link_topology()
{
	xmlXPathObjectPtr	obj;

	xmlNodePtr		node;

#if DYNAMIC_LINKS
	xmlNodePtr		status;
#endif

	int			i;
	int			j;
	int			mult;
	int			bw;

	rn_machine		*m;

	/* Links */
	obj = xpath("//link", ctxt);

	bw = 45000000;
	mult = 1500;

#if DHS
	printf("\nSETTING ALL HOST LINK BW: 768KB, DELAY: 0.0001 DOWN and BW 284KB UP \n");
	printf("OVER-SUBSCRIPTION: %d BW per %d users: %lf\n", mult, bw,
		(double) (mult * 768000 / bw));
#endif

	g_rn_nlinks = 0;

	if(0 == obj->nodesetval->nodeNr)
	{
		xmlXPathFreeObject(obj);
		return;
	}

	node = *obj->nodesetval->nodeTab;
	for(i = 0; i < obj->nodesetval->nodeNr; node = obj->nodesetval->nodeTab[++i])
	{
		//printf("Source: %d \n", atoi(xml_getprop(node, "src")));
		m = rn_getmachine(atoi(xml_getprop(node, "src")));
		g_rn_nlinks++;

		//printf("Configuring %d links for machine: %d \n", m->nlinks, m->id);

		for(j = 0; j < m->nlinks; j++)
		{
			if(m->link[j].wire == NULL)
			{
				m->link[j].addr = atoi(xml_getprop(node, "addr")); 
				m->link[j].delay = 0.01 * atof(xml_getprop(node, "delay")); 
				m->link[j].bandwidth = atof(xml_getprop(node, "bandwidth")); 
				m->link[j].cost = atoi(xml_getprop(node, "cost")); 
			
				if(m->link[j].addr < 0 || m->link[j].addr >= g_rn_nmachines)
					tw_error(TW_LOC, "out of range!");

				m->link[j].wire = &g_rn_machines[m->link[j].addr];

				// overwrite parser generated delay / BW, so I don't 
				// have to edit the parser!
#if DHS
				if((m->type == c_host && m->link[j].wire->type == c_ha_router) || 
				   (m->type == c_ha_router && m->link[j].wire->type == c_host))
				{
					m->link[j].delay = 0.0001;
					m->link[j].bandwidth = 10000000;
				}

				if(m->type == c_ha_router && m->link[j].wire->type == c_ct_router)
				{
					m->link[j].delay = 0.001;
					m->link[j].bandwidth = 384000;
				}

				if(m->type == c_ct_router && m->link[j].wire->type == c_ha_router)
				{
					m->link[j].delay = 0.001;
					m->link[j].bandwidth = 768000;
				}

				if((m->type == c_ct_router && m->link[j].wire->type == c_co_router) || 
				   (m->type == c_co_router && m->link[j].wire->type == c_ct_router))
				{
					m->link[j].delay = 0.0005;
					m->link[j].bandwidth = bw;

					if(m->type == c_co_router)
					{
						int multi = m->link[j].wire->nlinks / mult;

						if(multi)
						{
							m->link[j].bandwidth *= multi;
/*
							printf("CO %d to CT %d: bw %lf \n",
								m->uid, m->link[j].wire->uid,
								m->link[j].bandwidth);
*/
						}
					}
					
				}
#endif

#if DYNAMIC_LINKS
				if(0 != strcmp("", xml_getprop(node, "status")))
				{
					m->link[j].status = 
						(strcasecmp("up", xml_getprop(node, "status"))) ? 
							rn_link_down : rn_link_up;
					m->link[j].next_status = INT_MAX;
				} else
				{
					status = node->children->next;

					m->link[j].status = 
						(strcasecmp("up", xml_getprop(status, "start"))) ? 
							rn_link_down : rn_link_up;
					m->link[j].next_status = atoi(xml_getprop(status, "change"));
				}
#endif

				rn_hash_insert(m->hash_link, &m->link[j]);
				//rn_hash_fetch(m->hash_link, m->link[j].addr);

				break;
			}
		}
	}

	xmlXPathFreeObject(obj);
}

void
rn_xml_model()
{
	tw_error(TW_LOC, "Seperate file for model not implemented yet!!");
}

/* 
 * This function creates and setups the g_rn_machines global data structure of nodes
 */
void
rn_xml_topology()
{
	xmlXPathObjectPtr	obj;

	xmlNodePtr		node;

	unsigned int		i;
	//unsigned int		j;
	unsigned int		id;
	unsigned int		size;

	rn_as			*as;
	rn_area			*ar;
	rn_subnet		*sn;
	rn_machine		*m;

	size = 0;

	/* Environment */
	obj = xpath("//environment", ctxt);
	if(obj->nodesetval->nodeTab)
		g_rn_environment = *obj->nodesetval->nodeTab;
	xmlXPathFreeObject(obj);

	/* ASes */
	obj = xpath("/rossnet/as", ctxt);
	g_rn_nas = xmlXPathNodeSetGetLength(obj->nodesetval);	

	if(g_rn_nas > 0)
	{
		g_rn_as = (rn_as *) tw_calloc(TW_LOC, "", sizeof(rn_as), g_rn_nas);
		size += sizeof(rn_as) * g_rn_nas;
	}

	node = obj->nodesetval->nodeTab[0];
	for(i = 0; i < obj->nodesetval->nodeNr; )
	{
		g_rn_as[i].low = -1;
		g_rn_as[i].id = atoi(xml_getprop(node, "id"));	
		//g_rn_as[i].frequency = atoi(xml_getprop(node, "frequency"));	

#if WASTE_MEM
		if(0 == strcmp(xml_getprop(node, "name"), "name"))
			g_rn_as[i].name = (char *) xmlStrdup((xmlChar *) xml_getprop(node, "name"));
#endif

		node = obj->nodesetval->nodeTab[++i];
	}

	xmlXPathFreeObject(obj);

	/* Areas */
	obj = xpath("/rossnet/as/area", ctxt);
	g_rn_nareas = xmlXPathNodeSetGetLength(obj->nodesetval);	

	if(g_rn_nareas > 0)
	{
		g_rn_areas = (rn_area *) tw_calloc(TW_LOC, "", sizeof(rn_area), g_rn_nareas);
		size += sizeof(rn_area) * g_rn_nareas;
	}

	node = obj->nodesetval->nodeTab[0];
	for(i = 0; i < obj->nodesetval->nodeNr; )
	{
		//g_rn_areas[i].root = INT_MAX;
		//g_rn_areas[i].root_lvl = 0xff;
		g_rn_areas[i].low = INT_MAX;
		g_rn_areas[i].id = atoi(xml_getprop(node, "id"));	
		g_rn_areas[i].as = &g_rn_as[atoi(xml_getprop(node->parent, "id"))];

		if(g_rn_areas[i].as->areas == NULL)
			g_rn_areas[i].as->areas = &g_rn_areas[i];

		if(i > 0 && g_rn_areas[i-1].as->id == g_rn_areas[i].as->id)
			g_rn_areas[i-1].next = &g_rn_areas[i];

		g_rn_areas[i].as->nareas++;

		node = obj->nodesetval->nodeTab[++i];
	}

	xmlXPathFreeObject(obj);

	/* Subnets */
	obj = xpath("/rossnet/as/area/subnet", ctxt);
	g_rn_nsubnets = xmlXPathNodeSetGetLength(obj->nodesetval);	

	if(g_rn_nsubnets > 0)
	{
		g_rn_subnets = (rn_subnet *) tw_calloc(TW_LOC, "", sizeof(rn_subnet), g_rn_nsubnets);
		size += sizeof(rn_subnet) * g_rn_nsubnets;
	}

	node = *obj->nodesetval->nodeTab;
	for(i = 0; i < obj->nodesetval->nodeNr; )
	{
		g_rn_subnets[i].low = INT_MAX;
		g_rn_subnets[i].id = atoi(xml_getprop(node, "id"));	
		g_rn_subnets[i].area = &g_rn_areas[atoi(xml_getprop(node->parent, "id"))];

		if(g_rn_subnets[i].area->subnets == NULL)
			g_rn_subnets[i].area->subnets = &g_rn_subnets[i];

		if(i > 0 && g_rn_subnets[i-1].area->id == g_rn_subnets[i].area->id)
			g_rn_subnets[i-1].next = &g_rn_subnets[i];

		g_rn_subnets[i].area->nsubnets++;

		node = obj->nodesetval->nodeTab[++i];
	}

	xmlXPathFreeObject(obj);

	/* Machines */
	obj = xpath("//node", ctxt);
	g_rn_nmachines = xmlXPathNodeSetGetLength(obj->nodesetval);	

	if(g_rn_nmachines)
	{
		g_rn_machines = tw_calloc(TW_LOC, "machines", 
					  sizeof(rn_machine), 
					  g_rn_nmachines);
		size += sizeof(rn_machine) * g_rn_nmachines;
	} else
		tw_error(TW_LOC, "No machines found in XML!");

#if 0
	if(g_tw_mynode == tw_nnodes() - 1)
		g_tw_nlp = obj->nodesetval->nodeNr / tw_nnodes();
	else
#endif
		g_tw_nlp = ceil((double) obj->nodesetval->nodeNr / (double) tw_nnodes());

	node = *obj->nodesetval->nodeTab;

	for(i = 0; i < obj->nodesetval->nodeNr; )
	{
		id = atoi(xml_getprop(node, "id"));

		m = rn_getmachine(id);
		m->xml = node;
		m->conn = -1;
		m->id = id;
		m->nlinks = atoi(xml_getprop(node, "links"));	
		//m->level = atoi(xml_getprop(node, "lvl"));
		m->subnet = &g_rn_subnets[atoi(xml_getprop(node->parent, "id"))];

		if(0 != strcmp("", xml_getprop(node, "uid")))
			m->uid = atoi(xml_getprop(node, "uid"));
		else
			m->uid = -1;

		if(strcmp("c_router", xml_getprop(node, "type")) == 0)
		{
			m->type = c_router;
			g_rn_nrouters++;
		} else if(strcmp("c_ct_router", xml_getprop(node, "type")) == 0)
		{
			m->type = c_ct_router;
			g_rn_nrouters++;
		} else if(strcmp("c_og_router", xml_getprop(node, "type")) == 0)
		{
			m->type = c_og_router;
			g_rn_nrouters++;
		} else if(strcmp("c_co_router", xml_getprop(node, "type")) == 0)
		{
			m->type = c_co_router;
			g_rn_nrouters++;
		} else if(strcmp("c_ha_router", xml_getprop(node, "type")) == 0)
		{
			m->type = c_ha_router;
			g_rn_nrouters++;
		} else if(strcmp("c_host", xml_getprop(node, "type")) == 0)
			m->type = c_host;
		else
			tw_error(TW_LOC, "Unknown node type: %s", xml_getprop(node, "type"));

		if(m->subnet->machines == NULL)
			m->subnet->machines = &g_rn_machines[i];

#if 0
		if(id > 0 && 
		   g_rn_machines[id-1].subnet->id == g_rn_machines[id].subnet->id)
			g_rn_machines[id-1].next = &g_rn_machines[id];
#endif

		m->subnet->nmachines++;
		m->subnet->area->nmachines++;
		m->link = (rn_link *) tw_calloc(TW_LOC, "", sizeof(rn_link) * m->nlinks, 1);
		m->hash_link = rn_hash_create(m->nlinks);

		size += sizeof(rn_link) * m->nlinks;

		//printf("Init\'ing Machines id: %d \n", m->id);

		node = obj->nodesetval->nodeTab[++i];
	}

	xmlXPathFreeObject(obj);

	xml_link_topology();

	/*
	 * Set up the global connection library
	 */
	obj = xpath("//connect", ctxt);

	if(obj->nodesetval->nodeNr)
	{
		node = *obj->nodesetval->nodeTab;

		for(i = 0; i < obj->nodesetval->nodeNr; )
		{
			m = rn_getmachine(atoll(xml_getprop(node, "src")));
			m->conn = atoll(xml_getprop(node, "dst"));

			// back link the connection
			//rn_getmachine(m->conn)->conn = m->id;

			node = obj->nodesetval->nodeTab[++i];
		}
	}

	xmlXPathFreeObject(obj);

	for(i = 0; i < g_rn_nmachines; i++)
	{
		m = rn_getmachine(i);

		sn = m->subnet;
		ar = sn->area;
		as = ar->as;

		if(i < sn->low)
		{
			sn->low = i;

			if(i < ar->low)
			{
				ar->low = i;

				if(0 && i < as->low)
					as->low = i;
			}
		}

		if(i > sn->high)
		{
			sn->high = i;

			if(i > ar->high)
			{
				ar->high = i;

				if(0 && i > as->high)
					as->high = i;
			}
		}

		if(m->type == c_host)
			continue;

		ar->g_ospf_nlsa = ar->nmachines + ar->as->nareas + g_rn_nas;

		if(ar->nmachines == 0)
			tw_error(TW_LOC, "No machines in Area %d!\n", ar->id);

#if 0
		m->ft = (int *) tw_calloc(TW_LOC, "", sizeof(int ) * ar->g_ospf_nlsa, 1);
		size += sizeof(int) * ar->g_ospf_nlsa;

		for(j = 0; j < ar->g_ospf_nlsa; j++)
			m->ft[j] = -1;
#endif

/*
		printf("mach %d: alloc FT of size: %d \n", 
			i, ar->nmachines + as->nareas + g_rn_nas);
*/
	}

	ar = NULL;
	as = NULL;
	while(NULL != (as = rn_nextas(as)))
	{
		while(NULL != (ar = rn_nextarea_onas(ar, as)))
		{
			if(as->low == -1)
				as->low = ar->id;
			as->high = ar->id;
		}

		//printf("AS %d: low %ld, high %ld \n", as->id, as->low, as->high);
	}

#if RN_XML_DEBUG || 1
	if(tw_ismaster())
	{
		printf("\n");
		printf("%-32s %11d \n", "ASes: ", g_rn_nas);
		printf("%-32s %11d \n", "Areas: ", g_rn_nareas);
		printf("%-32s %11d \n", "Subnets: ", g_rn_nsubnets);
		printf("%-32s %11d \n", "Machines: ", g_rn_nmachines);
		printf("%-32s %11d \n", "Links: ", g_rn_nlinks);
		printf("%-32s %11d bytes\n", "Total topology size: ", size);
		printf("\n");
	}
#endif

	//verify_topology();
}
