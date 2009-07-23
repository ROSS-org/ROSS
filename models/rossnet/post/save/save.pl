#!/usr/bin/perl

# RN enum: ct = 3, og, co

my (@ct, @co, @og) = ();

open(F, "../tools/Chicago2/Chicago2.xml") or die "Unable to open: Chicago2.xml: $!";

print "Reading: Chicago2.xml\n";

while(<F>)
{
	next if $_ !~ /<node/;

	my ($type, $id, $nlinks) = split(/ /);

	if($type == 3)
	{
		die "Error: duplicate id: $id" if defined $ct[$id];
		die "Error: invalid id: $id" if $id > 2114;

		$ct[$id] = $nlinks;
	} elsif($type == 4)
	{
		die "Error: duplicate id: $id" if defined $og[$id];
		die "Error: invalid id: $id" if $id > 18233;

		$og[$id] = $nlinks;
	} elsif($type == 5)
	{
		die "Error: duplicate id: $id" if defined $co[$id];
		die "Error: invalid id: $id" if $id > 142;

		$co[$id] = $nlinks;
	} else
	{
		die "Error: invalid type: $type\n";
	}
}

close(F);

open(F, "ip.log") or die "Unable to open ip.log: $!";
open(OUT, ">ip.new") or die "Unable to open ip.new: $!";

print "Processing: ip.log\n";

while(<F>)
{
	my ($type, $day, $id, $load) = split(/ /);

	if($type == 3)
	{
		die "Error: invalid id: $id" if $id > 2114;

		$load *= ( ($ct[$id] - 1) * 768000 ) + 155000000;
	
		print OUT "$type $day $id $load\n";
	} elsif($type == 4)
	{
		die "Error: invalid id: $id" if $id > 18233;
	} elsif($type == 5)
	{
		die "Error: invalid id: $id" if $id > 142;
	} else
	{
		die "Error: invalid type: $type\n";
	}
}
