#include <tcp.h>

void
tcp_xml(tcp_state * state, const xmlNodePtr node, tw_lp * lp)
{
	xmlNodePtr	 e;

	for(e = node->children; e; e = e->next)
	{
		if(0 == strcmp((char *) e->name, "mss"))
			state->mss = atoi((char *)e->children->content);

		if(0 == strcmp((char *) e->name, "recv_wnd"))
			state->recv_wnd = atoi((char *)e->children->content);
	}

	if(!state->mss)
		state->mss = g_tcp_mss;

	if(!state->recv_wnd)
		state->recv_wnd = g_tcp_rwd;
}
