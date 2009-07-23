#!/bin/awk

BEGIN {
	d_i=1;
	a = b = c = d = e = 0.0;
	pop = 10000.0;

	print "0 100 0 0 0 0";
} 

{
	# print out the daily totals
	if($1 != d_i)
	{
		a = a / pop * 100;
		b = b / pop * 100;
		c = c / pop * 100;
		d = d / pop * 100;
		e = e / pop * 100;

		print d_i, a, b, c, d, e;
		a = b = c = d = e = 0.0;
		d_i = $1;
	}

	a += $10;
	b += $11;
	c += $12;
	d += $13;
	e += $14;
}
