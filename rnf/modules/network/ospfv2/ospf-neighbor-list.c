#include <ospf.h>

void
ospf_list_free(char * list, unsigned int * size)
{
	int	i;

	for(i = 0; i < *size; i++)
		list[i] = 0;

	*size = 0;
}

void
ospf_list_unlink(char * list, int index, unsigned int * size)
{
	list[index] = 0;
	(*size)--;
}

int
ospf_list_contains(char * list, int index)
{
	if(list[index] == 1)
		return TW_TRUE;

	return TW_FALSE;
}

void
ospf_list_link(char * list, int index, unsigned int * size)
{
	list[index] = 1;
	(*size)++;
}
