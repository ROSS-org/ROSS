#ifndef INC_ross_rc_api_h
#define INC_ross_rc_api_h

INLINE(void)
tw_rc_swap_uint(unsigned int * a, unsigned int * b)
{
	unsigned int temp;

	temp = *a;
	*a = *b;
	*b = temp;
}

#endif
