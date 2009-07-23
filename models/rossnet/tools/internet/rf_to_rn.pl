#!/usr/bin/perl

use Node;
use UNIVERSAL qw(isa);
use FileHandle;

my %ext_nodes = ();
my %aliases = ();
my %ases = ();
my @subnets = ();
my %areas = ();

my ($as_name, $nlinks, $nnodes, $n_tot_nodes, $namb, $ndisc, $n_ext) = 0;

my @cch = `find . -name "*.cch" -print`;
my @al = `find . -name "*.al" -print`;

#
# Get all of the aliases
#
&read_aliases();

$nnodes = 0;
foreach $file (@cch)
{
	chomp $file;
	$file =~ m/.*\/(.*)\.cch/;
	$as_name = $1;

	print "Processing AS: $as_name ($file)\n";
	&read_cch($file);
}

#
# Link nodes to nodes.. all RF external links are to unknown (not in alias
# file) external routers.
#
&link_external_nodes();
&enumerate_network();
&print_network();
#&print_non_network();

sub print_non_network
{
	my $n, $fh;

	$fh = new FileHandle "non-internet.xml", ">";
	print "Printing Non-Network...";
	foreach $n (values %aliases)
	{
		$n->print($fh) if($n->{in_network} == 0);
	}
	print " done!\n";
}

sub enumerate_network
{
	my %as = ();
	my %ar = ();
	my %sn_h = ();
	my @sn = ();

	my ($k0, $k1, $k2);

	my $asid = 0;
	my $arid = 0;
	my $snid = 0;
	my $cnt = 0;
	my $bgp_routes = 0;

	print "Enumerating Network...";

	foreach $k0 (keys %ases)
	{
		%ar = %{$ases{$k0}};

		foreach $k1 (keys %ar)
		{
			%sn_h = %{$areas{$k1}};

			foreach $k2 (keys %sn_h)
			{
				@sn = @{$subnets{$k2}};

				foreach $n (@sn)
				{
					$n->{nid} = $cnt++;
				}
			}
		}
	}

	print " done! Got $cnt nodes\n";
}

sub print_network
{
	my %as = ();
	my %ar = ();
	my %sn_h = ();
	my @sn = ();

	my ($k0, $k1, $k2);

	my $asid = 0;
	my $arid = 0;
	my $snid = 0;
	my $cnt = 0;
	my $bgp_routes = 0;

	$fh = new FileHandle "internet.xml", ">";
	print "Printing Network...";

	$fh->printf("<rossnet>\n");

	foreach $k0 (keys %ases)
	{
		%ar = %{$ases{$k0}};
		$fh->printf("\t<as id='%d' name='$k0' frequency='1'>\n", $asid++);

		foreach $k1 (keys %ar)
		{
			%sn_h = %{$areas{$k1}};
			$fh->printf("\t\t<area id='%d' name='%s'>\n", $arid++, $k1);

			foreach $k2 (keys %sn_h)
			{
				@sn = @{$subnets{$k2}};
				$fh->printf("\t\t\t<subnet id='%d' name='%s'>\n",
						$snid++, $k2);

				foreach $n (@sn)
				{
					$n->print($fh);
					$bgp_routes += $n->{n_ext};

					$cnt++ if $n->{name} ne "" or $n->{name} != undef;
				}

				$fh->printf("\t\t\t</subnet>\n");
			}

			$fh->printf("\t\t</area>\n");
		}

		$fh->printf("\t</as>\n");
	}

	$fh->printf("</rossnet>\n");
	print " done! Got $cnt nodes, $bgp_routes eBGP links\n";
}

