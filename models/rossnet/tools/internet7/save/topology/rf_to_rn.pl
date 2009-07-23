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
&make_broadcast_network();
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

# Fully connect nodes in same subnet
# Make default router per area which each ABR connects to
sub make_broadcast_network
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
	my @abrs = ();
	my $last;

	print "Making Broadcast Network...";

	foreach $k0 (keys %ases)
	{
		%ar = %{$ases{$k0}};
		@abrs = ();

		$first = $last = undef;
		foreach $k1 (keys %ar)
		{
			%sn_h = %{$areas{$k1}};

			foreach $k2 (keys %sn_h)
			{
				@sn = @{$subnets{$k2}};

				if(defined $last)
				{
					$n1 = $last;
					$n2 = $sn[0];
					if(!defined $n1->{links2}{$n2->{gid}})
					{
						$n1->{links2}{$n2->{gid}} = $n2;
						$n1->{nlinks2}++;
			
						if(!defined $n2->{links2}{$n1->{gid}})
						{
							$n2->{links2}{$n1->{gid}} = $n1;
							$n2->{nlinks2}++;
						}
					}
				}
				# Subnets must be fully connected
				foreach $n1 (@sn)
				{
					#printf("Connecting node: $n1->{gid} (%d)\n", $#sn);
					foreach $n2 (@sn)
					{
						next if ($n1 == $n2);
			
						#print "\t$n2->{gid} \n";
						if(!defined $n1->{links2}{$n2->{gid}})
						{
							$n1->{links2}{$n2->{gid}} = $n2;
							$n1->{nlinks2}++;
				
							if(!defined $n2->{links2}{$n1->{gid}})
							{
								$n2->{links2}{$n1->{gid}} = $n1;
								$n2->{nlinks2}++;
							}
						}
					}

					foreach $n2 (values %{$n1->{links2}})
					{
						if(0 && $n1->{area} ne $n2->{area})
						{
							push @abrs, $n1;
							last;
						}
					}

					$last = $n1;
				}
			}
		}

			# ABRs must be fully connected
			foreach $n1 (@abrs)
			{
				#printf("Connecting node: $n1->{gid} in area $n1->{area}\n");
				foreach $n2 (@abrs)
				{
					next if ($n1 == $n2);
					if(!defined $n1->{links2}{$n2->{gid}})
					{
						#print "\t$n2->{gid} in area $n2->{area}\n";
						$n1->{links2}{$n2->{gid}} = $n2;
						$n1->{nlinks2}++;
			
						if(!defined $n2->{links2}{$n1->{gid}})
						{
							$n2->{links2}{$n1->{gid}} = $n1;
							$n2->{nlinks2}++;
						}
					}
				}
			}
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
					$n->{area} = $k1;

					if($n->{n_ext2} > 0)
					{
						push @{$ases{$k0}{ibgp}}, $n->{nid};
					}
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
		next if(() == keys %ar);

		$fh->printf("\t<as id='%d' name='$k0' frequency='1'>\n", $asid++);
		#$fh->printf("\t<as id='%d' frequency='1'>\n", $asid++);

		foreach $k1 (keys %ar)
		{
			%sn_h = %{$areas{$k1}};

			next if(() == keys %sn_h);

			#$fh->printf("\t\t<area id='%d' name='%s'>\n", $arid++, $k1);
			$fh->printf("\t\t<area id='%d'>\n", $arid++, $k1);

			foreach $k2 (keys %sn_h)
			{
				@sn = @{$subnets{$k2}};
				#$fh->printf("\t\t\t<subnet id='%d' name='%s'>\n", $snid++, $k2);
				$fh->printf("\t\t\t<subnet id='%d'>\n", $snid++, $k2);

				foreach $n (@sn)
				{
					$n->print($fh, @{$ases{$k0}{ibgp}});
					#$n->print($fh);
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
	my $sn_n;

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

	# Max out-degree per node
	my $max = 5000;

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
					$n->{ext}{$k} = $d;

					next if($n->{gid} == $d->{gid});
					#next if(defined $n->{links2}{$d->{gid}});

					if(!defined $d->{as})
					{
						$d->{ip} =~ m/(\d+)/;
						$d->{as} = $1;
						$d->{nlinks} = 0;
					}

					if(!defined $d->{ext}{"$n->{as}-$n->{id}"})
					{
						$d->{ext}{"$n->{as}-$n->{id}"} = $n;
					  	$d->{n_ext}++;

						if(!defined $d->{ext2}{$n->{gid}} &&
					   	   ($d->{nlinks2} + $d->{n_ext2}) < $max)
						{
					  		$d->{n_ext2}++ if(!defined $d->{ext2}{$n->{gid}});
							$d->{ext2}{$n->{gid}} = $n;
						}
					}

					&create_network($d);

					# $max, since adding the link is +1
					if(!defined $n->{ext2}{$d->{gid}} &&
					   ($n->{nlinks2} + $n->{n_ext2}) < $max)
					{
						$n->{n_ext2}++ if(!defined $n->{ext2}{$d->{gid}});
						$n->{ext2}{$d->{gid}} = $d;
					}
				}
			}
		}

		# Links to nodes in same ISP AS
		if($n->{nlinks} > 0)
		{
			foreach $k (keys %{$n->{links}})
			{
				$d = $ext_nodes{$k};

				next if($n->{gid} == $d->{gid});
				#next if(defined $n->{ext2}{$d->{gid}});

				$n->{links}{$k} = $d;

				# $max, since adding the link is +1
				if(!defined $n->{links2}{$d->{gid}} &&
				   ($n->{nlinks2} + $n->{n_ext2}) < $max)
				{
					$n->{links2}{$d->{gid}} = $d;
					$n->{nlinks2}++;

					if(!defined $d->{links2}{$n->{gid}} &&
					   ($d->{nlinks2} + $d->{n_ext2}) < $max)
					{
						$d->{links2}{$n->{gid}} = $n;
						$d->{nlinks2}++;
					}
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
