#!/usr/bin/perl

use UNIVERSAL;

my $nsubnets = $ARGV[0];
die "No subnets!\n" if (!defined $nsubnets || $nsubnets < 1);

my $debug = 0;
my $optimal = 0;
my $rnodes = 82;
my $hcnt = $rnodes * $nsubnets;

# All links to hosts have the same stats
my $hlink_speed = "100000000";
my $hdelay = "0.001";
my $hbuffer_size = "100000000";

open (OUTPUT, ">output") or die "Unable to open output: $!";

print OUTPUT "1\n";
print OUTPUT "1\n";
print OUTPUT "-2\n";
print OUTPUT "100000\n";
print OUTPUT "0\n";
print OUTPUT "960\n";
print OUTPUT "64\n";
print OUTPUT $rnodes * $nsubnets, "\n";
print OUTPUT 1008 * $nsubnets, "\n";


print "Creating $nsubnets subnets:\n";

# Create the routers
for(my $i = 0; $i < $nsubnets; $i++)
{
	&create_routers($i);
}

# Create the hosts 
for(my $i = 0; $i < $nsubnets; $i++)
{
	&create_hosts($i);
}

sub create_routers
{
	my $sn = shift;
	my $start_index = $sn * $rnodes;
	my $r;

	my $link_speed = "2000000000";
	my $delay = "0.001";
	my $buffer_size = "100000000";

	print "Start index: $start_index \n";

	# The zero router must have the correct ring links
	$r = "router " . ($start_index+0) . "\n";
	$r = "router \n" if ($debug == 0);
	$r .= "5\n";

	$r .= "$link_speed $delay " . ($start_index + 1) . " $buffer_size\n";
	$r .= "$link_speed $delay " . ($start_index + 2) . " $buffer_size\n";
	$r .= "$link_speed $delay " . ($start_index + 20) . " $buffer_size\n";

	# First link is the RIGHT link
	if($sn == $nsubnets - 1)
	{
		$r .= "$link_speed 0.2 0 $buffer_size\n";
	} else
	{
		$r .= "$link_speed 0.2 " . (($sn + 1) * $rnodes) . " $buffer_size\n";
	}

	# Second link is the LEFT link
	if($sn == 0)
	{
		$r .= "$link_speed 0.2 " . (($nsubnets - 1) * $rnodes) . " $buffer_size\n";
	} else
	{
		$r .= "$link_speed 0.2 " . (($sn - 1) * $rnodes) . " $buffer_size\n";
	}

	#$r .= "\n";
	print OUTPUT $r;

	$r = "router " . ($start_index+1) . "\n";
	$r = "router \n" if ($debug == 0);
	$r .= "3\n";
	$r .= "$link_speed $delay " . ($start_index + 0) . " $buffer_size\n";
	$r .= "$link_speed $delay " . ($start_index + 2) . " $buffer_size\n";
	$r .= "$link_speed $delay " . ($start_index + 21) . " $buffer_size\n";
	#$r .= "\n";

	print OUTPUT $r;

	# r2
	$r = "router " . ($start_index+2) . "\n";
	$r = "router \n" if ($debug == 0);
	$r .= "3\n";
	$r .= "$link_speed $delay " . ($start_index + 0) . " $buffer_size\n";
	$r .= "$link_speed $delay " . ($start_index + 1) . " $buffer_size\n";
	$r .= "$link_speed $delay " . ($start_index + 3) . " $buffer_size\n";
	#$r .= "\n";

	print OUTPUT $r;

	# r3
	$r = "router " . ($start_index+3) . "\n";
	$r = "router \n" if ($debug == 0);
	$r .= "4\n";
	$r .= "$link_speed $delay " . ($start_index + 2) . " $buffer_size\n";
	$r .= "$link_speed $delay " . ($start_index + 4) . " $buffer_size\n";
	$r .= "$link_speed $delay " . ($start_index + 5) . " $buffer_size\n";
	$r .= "$link_speed $delay " . ($start_index + 6) . " $buffer_size\n";
	#$r .= "\n";

	print OUTPUT $r;

	# r4
	$r = "router " . ($start_index+4) . "\n";
	$r = "router \n" if ($debug == 0);
	$r .= "3\n";
	$r .= "$link_speed $delay " . ($start_index + 3) . " $buffer_size\n";
	$r .= "$link_speed $delay " . ($start_index + 7) . " $buffer_size\n";
	$r .= "$link_speed $delay " . ($start_index + 8) . " $buffer_size\n";
	#$r .= "\n";

	print OUTPUT $r;

	# r5
	$r = "router " . ($start_index+5) . "\n";
	$r = "router \n" if ($debug == 0);
	$r .= "127\n";
	$r .= "$link_speed $delay " . ($start_index + 3) . " $buffer_size\n";

	for(my $j = 0; $j < 126; $j++)
	{
		$r .= "$hlink_speed $hdelay " . $hcnt++ . " $buffer_size\n";
	}

	#$r .= "\n";

	print OUTPUT $r;

	# r6
	$r = "router " . ($start_index+6) . "\n";
	$r = "router \n" if ($debug == 0);
	$r .= "127\n";
	$r .= "$link_speed $delay " . ($start_index + 3) . " $buffer_size\n";

	for(my $j = 0; $j < 126; $j++)
	{
		$r .= "$hlink_speed $hdelay " . $hcnt++ . " $buffer_size\n";
	}

	#$r .= "\n";
	print OUTPUT $r;

	# r7
	$r = "router " . ($start_index+7) . "\n";
	$r = "router \n" if ($debug == 0);
	$r .= "127\n";
	$r .= "$link_speed $delay " . ($start_index + 4) . " $buffer_size\n";

	for(my $j = 0; $j < 126; $j++)
	{
		$r .= "$hlink_speed $hdelay " . $hcnt++ . " $buffer_size\n";
	}

	#$r .= "\n";
	print OUTPUT $r;

	# r8
	$r = "router " . ($start_index+8) . "\n";
	$r = "router \n" if ($debug == 0);
	$r .= "127\n";
	$r .= "$link_speed $delay " . ($start_index + 4) . " $buffer_size\n";

	for(my $j = 0; $j < 126; $j++)
	{
		$r .= "$hlink_speed $hdelay " . $hcnt++ . " $buffer_size\n";
	}

	#$r .= "\n";

	print OUTPUT $r;

	# r9
	$r = "router " . ($start_index+9) . "\n";
	$r = "router \n" if ($debug == 0);
	$r .= "3\n";
	$r .= "$link_speed $delay " . ($start_index + 10) . " $buffer_size\n";
	$r .= "$link_speed $delay " . ($start_index + 11) . " $buffer_size\n";
	$r .= "$link_speed $delay " . ($start_index + 20) . " $buffer_size\n";
	#$r .= "\n";

	print OUTPUT $r;

	# r10
	$r = "router " . ($start_index+10) . "\n";
	$r = "router \n" if ($debug == 0);
	$r .= "3\n";
	$r .= "$link_speed $delay " . ($start_index + 9) . " $buffer_size\n";
	$r .= "$link_speed $delay " . ($start_index + 12) . " $buffer_size\n";
	$r .= "$link_speed $delay " . ($start_index + 20) . " $buffer_size\n";
	#$r .= "\n";

	print OUTPUT $r;

	# r11
	$r = "router " . ($start_index+11) . "\n";
	$r = "router \n" if ($debug == 0);
	$r .= "4\n";
	$r .= "$link_speed $delay " . ($start_index + 9) . " $buffer_size\n";
	$r .= "$link_speed $delay " . ($start_index + 12) . " $buffer_size\n";
	$r .= "$link_speed $delay " . ($start_index + 13) . " $buffer_size\n";
	$r .= "$link_speed $delay " . ($start_index + 52) . " $buffer_size\n";
	#$r .= "\n";

	print OUTPUT $r;

	# r12
	$r = "router " . ($start_index+12) . "\n";
	$r = "router \n" if ($debug == 0);
	$r .= "4\n";
	$r .= "$link_speed $delay " . ($start_index + 10) . " $buffer_size\n";
	$r .= "$link_speed $delay " . ($start_index + 11) . " $buffer_size\n";
	$r .= "$link_speed $delay " . ($start_index + 14) . " $buffer_size\n";
	$r .= "$link_speed $delay " . ($start_index + 32) . " $buffer_size\n";
	#$r .= "\n";

	print OUTPUT $r;

	# r13
	$r = "router " . ($start_index+13) . "\n";
	$r = "router \n" if ($debug == 0);
	$r .= "2\n";
	$r .= "$link_speed $delay " . ($start_index + 11) . " $buffer_size\n";
	$r .= "$link_speed $delay " . ($start_index + 47) . " $buffer_size\n";
	#$r .= "\n";

	print OUTPUT $r;

	# r14
	$r = "router " . ($start_index+14) . "\n";
	$r = "router \n" if ($debug == 0);
	$r .= "3\n";
	$r .= "$link_speed $delay " . ($start_index + 12) . " $buffer_size\n";
	$r .= "$link_speed $delay " . ($start_index + 15) . " $buffer_size\n";
	$r .= "$link_speed $delay " . ($start_index + 42) . " $buffer_size\n";
	#$r .= "\n";

	print OUTPUT $r;

	# r15
	$r = "router " . ($start_index+15) . "\n";
	$r = "router \n" if ($debug == 0);
	$r .= "4\n";
	$r .= "$link_speed $delay " . ($start_index + 14) . " $buffer_size\n";
	$r .= "$link_speed $delay " . ($start_index + 22) . " $buffer_size\n";
	$r .= "$link_speed $delay " . ($start_index + 27) . " $buffer_size\n";
	$r .= "$link_speed $delay " . ($start_index + 37) . " $buffer_size\n";
	#$r .= "\n";

	print OUTPUT $r;

	# r16
	$r = "router " . ($start_index+16) . "\n";
	$r = "router \n" if ($debug == 0);
	$r .= "4\n";
	$r .= "$link_speed $delay " . ($start_index + 17) . " $buffer_size\n";
	$r .= "$link_speed $delay " . ($start_index + 21) . " $buffer_size\n";
	$r .= "$link_speed $delay " . ($start_index + 62) . " $buffer_size\n";
	$r .= "$link_speed $delay " . ($start_index + 67) . " $buffer_size\n";
	#$r .= "\n";

	print OUTPUT $r;

	# r17
	$r = "router " . ($start_index+17) . "\n";
	$r = "router \n" if ($debug == 0);
	$r .= "4\n";
	$r .= "$link_speed $delay " . ($start_index + 16) . " $buffer_size\n";
	$r .= "$link_speed $delay " . ($start_index + 18) . " $buffer_size\n";
	$r .= "$link_speed $delay " . ($start_index + 19) . " $buffer_size\n";
	$r .= "$link_speed $delay " . ($start_index + 21) . " $buffer_size\n";
	#$r .= "\n";

	print OUTPUT $r;

	# r18
	$r = "router " . ($start_index+18) . "\n";
	$r = "router \n" if ($debug == 0);
	$r .= "3\n";
	$r .= "$link_speed $delay " . ($start_index + 17) . " $buffer_size\n";
	$r .= "$link_speed $delay " . ($start_index + 19) . " $buffer_size\n";
	$r .= "$link_speed $delay " . ($start_index + 57) . " $buffer_size\n";
	#$r .= "\n";

	print OUTPUT $r;

	# r19
	$r = "router " . ($start_index+19) . "\n";
	$r = "router \n" if ($debug == 0);
	$r .= "4\n";
	$r .= "$link_speed $delay " . ($start_index + 17) . " $buffer_size\n";
	$r .= "$link_speed $delay " . ($start_index + 18) . " $buffer_size\n";
	$r .= "$link_speed $delay " . ($start_index + 72) . " $buffer_size\n";
	$r .= "$link_speed $delay " . ($start_index + 77) . " $buffer_size\n";
	#$r .= "\n";

	print OUTPUT $r;

	# r20
	$r = "router " . ($start_index+20) . "\n";
	$r = "router \n" if ($debug == 0);
	$r .= "4\n";
	$r .= "$link_speed $delay " . ($start_index + 0) . " $buffer_size\n";
	$r .= "$link_speed $delay " . ($start_index + 9) . " $buffer_size\n";
	$r .= "$link_speed $delay " . ($start_index + 10) . " $buffer_size\n";
	$r .= "$link_speed $delay " . ($start_index + 21) . " $buffer_size\n";
	#$r .= "\n";

	print OUTPUT $r;

	# r21
	$r = "router " . ($start_index+21) . "\n";
	$r = "router \n" if ($debug == 0);
	$r .= "4\n";
	$r .= "$link_speed $delay " . ($start_index + 1) . " $buffer_size\n";
	$r .= "$link_speed $delay " . ($start_index + 16) . " $buffer_size\n";
	$r .= "$link_speed $delay " . ($start_index + 17) . " $buffer_size\n";
	$r .= "$link_speed $delay " . ($start_index + 20) . " $buffer_size\n";
	#$r .= "\n";

	print OUTPUT $r;

	&create_lans($sn);
}