sub read_cch
{
	my $f = shift;
	my $nl, $ne, $cnt, $lvl;

	my %as = $ases{$as_name};

	open (F, "$f") or die "Unable to to open $f: $!";

	foreach (<F>)
	{
		my ($i, $n, $next);
		my @attr = ();

		$_ =~ s/\(//g;
		$_ =~ s/\)//g;
		$_ =~ s/{//g;
		$_ =~ s/[<|>|=|!]//g;
		$_ =~ s/[&|}]//g;

		@_ = split(/\s+/, $_);

		my $id = shift @_;

		next if $id =~ /^#/;

		@attr = split(/\s+/, $_);

		#
		# Links to external (outside my AS) routers.  
		# May have ext router, may not.
		#
		if($id =~ /^-/)
		{
			$n = $aliases{$attr[1]};
			next if($n == undef);

			$n->{dns} = shift @_;
			$lvl = shift @_;
			$lvl =~ s/r//g;

			$n->{level} = $lvl;

			$ext_nodes{"$as_name-$id"} = $n;
			$n_ext++;

			next;
		}

		#print "my ip: ", $attr[$#attr-1], "\n";
		$n = $aliases{$attr[$#attr-1]};

		if(!defined $n)
		{
			print STDERR "Could not find node: $attr[$#attr-1] (@attr) \n";
			exit(0);
		}

		$ext_nodes{"$as_name-$id"} = $n;
	
		$n->name(shift @_);

		$status = $n->connected();
	
		if($status eq "ambiguous")
		{
			$namb++;
		} elsif($status eq "false")
		{
			$ndisc++;
		}
	
		$nl = shift @_;
	
		if($nl eq "+")
		{
			$n->{plus} = 1;
			$nl = shift @_;
		}
	
		if($nl eq "bb")
		{
			$n->{bb} = 1;
			$nl = shift @_;
		}

		$ne = shift @_;
		$ne = 0 if($ne =~ /^-/);
	
		$i = $cnt = 0;
		while($i < $nl)
		{
			$next = shift @_;
			next if $next =~ /^-/;

			$i++;
			next if defined $n->{links}{"$as_name-$next"};
			$cnt++;

			$n->{links}{"$as_name-$next"} = 1;
		}

		$nlinks += $cnt;
		$n->{nlinks} += $cnt;

		$i = $cnt = 0;
		while($i < $ne)
		{
			$i++;
			$yy = $as_name . "-" . shift @_;

			next if($n->{ext}{$yy} == 1);
			$cnt++;

			$n->{ext}{$yy} = 1;
		}

		$nlinks += $cnt;
		$n->{n_ext} += $cnt;

		$n->{dns} = shift @_;
		$n->{level} = shift @_;
		$n->{as} = $as_name;
		$nnodes++;

		&create_network($n);
	}
}

#
# Subnets have nodes..
# Areas have subnets..
# ASes have areas..
#
sub create_network()
{
	my $n = shift;
	my ($sn, $ar, $as);

	return if($n->{in_network} == 1);

	$as = $n->{as};

	# get the subnet addr and put node into subnet
	$n->{ip} =~ m/(\d+\.\d+\.\d+)/;
	$sn = $as . '-' . $1;
	push @{$subnets{$sn}}, $n;

	# get the area addr and put the subnet into the area
	$n->{ip} =~ m/(\d+\.\d+)/;
	$ar = $as . '-' . $1;
	$areas{$ar}{$sn} = $subnets{$sn};

	# put the area into the AS
	$ases{$n->{as}}{$ar} = $areas{$ar};

	$n->{in_network} = 1;
}

#
# Link external and internal link numbers to IP addrs
#
sub link_external_nodes
{
	my ($n, $d, $i, $cnt) = (undef, undef, 0, 0);

	print "Linking network...";

	foreach $n (values %aliases)
	{
		next if !defined $n;

		if($n->{n_ext} > 0)
		{
			foreach $k (keys %{$n->{ext}})
			{
				next if isa($k, 'Node');

				$d = $ext_nodes{$k};
				if(!defined $d || !defined $d->{name})
				{
					delete $n->{ext}{$k};
					--$n->{n_ext};
				} else
				{
print "1: $d->{ip} \n" if($n->{ip} eq "139.130.161.140");
					$n->{ext}{$k} = $d;

					if(!defined $d->{as})
					{
print "2: $d->{ip} \n" if($n->{ip} eq "139.130.161.140");
						$d->{ip} =~ m/(\d+)/;
						$d->{as} = $1;
						$d->{nlinks} = 0;
					}

					if(!defined $d->{ext}{"$n->{as}-$n->{id}"})
					{
print "adding: $d->{ip} \n" if($n->{ip} eq "139.130.161.140");
						$d->{ext}{"$n->{as}-$n->{id}"} = $n;
					  	$d->{n_ext}++;
						$d->{ext2}{$n->{gid}} = $n;
					  	$d->{n_ext2}++;
					}

					&create_network($d);

					# 255, since adding the link is +1
					if(!defined $n->{ext2}{$d->{gid}} &&
					   ($n->{nlinks2} + $n->{n_ext2}) < 255)
					{
						$n->{ext2}{$d->{gid}} = $d;
						$n->{n_ext2}++;
					}
				}
			}
		}

		if($n->{nlinks} > 0)
		{
			foreach $k (keys %{$n->{links}})
			{
				$d = $ext_nodes{$k};
				$n->{links}{$k} = $d;

				# 255, since adding the link is +1
				if(!defined $n->{links2}{$d->{gid}} &&
				   ($n->{nlinks2} + $n->{n_ext2}) < 255)
				{
					$n->{links2}{$d->{gid}} = $d;
					$n->{nlinks2}++;
				}
			}
		}

		$cnt++;
		#print "Done linking node: $n->{ip} \n" if ($cnt % 1000 == 0);
	}

	print " done!\n";
}

print STDERR "#in topology, saw $nlinks links, $nnodes of $n_tot_nodes nodes: $namb ambiguous, $ndisc disconnected\n";

sub read_aliases
{
        my $f, $l, $uid, $ip, $dns, $last;

	chomp @al;
	$n_tot_nodes = 0;

        foreach $f (@al)
        {
                open (F, "$f") or die "Could not open file: $f: $!";
		print "Processing alias: $f \n";

		$last = -1;
                foreach $l (<F>)
                {
			($uid, $ip, $dns) = split(/\s+/, $l);

			if($uid != $last)
			{
				$n = new Node($uid);
				$n->{gid} = $n_tot_nodes++;
			}

			$last = $uid;
			$aliases{$ip} = $aliases{$dns} = $n;
			$n->{ip} = $ip;
                }

                close (F);
        }
}
