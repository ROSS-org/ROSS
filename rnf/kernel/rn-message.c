#include <rossnet.h>

void
rn_message_setsize(rn_message * m, unsigned long size)
{
	m->size = size;
}

unsigned long
rn_message_getsize(rn_message * m)
{
	return m->size;
}

void
rn_message_setdirection(rn_message * m, rn_message_type direction)
{
	tw_error(TW_LOC, "This is no longer needed!!!");
	m->type = direction;
}

int
rn_message_getdirection(rn_message * m)
{
	return m->type;
}
