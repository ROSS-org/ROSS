#!/bin/awk

BEGIN {
	d_i=1;
	infected_tminus1 = 0;
	a = b = c = d = e = f = 0.0;
	pop = 10000.0;
} 

{
	# print out the daily totals
	if($1 != d_i)
	{
		if(infected_tminus1)
		{
			#print "inf:", infected_tminus1, " exp:", f;
			#print "R(" d_i "):", (f / infected_tminus1 * 3.5);
			print d_i, (f / infected_tminus1 * 3.5);
		}

		infected_tminus1 = c;

		a = b = c = d = e = f = 0.0;
		d_i = $1;
	}

	a += $9;
	b += $10;
	c += $11;
	d += $12;
	e += $13;
	f += $14;
}
