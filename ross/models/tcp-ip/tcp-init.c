#include <tcp.h>

void
tcp_init(tcp_state * state, FILE * f, tw_lp * lp)
{
#if 0
	for (layer = node; layer != NULL; layer = layer->next)
	{
		if (0 != strcmp((char *)layer->name, "layer"))
			continue;

		if (strcmp(xml_getprop(layer, "name"), "tcp") == 0)
		{
			tw_lp_settype(lp, TCP_LP_TYPE);
			break;
		}
	}

	for(e = node->children; e; e = e->next)
	{
		if(0 == strcmp((char *) e->name, "mss"))
			state->mss = atoi((char *)e->children->content);

		if(0 == strcmp((char *) e->name, "recv_wnd"))
			state->recv_wnd = atoi((char *)e->children->content);
	}
#endif
}
