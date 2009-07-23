#!/bin/sh

NET=att
RHOME=/u/bauerd/work/projects/rossnet

LOG=tests
mkdir -p $LOG

#H=`hostname`
H=1
D=`find $LOG -name expnum -print`
if [ x$D = x ] ; then
	D=0
else
	D=`cat $LOG/expnum`;
fi
D=`expr $D + 1`
echo $D > $LOG/expnum

#echo hello inact 1 ack mtu route > $LOG/ospf$D.conf
echo hello inact 1 .001 1500 .001 > $LOG/ospf$D.conf

DIR=`pwd`

echo About to run rossnet  >> out
$RHOME/rossnet/rn -c $RHOME/tools/$NET/$NET.xml -r $RHOME/tools/$NET/$NET.rt -l $RHOME/tools/$NET/links.xml -t 100 -z $LOG/ospf$D.conf > $LOG/ospf$D.log 2>&1 

R=`cat $LOG/ospf$D.log | grep "Total Network Converge" | cut '-d ' -f5`
echo Result=$R   >> out
cat $LOG/ospf$D.log | grep "Total Network Converge" | cut '-d ' -f5
