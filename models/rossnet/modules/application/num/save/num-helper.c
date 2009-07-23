#include <num.h>

static char tod[1024];

char *
get_tod(tw_stime ts)
{
 	int time = (int) ts % TWENTY_FOUR_HOURS;
	int d = (int) ts / TWENTY_FOUR_HOURS;;
	int h = time / 3600;
	int m = (time % 3600) / 60;
	double s = ts - (d * 86400) - (h * 3600) - (m * 60);
	
	sprintf(tod, "D%d %02d:%02d:%02.4lf", d, h, m, s);

	return tod;
}
