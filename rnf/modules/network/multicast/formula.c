/* -*-  Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1999  International Computer Science Institute
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by ACIRI, the AT&T 
 *      Center for Internet Research at ICSI (the International Computer
 *      Science Institute).
 * 4. Neither the name of ACIRI nor of ICSI may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ICSI AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL ICSI OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <stdio.h>
#include <math.h> 
#include "formula.h"

double p_to_b(double p, double rtt, double tzero, int psize, int bval) 
{
	double tmp1, tmp2, res;

	if (p < 0 || rtt < 0) {
		return MAXRATE ; 
	}
	res=rtt*sqrt(2*bval*p/3);
	tmp1=3*sqrt(3*bval*p/8);
	if (tmp1>1.0) tmp1=1.0;
	tmp2=tzero*p*(1+32*p*p);
	res+=tmp1*tmp2;
	if (res < SAMLLFLOAT) {
		res=MAXRATE;
	} else {
		res=psize/res;
	}
	if (res > MAXRATE) {
		res = MAXRATE ; 
	}
	return res;
}

double b_to_p(double b, double rtt, double tzero, int psize, int bval) 
{
	double p, pi, bres;
	int ctr=0;
	p=0.5;pi=0.25;
	while(1) {
		bres=p_to_b(p,rtt,tzero,psize, bval);
		/*
		 * if we're within 5% of the correct value from below, this is OK
		 * for this purpose.
		 */
		if ((bres>0.95*b)&&(bres<1.05*b)) 
			return p;
		if (bres>b) {
			p+=pi;
		} else {
			p-=pi;
		}
		pi/=2.0;
		ctr++;
		if (ctr>30) {
			return p;
		}
	}
}

double simple_p_to_b(double p, double rtt, int psize) 
{
	return sqrt(3.0/2.0) * psize / rtt / sqrt(p);
}

double simple_b_to_p(double b, double rtt, int psize) 
{
	return (sqrt(3.0/2.0) * psize / rtt / b) * (sqrt(3.0/2.0) * psize / rtt / b);
}
