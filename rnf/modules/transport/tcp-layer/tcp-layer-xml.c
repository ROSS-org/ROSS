#include <tcp-layer.h>

void
tcp_layer_xml(tw_lp * lp, xmlNodePtr n)
{
	xmlXPathContextPtr	ctxt;
	xmlXPathObjectPtr	obj;
	xmlNodePtr		layer;
	static int              init = 0;
	xmlNodePtr		node;

	for(layer = n; layer; layer = layer->next)
	{
		if(0 != strcmp((char *) layer->name, "layer"))
			continue;

		if(strcmp(xml_getprop(layer, "name"), "tcp") == 0)
		{
			tw_lp_settype(lp, TCP_LAYER_LP_TYPE);
			//printf("Setting LP %d to TCP! \n", lp->id);

			break;
		}
	}

	ctxt = xmlXPathNewContext(document_network);
	ctxt->node = xmlDocGetRootElement(document_network);
	
	if(!init){
	  init = 1;
	  obj = xpath("//mss", ctxt);
	  
	  if(obj && obj->nodesetval->nodeNr)
	    {
	    node = *obj->nodesetval->nodeTab;
	    g_tcp_layer_mss = atoi((char *) node->children->content);
	    }
	  
	  xmlXPathFreeObject(obj);
	  
	  obj = xpath("//recv_wnd", ctxt);
	  
	  if(obj && obj->nodesetval->nodeNr)
	  {
	    node = *obj->nodesetval->nodeTab;
	    g_tcp_layer_recv_wnd = atoi((char *) node->children->content);
	  }
	  
	  xmlXPathFreeObject(obj);
	}
	//tw_mutex_unlock(&g_tw_debug_lck);
}
