#!/usr/bin/perl

use Node;
use UNIVERSAL qw(isa);
use FileHandle;

my (%g_isp_leaf, %ext_nodes, %aliases, %ases, %areas) = ();
my (%g_co, @g_nodes, @g_ag_svr, @subnets, %g_isp_bb) = ();
my ($as_name, $nnodes, $n_tot_nodes, $namb, $ndisc, $n_ext, $g_next_bb) = 0;
my ($g_cnt, $g_n_extlinks, $g_nlinks) = 0;
my $g_root = undef;

my $next_area = 255;

my @cch = `find . -name "*.cch" -print`;
my @al = `find . -name "*.al" -print`;

#
# Get all of the aliases
#
&read_aliases();

$nnodes = 0;
chomp @cch;
foreach $file (@cch)
{
	$file =~ m/.*\/(.*)\.cch/;

	printf("%-50s %-15d\n", "Processing AS:", $1);
	&read_cch($file, $1);
}

&link_external_nodes();

&create_tables();
&create_access();

# Link nodes to nodes.. all RF external links are to unknown (not in alias
# file) external routers.

&rr_access_network();
&enumerate_network();

&print_network();

printf("\nTopology Statistics:\n\n");
printf("\t%-50s %-15d\n", "Total Links", $g_nlinks + $g_n_extlinks);
printf("\t%-50s %-15d\n", "Total Nodes", $n_tot_nodes);
printf("\t%-50s %-15d\n", "Nodes Used", $nnodes);
printf("\t%-50s %-15d\n", "Ambiguous", $namb);
printf("\t%-50s %-15d\n", "Disconnected", $ndisc);

exit(0);

