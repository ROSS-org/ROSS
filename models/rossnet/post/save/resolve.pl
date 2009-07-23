#!/usr/bin/perl

# RN enum: ct = 3, og, co

my (@ct, @co, @og) = ();
my (@co_max, @og_max) = ();
my ($nlinks, $type, $id, $bw, $capacity, $max) = undef;

open(F, "../tools/Chicago2/Chicago2.xml") or die "Unable to open: Chicago2.xml: $!";

print "Reading: Chicago2.xml\n";

while(<F>)
{
	next if $_ !~ /<node/;

	$nlinks = (split(/'/))[3];
	$type = (split(/'/))[5];
	$id = (split(/'/))[9];

	if($type =~ /c_ct_router/)
	{
		$type = 3;
	} elsif($type =~ /c_og_router/)
	{
		$type = 4;
	} elsif($type =~ /c_co_router/)
	{
		$type = 5;
	} else
	{
		next;
	}

	#print "$_\n";
	#print "Type: $type, id: $id, Nlinks: $nlinks\n";

	# parse through links
	$max = 0;
	while(<F>)
	{
		last if $_ =~ /<\/node/;
		next if $_ !~ /<link/;

		$c = (split(/'/))[5];

		$capacity += $c;
		$max = $c if($c > $max);
	}

	#print "Total Capacity: $capacity\n";

	if($type == 3)
	{
		die "Error: duplicate id: $id" if defined $ct[$id];
		die "Error: invalid id: $id" if $id > 2114;

		$ct[$id] = $capacity;
	} elsif($type == 4)
	{
		die "Error: duplicate id: $id" if defined $og[$id];
		die "Error: invalid id: $id" if $id > 18233;

		$og[$id] = $capacity;
		$og_max[$id] = $max;
	} elsif($type == 5)
	{
		die "Error: duplicate id: $id" if defined $co[$id];
		die "Error: invalid id: $id" if $id > 142;

		$co[$id] = $capacity;
		$co_max[$id] = $max;
	} else
	{
		die "Error: invalid type: $type\n";
	}
}

close(F);

exit;

open(F, "ip.log") or die "Unable to open ip.log: $!";
open(OUT, ">ip.new") or die "Unable to open ip.new: $!";

print "Processing: ip.log\n";

while(<F>)
{
	my ($type, $day, $id, $load) = split(/ /);

	$max = 0;

	if($type == 3)
	{
		die "Error: invalid id: $id" if $id > 2114;

		$capacity = $ct[$id];
		$max = 155000000;
	} elsif($type == 4)
	{
		die "Error: invalid id: $id" if $id > 18233;

		$capacity = $og[$id];
		$max = $og_max[$id];
	} elsif($type == 5)
	{
		die "Error: invalid id: $id" if $id > 142;

		$capacity = $co[$id];
		$max = $co_max[$id];
	} else
	{
		die "Error: invalid type: $type\n";
	}

	die "Error: invalid capacity: $capacity" if(!defined $capacity || $capacity <= 0);
	die "Error: invalid max capacity: $max" if(!defined $max || $max <= 0);

	$load *= $capacity;
	$load /= $max;

	print OUT "$type $day $id $load\n";
}
