#include <num.h>

void
num_xml(num_state * state, const xmlNodePtr node, tw_lp * lp)
{
	state->stats = tw_calloc(TW_LOC, "", sizeof(num_statistics), 1);
	state->type = atoi(xml_getprop(node, "type"));

	g_num_profile_cnts[state->type]++;
}