sub create_tables()
{
	my $cnt = 0;
	my @nodes = ();
	my ($node, $n) = undef;

	@nodes = values %g_isp_bb;
	push @nodes, values %g_isp_leaf;

	# Fully connect BB routers
	
	print "\nCreating Routing Tables: ", $#nodes+1, "\n";

	for $n (@nodes)
	{
		my (@frontier, @solution, @previous, @distance) = ();
		my ($j, $q, $u, $v, $size, $dist, $last) = 0;

		next if !defined $$n->{nlinks2} && !defined $$n->{n_ext2};
		next if !defined $$n->{in_network};

		print "\tComputing table for node: $cnt \n" if (++$cnt % 100) == 0;

		for $j (0..$#g_nodes+1)
		{
			push @frontier, -1;
			push @previous, -1;
			push @distance, -1;
			push @solution, 0;
		}

		$solution[$$n->{gid}] = 1;
		$frontier[0] = $$n->{gid};
		$$n->{in_network} = 2;
		$size = 1;

		while($size)
		{
			for($q = 0, $u = $frontier[$q], $last = $q; $q < $size; $q++)
			{
				if($distance[$frontier[$q]] < $distance[$u])
				{
					$u = $frontier[$q];
					$last = $q;
				}
			}

			$frontier[$last] = $frontier[$size] if(--$size);

			$dist = $distance[$u] + 1;
			$solution[$u] = 1;
			$node = $g_nodes[$u];

			next if !defined $$node->{in_network};

			for my $link (values %{$$node->{links2}})
			{
				$v = $$link->{gid};

				next if !defined $$link->{in_network};
				next if($solution[$v]);
				next if($dist > 32);

				if($dist < $distance[$v] || 
					$distance[$v] == -1)
				{
					$$node->{in_network} = 2;
					$previous[$v] = $node;
					$distance[$v] = $dist;

					$frontier[$size++] = $v;
				}
			}

			for my $link (values %{$$node->{ext2}})
			{
				$v = $$link->{gid};

				next if !defined $$link->{in_network};
				next if($solution[$v]);
				next if($dist > 32);

				if($dist < $distance[$v])
				{
					$$node->{in_network} = 2;
					$previous[$v] = $node;
					$distance[$v] = $dist;

					$frontier[$size++] = $v;
				}
			}
		}

		$$n->{previous} = \@previous;

		if(1) # $n == $nodes[0])
		{
			$i = 0;
			for(0..$#g_nodes)
			{
				$i += $solution[$_];
			}

			#print("\t\t$i paths from $$n->{name}\n");
		}
	}
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
	$g_cnt = 0;
	my $bgp_routes = 0;

	print "Enumerating Network...";

	foreach $k0 (keys %ases)
	{
		die "Cannot enumerate AS: $k0!" if $k0 == '';

		%ar = %{$ases{$k0}};

		foreach $k1 (keys %ar)
		{
			%sn_h = %{$areas{$k1}};

			foreach $k2 (keys %sn_h)
			{
				@sn = @{$subnets{$k2}};

				foreach $n (@sn)
				{
					next if 2 != $$n->{in_network};

					$g_root = $n if ($g_cnt == 0);
					$$n->{nid} = $g_cnt++;
				}
			}
		}
	}

	print " done!\n\tEnumerated $g_cnt nodes\n";
}

sub print_network
{
	my %as = ();
	my %ar = ();
	my %sn_h = ();
	my @sn = ();

	my ($k0, $k1, $k2);

	my $good = 0;
	my $bad = 0;
	my $bad1 = 0;

	my $asid = 0;
	my $arid = 0;
	my $snid = 0;
	my $cnt = 0;
	my $cnt_l = 0;
	my $bgp_routes = 0;

	my ($s, $t) = undef;

	my $rt = new FileHandle "ip.rt", ">";
	my $fh = new FileHandle "ncs2.xml", ">";

	print "Printing Network...";

	$fh->printf("<rossnet>\n");

	foreach $k0 (keys %ases)
	{
		%ar = %{$ases{$k0}};
		#$fh->printf("<as id='%d' name='$k0' frequency='1'>\n", $asid++);
		$fh->printf("<as id='%d' frequency='1'>\n", $asid++);

		foreach $k1 (keys %ar)
		{
			%sn_h = %{$areas{$k1}};
			#$fh->printf("\t<area id='%d' name='%s'>\n", $arid++, $k1);
			$fh->printf("\t<area id='%d'>\n", $arid++, $k1);

			foreach $k2 (keys %sn_h)
			{
				@sn = @{$subnets{$k2}};
				#$fh->printf("\t<subnet id='%d' name='%s'>\n", $snid++, $k2);
				$fh->printf("\t\t<subnet id='%d'>\n", $snid++, $k2);

				foreach $n (@sn)
				{
					next if 2 != $$n->{in_network};

					$cnt_l += $$n->print($fh);
					$bgp_routes += $$n->{n_ext};
					$cnt++;

					next if $$n->{type} eq "c_router";

if($$n->{nid} == 228875)
{
	print "here!\n";
}
					# print forwarding table
					my @ft = ();
					push @ft, @{$$n->{rt}};

					$s = ${$$n->{conn}}->{isp};
					$t = ${$$n->{isp}}->{gid};

					if(-1 == $$s->{previous}[$t] &&
						$$s->{gid} != $t)
					{
						$rt->printf("%d %d 0\n", 
							    $$n->{nid}, 
							    ${$$n->{conn}}->{nid});
						$bad++;

						next;
					}

					# push on T
					push @ft, $$n->{isp} 
						if $$s->{gid} != ${$$n->{isp}}->{gid};

					# push on path from T to so S
					while(-1 != $$s->{previous}[$t] && $#ft <= 32)
					{
						push @ft, $$s->{previous}[$t];
						$t = ${$$s->{previous}[$t]}->{gid};
					}

					# push on S if I haven't already
					push @ft, $s
						if $$s->{gid} != ${$$n->{isp}}->{gid} &&
						   -1 == $$s->{previous}[${$$n->{isp}}->{gid}];

					# now push on access network for destination
					$d = $$n->{conn};

					# Already have CO if COND == true
					pop @ft if $$s->{gid} == ${$$n->{isp}}->{gid};

					push @ft, reverse @{$$d->{rt}};

					# have 3 hops with HA, CT, CO and LEAF
					if(-1 == $#ft || $#ft > 32)
					{
						$rt->printf("%d %d 0\n", 
							    $$n->{nid}, 
							    ${$$n->{conn}}->{nid});
						$bad1++;
					} else
					{
						$rt->printf("%d %d %d ", 
							    $$n->{nid}, 
							    ${$$n->{conn}}->{nid},
							    $#ft+1);

						for $d (@ft)
						{
							$rt->printf("$$d->{nid} ");
						}

						$rt->printf("\n");
						$good++;
					}
				}

				$fh->printf("\t\t</subnet>\n");
			}

			$fh->printf("\t</area>\n");
		}

		$fh->printf("</as>\n");
	}

	for $n (@g_ag_svr)
	{
		$d = $$n->{conn};
		$fh->printf("<connect src='$$n->{nid}' dst='$$d->{nid}'/>\n");
	}

	$fh->printf("</rossnet>\n");

	print " done!\n\tGot $cnt nodes, $bgp_routes eBGP links\n";
	print "Forwarding Table Connections:\n\n";
	print "\tGood: $good \n";
	print "\tBad : $bad\n";
	print "\tBadP: $bad1\n";
}

sub read_cch
{
	my $f = shift;
	my $as_name = shift;
	my ($id, $nl, $ne, $cnt, $lvl);

	my %as = $ases{$as_name};
	my @attr = ();

	open (F, "$f") or die "Unable to to open $f: $!";

	foreach (<F>)
	{
		my ($i, $n, $next) = undef;

		$_ =~ s/[<|>|=|!|&|{|}|(|)]//g;

		@_ = split(/\s+/, $_);

		$id = shift @_;

		next if $id =~ /^#/;

		@attr = split(/\s+/, $_);

		#
		# Links to external (outside my AS) routers.  
		# May have ext router, may not.
		#
		if($id =~ /^-/)
		{
			$n = undef;
			$n = $aliases{$attr[1]};
			next if($n == undef);

			$$n->{dns} = shift @_;
			$lvl = shift @_;
			$lvl =~ s/r//g;

			$$n->{level} = $lvl;

			$ext_nodes{"$as_name-$id"} = $n;
			$n_ext++;

			next;
		} else
		{
			next if $_ !~ /.*IL.*/;
		}

		#print "my ip: ", $attr[$#attr-1], "\n";
		$n = undef;
		$n = $aliases{$attr[$#attr-1]};

		if(!defined $n)
		{
			print STDERR "Could not find node: $attr[$#attr-1] (@attr) \n";
			exit(0);
		}

		$ext_nodes{"$as_name-$id"} = $n;

		#shift @_; 
		$$n->name(shift @_);
		$status = $$n->connected();

		#print "\tName: ", $$n->name(), "\n";
		#print "\tStat: ", $status, "\n";
	
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
			$$n->{plus} = 1;
			$nl = shift @_;
		}
	
		if($nl eq "bb")
		{
			$$n->{bb} = 1;
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
			next if(defined $$n->{links}{"$as_name-$next"});

			$$n->{links}{"$as_name-$next"} = 1;
			$cnt++;
		}

		$g_nlinks += $cnt;
		$$n->{nlinks} += $cnt;

		if($$n->{nlinks} > 2000)
		{
			die "too many links?!";
		}

		die "No links on router!" if($$n->{nlinks} == 0);

		$i = $cnt = 0;
		while($i < $ne)
		{
			$i++;
			$yy = $as_name . "-" . shift @_;

			next if($$n->{ext}{$yy} == 1);
			$cnt++;

			$$n->{ext}{$yy} = 1;
		}

		$g_nlinks += $cnt;

		$$n->{n_ext} += $cnt;
		$$n->{dns} = shift @_;
		$$n->{level} = shift @_;
		$$n->{level} =~ s/r//;
		$$n->{as} = $as_name;

		$g_isp_leaf{$as_name . "-" . $id} = $n 
			if(0 == $$n->{bb} && $n != defined $ext_nodes{"$as_name-$id"} &&
				$$n->{nlinks} > 0 &&
				$$n->{name} =~ /.*IL.*/);

		#$g_isp_leaf{$as_name . "-" . $id} = $n 
		#	if(0 == $n->{bb} && $n != defined $ext_nodes{"$as_name-$id"} &&
		#		$n->{nlinks2} == 1);

		if(!defined $$n->{in_network} && 1 == $$n->{bb} && $$n->{nlinks} > 0 &&
			$$n->{name} =~ /.*IL.*/)
		{
			$g_isp_bb{$as_name . '-' . $id} = $n;
		}

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

	return if defined $$n->{in_network};

	die "No type!" if(!defined $$n->{type});

	$as = $$n->{as};

	# get the subnet addr and put node into subnet
	$$n->{ip} =~ m/(\d+\.\d+\.\d+)/;
	$sn = $as . '-' . $1;
	push @{$subnets{$sn}}, $n;

	# get the area addr and put the subnet into the area
	$$n->{ip} =~ m/(\d+\.\d+)/;
	$ar = $as . '-' . $1;
	$areas{$ar}{$sn} = $subnets{$sn};

	# put the area into the AS
	$ases{$$n->{as}}{$ar} = $areas{$ar};

	$$n->{in_network} = 1;
}

#
# Round-robin allocate access level CO routers to ISP topology
#
sub rr_access_network()
{
	my ($n) = undef;
	my ($next, $k) = 0;
	my @co_a = values %g_co;

	print "Linking access network...";

	for $k (keys %g_isp_leaf)
	{
		$l = $g_isp_leaf{$k};

		if($$l->{in_network} != 2)
		{
			delete $g_isp_leaf{$k};
		}
	}

	while(@co_a)
	{
		for $l (values %g_isp_leaf)
		{
			die "LEAF not in network!" if $$l->{in_network} != 2;
			$co = pop @co_a;

			last if(!defined $co);

			$next_area++;

			#print "LEAF $$l->{name}: ", $$l->{ip}, " lvl: $$l->{level} \n";
			$$l->{ip} =~ m/(\d+)/;
			$$co->{ip} = $1 . ".$next_area.0.$$co->{gid}";

			#$$l->{ip} =~ m/(\d+\.\d+\.\d+\.)/;
			#$$co->{ip} = $1 . "$$co->{gid}";

			$$co->{as} = $$l->{as};
			$$co->{level} = $$l->{level} + 1;
			#print "  CO $$co->{name}: ", $$co->{ip}, " lvl: $$co->{level} \n";

			$$co->{links2}{$$l->{gid}} = $l;
			$$co->{nlinks2}++;

			$$l->{links2}{$$co->{gid}} = $co;
			$$l->{nlinks2}++;

			&create_network($co);
			$$co->{in_network} = 2;

			foreach $ct (values %{$$co->{links2}})
			{
				next if $ct == $l;

				$$ct->{as} = $$l->{as};
				$$ct->{ip} = $1 . ".$next_area.0.$$ct->{gid}";
				#$$ct->{ip} = $1 . "$$ct->{gid}";

				$next_area++;

				$$ct->{level} = $$co->{level} + 1;
				#print "\tCT/OG $$ct->{name}: ", $$ct->{ip}, "\n" if $$ct->{name} =~ /^CT/;

				&create_network($ct);
				$$ct->{in_network} = 2;

				foreach $ha (values %{$$ct->{links2}})
				{
					next if $$ha == $$co;

					$$ha->{as} = $$l->{as};
					$$ha->{ip} = $1 . ".$next_area.0.$$ha->{gid}";
					#$$ha->{ip} = $1 . "$$ha->{gid}";
					$$ha->{level} = $$ct->{level} + 1;

					#print "\t\tHA $$ha->{gid}: ", $$ha->{ip}, "\n";

					if($$ct->{name} =~ /^OGR/)
					{
						$$ha->{isp} = $l;
						push @{$$ha->{rt}}, $ct;
						push @{$$ha->{rt}}, $co;
					}

					&create_network($ha);
					$$ha->{in_network} = 2;

					foreach $ag (values %{$$ha->{links2}})
					{
						next if $ag == $ct;

						$$ag->{as} = $$l->{as};
						$$ag->{ip} = $1 . ".$next_area.0.$$ag->{gid}";
						#$$ag->{ip} = $1 . "$$ag->{gid}";
						$$ag->{level} = $$ha->{level} + 1;

						push @{$$ag->{rt}}, $ha;
						push @{$$ag->{rt}}, $ct;
						push @{$$ag->{rt}}, $co;

						$$ag->{isp} = $l;

						#print "\t\t\tAG $$ag->{gid}: ", $$ag->{ip}, "\n";

						&create_network($ag);
						$$ag->{in_network} = 2;
					}
				}
			}

			last if(0 == @co_a);
		}
	}

	print "done!\n";
	print "\tGot " . keys (%g_isp_bb) . " ISP Backbone Routers \n";
	print "\tGot " . keys (%g_isp_leaf) . " ISP Leaf Routers \n";
}

#
# Link external and internal link numbers to IP addrs
#
sub link_external_nodes
{
	my ($n, $d, $i, $cnt) = (undef, undef, 0, 0);

	# Max out-degree per node
	my $max = 5000;

	$g_n_extlinks = 0;

	# fully connect BB so that (hopefully) we can reach everything
	for $n (values %g_isp_bb)
	{
		for $d (values %g_isp_bb)
		{
			next if $n == $d;

			if(!defined $$n->{links2}{$$d->{gid}})
			{
				$$n->{links2}{$$d->{gid}} = $d;
				$$n->{nlinks2}++;

				$$d->{links2}{$$n->{gid}} = $n;
				$$d->{nlinks2}++;
			}
		}
	}

	print "\nLinking network...";

	for $n (values %aliases)
	{
		next if !defined $n;

		if($$n->{n_ext} > 0)
		{
			foreach $k (keys %{$$n->{$ext}})
			{
				next if isa($k, 'Node');

				$d = $ext_nodes{$k};

				if(!defined $d || !defined $$d->{name})
				{
					delete $$n->{ext}{$k};
					--$$n->{n_ext};
				} else
				{
					$$n->{ext}{$k} = $d;

					next if($$n->{gid} == $$d->{gid});
					#next if(defined $$n->{links2}{$$d->{gid}});

					if(!defined $$d->{as})
					{
						$$d->{ip} =~ m/(\d+)/;
						$$d->{as} = $1;
						$$d->{nlinks} = 0;
					}

					die "No type!" if !defined $$d->{type};

					if(!defined $$d->{ext}{"$$n->{as}-$$n->{gid}"})
					{
						$$d->{ext}{"$$n->{as}-$$n->{gid}"} = $n;
					  	$$d->{n_ext}++;

						$g_n_extlinks++;

						if(!defined $$d->{ext2}{$$n->{gid}} &&
					   	   ($$d->{nlinks2} + $$d->{n_ext2}) < $max)
						{
die "here\n" if $$n->{type} eq "";
							$$d->{ext2}{$$n->{gid}} = $n;
					  		$$d->{n_ext2}++;
						
							$g_n_extlinks++;
						}
					}

					&create_network($d);

					# $max, since adding the link is +1
					if(!defined $$n->{ext2}{$$d->{gid}} &&
					   ($$n->{nlinks2} + $$n->{n_ext2}) < $max)
					{
						$$n->{ext2}{$$d->{gid}} = $d;
						$$n->{n_ext2}++;

						$g_n_extlinks++;
					}
				}
			}
		}

		if($$n->{nlinks} > 0)
		{
			foreach $k (keys %{$$n->{links}})
			{
				$d = $ext_nodes{$k};

				next if !defined $d;
				next if($$n->{gid} == $$d->{gid});
				#next if(defined $$n->{ext2}{$$d->{gid}});

				$$n->{links}{$k} = $d;

				# $max, since adding the link is +1
				if(!defined $$n->{links2}{$$d->{gid}} &&
				   ($$n->{nlinks2} + $$n->{n_ext2}) < $max)
				{
die "here\n" if $$d->{type} eq "";
					$$n->{links2}{$$d->{gid}} = $d;
					$$n->{nlinks2}++;

					$g_n_extlinks++;

					if(!defined $$d->{links2}{$$n->{gid}} &&
					   ($$d->{nlinks2} + $$d->{n_ext2}) < $max)
					{
die "here\n" if $$n->{type} eq "";
						$$d->{links2}{$$n->{gid}} = $n;
						$$d->{nlinks2}++;

						$g_n_ext_links++;
					}
				}
			}
		}

		$cnt++;
		#print "Done linking node: $$n->{ip} ($$n->{gid})\n" if ($cnt % 1000 == 0);
	}

	print " done!\n";
	print "\tGot $g_n_extlinks eBGP links\n";
	print "\tGot $cnt nodes\n";
}

sub random($$)
{
	my $low = shift;
	my $high = shift;

	return int(rand($high)) + $low;
}

sub create_access
{
	my ($nco, $nco_good, $nct, $nha, $ncg, $nag) = 0;
	my %g_ct = ();
	my @g_ha = ();
	my @g_og = ();
	my ($jj, $kk, $ll);

	$n_tot_nodes = 0 if !defined $n_tot_nodes;

	# read in COs, connect to CTs
	open(F, 'access/co.dat') or die "Unable to open access/co.dat: $!";
	printf("%-50s %-15s\n", "\nProcessing Access:", "access/co.dat");

	$nco = -1;
	while(<F>)
	{
		$nco++;
		chop $_; chop $_;

		@_ = split(/\s+/);
		next if !@_;

		#print "Next CO: $nco ($n_tot_nodes) \n";

		my $n = $g_co{$nco} = \(new Node($n_tot_nodes++));
		$$n->{type} = 'c_router';
		$$n->name("CO_" . $nco);

		for(@_)
		{
			die "CT $_ in two Central Offices!\n" if defined $g_ct{$_};

			#print "\tnew CT: $_\n";
			$d = $g_ct{$_} = \(new Node($n_tot_nodes++));
			$$d->{type} = 'c_router';
			$$d->name("CT_$_");
			$nct++;

			$$n->{links2}{$$d->{gid}} = $d;
			$$n->{nlinks2}++;

			$$d->{links2}{$$n->{gid}} = $n;
			$$d->{nlinks2}++;

			$g_nlinks += 2;
		}

		$nco_good++;
	}

	close(F);

	# read in HARs, connect to CTs
	open(F, 'access/har.dat') or die "Unable to open access/har.dat: $!";
	printf("%-50s %-15s\n", "Processing Access:", "access/har.dat");

	while(<F>)
	{
		chop; chop;

		printf("\t%-50s %-15d \n", "Configuring HAR", $#g_ha) if $#g_ha % 25000 == 0;

		my $n = new Node($n_tot_nodes++);
		$n->{type} = 'c_router';
		$n->name("HA_" . $nha++);

		die "No CT router for Home $_!\n" if !defined $g_ct{$_};

		my $d = $g_ct{$_};
		$$d->{links2}{$n->{gid}} = \$n;
		$$d->{nlinks2}++;

		$n->{links2}{$$d->{gid}} = $d;
		$n->{nlinks2}++;

		$g_nlinks += 2;

		push @g_ha, \$n;
	}

	close(F);

	# read in OGRs, connect to COs
	open(F, 'access/ogr.dat') or die "Unable to open access/ogr.dat: $!";
	printf("%-50s %-15s\n", "Processing Access:", "access/ogr.dat");

	my ($h, $g, $f) = (0, 0, 0);

	while(<F>)
	{
		chop; chop;
		$_ = (split(/\s+/))[0];

		my $n = new Node($n_tot_nodes++);
		$n->{type} = 'c_router';
		$n->name("OGR_" . $nog++);

		die "CO $_ not defined for Office ", $nog-1, "!\n" if !defined $g_co{$_};

		my $d = $g_co{$_};
		$$d->{links2}{$n->{gid}} = \$n;
		$$d->{nlinks2}++;

		$n->{links2}{$$d->{gid}} = $d;
		$n->{nlinks2}++;

		$g_nlinks += 2;

		push @g_og, \$n;
	}

	#print "Last OGR: $n_tot_nodes \n";

	close(F);

	# read in Agents
	open(F, 'access/agents.dat') or die "Unable to open access/agents.dat: $!";
	printf("%-50s %-15s\n", "Processing Access:", "access/agents.dat");

	$g_next_bb = 0;
	my @bb = keys %g_isp_bb;
	my $c;
	my $nag = 1;

	for $k (keys %g_isp_bb)
	{
		$l = $g_isp_bb{$k};

		if($$l->{in_network} != 2)
		{
			delete $g_isp_bb{$k};
		}
	}

	while(<F>)
	{
		chop; chop;

		#last if $nag > 10000;

		($h, $g, $f) = split(/\s+/);
		#print "\tconnected to: $h-$g ($f)\n";

		die "Bad HA: $h for agent $nag\n" if !defined $g_ha[$h];

		printf("\t%-50s %-15d \n", "Configuring Agent", $nag) if $#g_ha % 50000 == 0;

		my $n = new Node($n_tot_nodes++);
		$n->{type} = 'c_host';
		$n->name("AG_" . $nag);

		my $d = $g_ha[$h];
		$$d->{links2}{$n->{gid}} = \$n;
		$$d->{nlinks2}++;

		die "too many links!\n" if($$d->{nlinks2} > 10000 || $n->{nlinks2} > 10000);

		$n->{links2}{$$d->{gid}} = $d;
		$n->{nlinks2}++;

		$g_nlinks += 2;

		# here, $n is the TCP server 'c_host'
		my $svr = new Node($n_tot_nodes++);
		$svr->{type} = 'c_host';
		$svr->name("AG_SVR_$nag");
		$svr->{conn} = \$n;
		$n->{conn} = \$svr;

		my $svr_d = undef;

		# either connected to an OGR ($g) or ISP BB router
		if($g != -1)
		{
			die "Invalid OGR!" if !defined $g_og[$g];

			$svr_d = $g_og[$g];
		} else
		{
			$g_next_bb = 0 if(++$g_next_bb > $#bb);
			$svr_d = $g_isp_bb{$bb[$g_next_bb]};

			die "Invalid BB router!" if !defined $svr_d || $$svr_d->{bb} != 1;
			die "BB not in network!" if $$svr_d->{in_network} != 2;

			# Will not be added to network in rr_access_network!

			$$svr_d->{ip} =~ m/(\d+\.\d+\.\d+\.)/;

			$svr->{ip} = $1 . $svr->{gid};
			$svr->{as} = $$svr_d->{as};

			&create_network(\$svr);
			$svr->{in_network} = 2;

			$svr->{isp} = $svr_d;
		}

		#$$svr_d->{ip} =~ m/(\d+\.\d+\.\d+\.)/;

		#$svr->{as} = $$svr_d->{as};
		#$svr->{ip} = $1 . $svr->{gid};
		$svr->{level} = $$svr_d->{level};

		$$svr_d->{links2}{$svr->{gid}} = \$svr;
		$$svr_d->{nlinks2}++;

		$svr->{links2}{$$svr_d->{gid}} = $svr_d;
		$svr->{nlinks2}++;

		$g_nlinks += 2;

		push @g_ag_svr, \$svr;
		$nag++;
	}

	close(F);

	printf("\n");
	printf("\t%-50s %-15d \n", "Total CO", $nco_good);
	printf("\t%-50s %-15d \n", "Total CT", $nct);
	printf("\t%-50s %-15d \n", "Total HA", $#g_ha + 1);
	printf("\t%-50s %-15d \n", "Total OG", $#g_og + 1);
	printf("\n");
	printf("\t%-50s %-15d \n", "Total TCP Clients", $nag-1);
	printf("\t%-50s %-15d \n", "Total TCP Server", $#g_ag_svr + 1);
	printf("\n");
}

sub read_aliases
{
        my ($f, $l, $id, $ip, $dns, $last);

	chomp @al;

        foreach $f (@al)
	{
		open (F, "$f") or die "Could not open file: $f: $!";
		$f =~ m/.*\/(.*)\.al/;
		printf("%-50s %-15d\n", "Processing Alias:", $1);

		$last = -1;
                foreach $l (<F>)
                {
			($id, $ip, $dns) = split(/\s+/, $l);

			if($id != $last)
			{
				$aliases{$ip} = $aliases{$dns} = \(new Node($n_tot_nodes++));

				$n = $aliases{$ip};
				$$n->{type} = 'c_router';
				$$n->{dns} = $dns;
				$$n->name("no name-alias");
				$$n->{ip} = $ip;
				push @g_nodes, $n;
			} else
			{
				$aliases{$ip} = $aliases{$dns} = $n;
			}

			$last = $id;
                }

                close (F);
        }

	print ("\n");
}
