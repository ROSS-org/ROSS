#include<rp.h>

/*
 * sort two elements in a double array
 */
inline
void
rw_sort(double * p)
{
	double	temp;

	if(p[0] > p[1])
	{
		temp = p[0];
		p[0] = p[1];
		p[1] = temp;
	}
}

/*
 * rw_distance - compute distance between two points
 */
inline
double
rw_distance(double x0, double y0, double x1, double y1)
{
	return (sqrt( pow(y1 - y0, 2) + pow(x1 - x0, 2)));
}

/*
 * rw_intercept - compute intercept between 2 lines
 */
inline
double
rw_intercept(double x, double y, double slope)
{
	return y - x * slope;
}

/*
 * rw_slope - compute the slope of a line
 */
inline
double
rw_slope(double x0, double y0, double x1, double y1)
{
	if(0 == x1 - x0)
		return 0.0;

	return (y1 - y0) / (x1 - x0);
}
