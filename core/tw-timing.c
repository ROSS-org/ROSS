#include <ross.h>

void
tw_wall_now(tw_wtime * t)
{
	if(0 != gettimeofday((struct timeval *)t, NULL))
		tw_error(TW_LOC, "Unable to get time of day!");
}

void
tw_wall_sub(tw_wtime * r, tw_wtime * a, tw_wtime * b)
{
	r->tv_sec = a->tv_sec - b->tv_sec;
	r->tv_usec = a->tv_usec - b->tv_usec;

	if (r->tv_usec < 0)
	{
		r->tv_sec--;
		r->tv_usec += 1000000;
	}
}

double
tw_wall_to_double(tw_wtime * t)
{
	return (double)t->tv_sec + (((double)t->tv_usec) / 1000000);
}