sub create_lans
{
	my $sn = shift;
	my $start = 22;
	my $back;
	my $r;

	my $link_speed = "1000000000";
	my $delay ="0.005";
	my $buffer_size = "100000000";
	$start = $start + ($sn * $rnodes);

	#my @start_nodes = qw(11 12 13 14 15 15 15 16 16 18 19 19);
	my @start_nodes = qw(15 15 12 15 14 13 11 18 16 16 19 19);
	foreach $back (@start_nodes)
	{
		$b = $back + ($sn * $rnodes);

		$r = "router " . ($start) . "\n";
		$r = "router \n" if ($debug == 0);
		$r .= "5\n";
		$r .= "$link_speed $delay " . ($b) . " $buffer_size\n";
		$r .= "$link_speed $delay " . ($start + 1) . " $buffer_size\n";
		$r .= "$link_speed $delay " . ($start + 2) . " $buffer_size\n";
		$r .= "$link_speed $delay " . ($start + 3) . " $buffer_size\n";
		$r .= "$link_speed $delay " . ($start + 4) . " $buffer_size\n";
		#$r .= "\n";

		print OUTPUT $r;

		for(my $j = 0; $j < 4; $j++, $start_host += 10)
		{
			$r = "router " . ($start + $j + 1) . "\n";
			$r = "router \n" if ($debug == 0);

			$r .= "11\n" if($j != 3);
			$r .= "13\n" if($j == 3);

			$r .= "$hlink_speed $hdelay " . ($start) . " $hbuffer_size\n";
			$r .= "$hlink_speed $hdelay " . ($hcnt++) . " $hbuffer_size\n";
			$r .= "$hlink_speed $hdelay " . ($hcnt++) . " $hbuffer_size\n";
			$r .= "$hlink_speed $hdelay " . ($hcnt++) . " $hbuffer_size\n";
			$r .= "$hlink_speed $hdelay " . ($hcnt++) . " $hbuffer_size\n";
			$r .= "$hlink_speed $hdelay " . ($hcnt++) . " $hbuffer_size\n";
			$r .= "$hlink_speed $hdelay " . ($hcnt++) . " $hbuffer_size\n";
			$r .= "$hlink_speed $hdelay " . ($hcnt++) . " $hbuffer_size\n";
			$r .= "$hlink_speed $hdelay " . ($hcnt++) . " $hbuffer_size\n";
			$r .= "$hlink_speed $hdelay " . ($hcnt++) . " $hbuffer_size\n";
			$r .= "$hlink_speed $hdelay " . ($hcnt++) . " $hbuffer_size\n";

			if($j == 3)
			{
				$r .= "$hlink_speed $hdelay " . ($hcnt++) . " $hbuffer_size\n";
				$r .= "$hlink_speed $hdelay " . ($hcnt++) . " $hbuffer_size\n";
			}

			#$r .= "\n";

			print OUTPUT $r;
		}

		$start += 5;
	}
}

sub create_hosts
{
	my $sn = shift;
	my $start = 22 + ($sn * $rnodes);
	my $host = $nsubnets * $rnodes + ($sn * 1008);

	#my @start_nodes = qw(11 12 13 14 15 15 15 16 16 18 19 19);
	my @start_nodes = qw(15 15 12 15 14 13 11 18 16 16 19 19);
	print "Starting host id: $host, subnet $sn\n";

	$h = ($nsubnets*$rnodes) + 504 + (1008 * ($sn-1)) if ($sn != 0);

	$h = ($nsubnets*$rnodes) + 504 + (1008 * ($nsubnets-1)) if ($sn == 0);

	# For Optimal Performance
	$h = ($nsubnets*$rnodes) + 504 + (1008 * ($sn)) if ($optimal == 1);

	# TCP Servers
	for(my $i = 5; $i < 9; $i++)
	{
		for(my $j = 0; $j < 126; $j++)
		{
			$r = "host " . ($host++) . "\n";
			$r = "host \n" if ($debug == 0);

			$rtr = ($sn * $rnodes) + $i;
			$r .= "$hlink_speed $hdelay $rtr direct " . $h++ . " 268000\n";

			print OUTPUT $r;
		}
	}

	# TCP clients
	foreach $back (@start_nodes)
	{
		$back = $back + (($sn+1) * $rnodes);
		for(my $j = 0; $j < 4; $j++, $start++)
		{
			for(my $k = 0; $k < 10; $k++)
			{
				$r = "host " . ($host++) . "\n";
				$r = "host \n" if($debug == 0);

				$rtr = $start + 1;
				$r .= "$hlink_speed $hdelay $rtr server\n";

				print OUTPUT $r;
			}

			# Print two extra hosts
			if($j == 3)
			{
				$r = "host " . ($host++) . "\n";
				$r = "host \n" if($debug == 0);

				$rtr = ($start + 1);
				$r .= "$hlink_speed $hdelay $rtr server\n";

				print OUTPUT $r;

				$r = "host " . ($host++) . "\n";
				$r = "host \n" if($debug == 0);

				$rtr = ($start + 1);
				$r .= "$hlink_speed $hdelay $rtr server\n";

				print OUTPUT $r;
			}
		}

		# Skip top level router in PoP
		$start++;
	}
}
