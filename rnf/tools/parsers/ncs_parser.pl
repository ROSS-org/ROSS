#!/usr/bin/perl

use NcsNode;
use UNIVERSAL qw(isa);
use FileHandle;

my (%g_isp_leaf, %ext_nodes, %aliases, %ases, %areas) = ();
my (%g_co, @g_ag_svr, @subnets, %g_isp_bb) = ();
my ($as_name, $nlinks, $nnodes, $n_tot_nodes, $namb, $ndisc, $n_ext, $g_next_bb) = 0;
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

&create_access();

# Link nodes to nodes.. all RF external links are to unknown (not in alias
# file) external routers.

&link_external_nodes();
&rr_access_network();
&enumerate_network();

&print_network();
&print_areas();

#&print_non_network();

printf("\nTopology Statistics:\n\n");
printf("        %-50s %-11d\n", "Total Links", $nlinks);
printf("        %-50s %-11d\n", "Total Nodes", $n_tot_nodes);
printf("        %-50s %-11d\n", "Nodes Used", $nnodes);
printf("        %-50s %-11d\n", "Ambiguous", $namb);
printf("        %-50s %-11d\n", "Disconnected", $ndisc);

exit(0);

sub print_non_network
{
	my ($n, $fh);

	$fh = new FileHandle "non-internet.xml", ">";
	print "Printing Non-Network...";
	foreach $n (values %aliases)
	{
		$n->print($fh) if !defined $n->{in_network};
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
					next if !defined $$n->{nlinks2} || !defined $$n->{in_network};

					$$n->{nid} = $cnt++;

					if($$n->{n_ext2} > 0)
					{
						#push @{$ases{$k0}{ibgp}}, $$n->{nid};
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
	my $cnt_l = 0;
	my $bgp_routes = 0;

	$fh = new FileHandle "ncs2.xml", ">";
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
					next if !defined $$n->{nlinks2} || !defined $$n->{in_network};

					if(!defined $$n->{nid})
					{
						die "no nid for $$n->{name}!";
						next;
					}

					#bless $$n, Node;
					$cnt_l += $$n->print($fh);
					$bgp_routes += $$n->{n_ext};

					$cnt++;
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

	print " done! Got $cnt nodes, $bgp_routes eBGP links\n";
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

		$nlinks += $cnt;
		$$n->{nlinks} += $cnt;

		if($$n->{nlinks} > 2000)
		{
			die "here!";
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

		$nlinks += $cnt;

		$$n->{n_ext} += $cnt;
		$$n->{dns} = shift @_;
		$$n->{level} = shift @_;
		$$n->{level} =~ s/r//;
		$$n->{as} = $as_name;

		$g_isp_leaf{$as_name . "-" . $id} = $n 
			if(0 == $$n->{bb} && $n != defined $ext_nodes{"$as_name-$id"} &&
				$$n->{nlinks} > 0);
		#$g_isp_leaf{$as_name . "-" . $id} = $n 
		#	if(0 == $n->{bb} && $n != defined $ext_nodes{"$as_name-$id"} &&
		#		$n->{nlinks2} == 1);

		if(!defined $$n->{in_network} && 1 == $$n->{bb} && $$n->{nlinks} > 0)
		{
			$g_isp_bb{$as_name . '-' . $id} = $n;
		}

		#push @g_isp_bb, $n if(1 == $n->{bb});

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
# Round-robin allocate access level CO routers to ISP datology
#
sub rr_access_network()
{
	my ($n) = undef;
	my $next = 0;
	my @co_a = values %g_co;

	print "Linking access network...";

	while(@co_a)
	{
		for $l (values %g_isp_leaf)
		{
			$co = pop @co_a;

			last if(!defined $co);

			$next_area++;

			#print "LEAF $$l->{name}: ", $$l->{ip}, " lvl: $$l->{level} \n";
			$$l->{ip} =~ m/(\d+)/;
			$$co->{as} = $$l->{as};
			$$co->{ip} = $1 . ".$next_area.0.$$co->{gid}";
			$$co->{level} = $$l->{level} + 1;
			#print "  CO $$co->{name}: ", $$co->{ip}, " lvl: $$co->{level} \n";

			$$co->{links2}{$$l->{gid}} = $l;
			$$co->{nlinks2}++;

			$$l->{links2}{$$co->{gid}} = $co;
			$$l->{nlinks2}++;

			&create_network($co);

			foreach $ct (values %{$$co->{links2}})
			{
				next if $ct == $l;

				$$ct->{as} = $$l->{as};
				$$ct->{ip} = $1 . ".$next_area.0.$$ct->{gid}";

				$next_area++;

				$$ct->{level} = $$co->{level} + 1;
				#print "\tCT/OG $$ct->{name}: ", $$ct->{ip}, "\n" if $$ct->{name} =~ /^CT/;
			
				&create_network($ct);

				foreach $ha (values %{$$ct->{links2}})
				{
					next if $$ha == $$co;

					$$ha->{as} = $$l->{as};
					$$ha->{ip} = $1 . ".$next_area.0.$$ha->{gid}";
					$$ha->{level} = $$ct->{level} + 1;

					#print "\t\tHA $$ha->{gid}: ", $$ha->{ip}, "\n";

					&create_network($ha);

					foreach $ag (values %{$$ha->{links2}})
					{
						next if $ag == $ct;

						$$ag->{as} = $$l->{as};
						$$ag->{ip} = $1 . ".$next_area.0.$$ag->{gid}";
						$$ag->{level} = $$ha->{level} + 1;

						#print "\t\t\tAG $$ag->{gid}: ", $$ag->{ip}, "\n";

						&create_network($ag);
					}
				}
			}

			last if(0 == @co_a);
		}
	}

	print "done! " . keys (%g_isp_leaf) . " ISP Leaf Routers \n";
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

						if(!defined $$d->{ext2}{$$n->{gid}} &&
					   	   ($$d->{nlinks2} + $$d->{n_ext2}) < $max)
						{
die "here\n" if $$n->{type} eq "";
							$$d->{ext2}{$$n->{gid}} = $n;
					  		$$d->{n_ext2}++;
						}
					}

					&create_network($d);

					# $max, since adding the link is +1
					if(!defined $$n->{ext2}{$$d->{gid}} &&
					   ($$n->{nlinks2} + $$n->{n_ext2}) < $max)
					{
die "here\n" if $$d->{type} eq "";
						$$n->{ext2}{$$d->{gid}} = $d;
						$$n->{n_ext2}++;
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

					if(!defined $$d->{links2}{$$n->{gid}} &&
					   ($$d->{nlinks2} + $$d->{n_ext2}) < $max)
					{
die "here\n" if $$n->{type} eq "";
						$$d->{links2}{$$n->{gid}} = $n;
						$$d->{nlinks2}++;
					}
				}
			}
		}

		$cnt++;
		#print "Done linking node: $$n->{ip} ($$n->{gid})\n" if ($cnt % 1000 == 0);
	}

	print " done!\n";
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
	open(F, 'co.dat') or die "Unable to open co.dat: $!";
	printf("%-50s %-15s\n", "\nProcessing Access:", "co.dat");

	$nco = -1;
	while(<F>)
	{
		$nco++;
		chop $_; chop $_;

		@_ = split(/\s+/);
		next if !@_;

		print "Next CO: $nco ($n_tot_nodes) \n";

		$g_co{$nco} = \(new Node($n_tot_nodes++));
		my $n = $g_co{$nco};
		$$n->{type} = 'c_router';
		$$n->name("CO_" . $nco);

		for(@_)
		{
			die "CT $_ in two Central Offices!\n" if defined $g_ct{$_};

			print "\tnew CT: $_\n";
			$g_ct{$_} = \(new Node($n_tot_nodes++));
			$d = $g_ct{$_};
			$$d->{type} = 'c_router';
			$$d->name("CT_$_");
			$nct++;

			$$n->{links2}{$$d->{gid}} = $d;
			$$n->{nlinks2}++;

			$$d->{links2}{$$n->{gid}} = $n;
			$$d->{nlinks2}++;
		}

		$nco_good++;
	}

	close(F);

	# read in HARs, connect to CTs
	open(F, 'har.dat') or die "Unable to open har.dat: $!";
	printf("%-50s %-15s\n", "Processing Access:", "har.dat");

	while(<F>)
	{
		chop; chop;

		#print "      Configuring HAR: $#g_ha \n" if $#g_ha % 10000 == 0;

		my $n = new Node($n_tot_nodes++);
		$n->{type} = 'c_router';
		$n->name("HA_" . $nha++);

		die "No CT router for Home $_!\n" if !defined $g_ct{$_};

		my $d = $g_ct{$_};
		$$d->{links2}{$n->{gid}} = \$n;
		$$d->{nlinks2}++;

		$n->{links2}{$$d->{gid}} = $d;
		$n->{nlinks2}++;

		push @g_ha, \$n;
	}

	close(F);

	# read in OGRs, connect to COs
	open(F, 'ogr.dat') or die "Unable to open ogr.dat: $!";
	printf("%-50s %-15s\n", "Processing Access:", "ogr.dat");

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

		push @g_og, \$n;
	}

	#print "Last OGR: $n_tot_nodes \n";

	close(F);

	# read in Agents
	open(F, 'agents.dat') or die "Unable to open agents.dat: $!";
	printf("%-50s %-15s\n", "Processing Access:", "agents.dat");

	$g_next_bb = 0;
	my @bb = keys %g_isp_bb;
	my $c;
	my $nag = 1;

	for (@bb)
	{
		${$g_isp_bb{$_}}->{ip} =~ m/(\d+)/;
		${$g_isp_bb{$_}}->{ip} = $1 . ".$next_area.0.${$g_isp_bb{$_}}->{gid}";
		#print "BB $_ (${$g_isp_bb{$_}}->{ip}): ${$g_isp_bb{$_}}->{nlinks2}\n";
		$next_area++;
	}

	while(<F>)
	{
		chop; chop;

		#last if $nag % 1000000 == 0;

		print "      Configuring Agent: $nag \n" if $nag % 10000 == 0;

		my $n = new Node($n_tot_nodes++);
		$n->{type} = 'c_host';
		$n->name("AG_" . $nag);

		($h, $g, $f) = split(/\s+/);
		#print "\tconnected to: $h-$g ($f)\n";

		die "Bad HA: $h for agent $nag\n" if !defined $g_ha[$h];

		my $d = $g_ha[$h];
		$$d->{links2}{$n->{gid}} = \$n;
		$$d->{nlinks2}++;

		die "too many links!\n" if($$d->{nlinks2} > 10000 || $n->{nlinks2} > 10000);

		$n->{links2}{$$d->{gid}} = $d;
		$n->{nlinks2}++;

		# here, $n is the TCP server 'c_host'
		my $svr = new Node($n_tot_nodes++);
		$svr->{type} = 'c_host';
		$svr->name("AG_SVR_$nag");
		$svr->{conn} = \$n;

		my $svr_d = undef;

		# either connected to an OGR ($g) or ISP BB router
		if($g != -1)
		{
			# get the OG router
			die "Invalid OGR!" if !defined $g_og[$g];
			$svr_d = $g_og[$g];
		} else
		{
			$g_next_bb = 0 if(++$g_next_bb > $#bb);

			$svr_d = $g_isp_bb{$bb[$g_next_bb]};
			die "Invalid BB router!" if !defined $svr_d;
			#print "NEXT BB ($g_next_bb): $$svr_d->{gid} \n";

			$svr->{as} = $$svr_d->{as};
			&create_network(\$svr);
		}

		$$svr_d->{ip} =~ m/(\d+\.\d+\.\d+)/;

		$svr->{as} = $$svr_d->{as};
		$svr->{ip} = $1 . '.' . $svr->{gid};
		$svr->{level} = $$svr_d->{level} + 1;

		$$svr_d->{links2}{$svr->{gid}} = \$svr;
		$$svr_d->{nlinks2}++;

		$svr->{links2}{$$svr_d->{gid}} = $svr_d;
		$svr->{nlinks2}++;

		push @g_ag_svr, \$svr;
		$nag++;
	}

	close(F);

	printf("\n");
	printf("\t%-42s %-15d \n", "Total CO", $nco_good);
	printf("\t%-42s %-15d \n", "Total CT", $nct);
	printf("\t%-42s %-15d \n", "Total HA", $#g_ha + 1);
	printf("\t%-42s %-15d \n", "Total OG", $#g_og + 1);
	printf("\n");
	printf("\t%-42s %-15d \n", "Total TCP Clients", $nag-1);
	printf("\t%-42s %-15d \n", "Total TCP Server", $#g_ag_svr + 1);
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
				#my $new_node = new Node($n_tot_nodes++);
				$aliases{$ip} = $aliases{$dns} = \(new Node($n_tot_nodes++));

				$n = $aliases{$ip};
				$$n->{type} = 'c_router';
				$$n->{dns} = $dns;
				$$n->name("no name-alias");
				$$n->{ip} = $ip;
			} else
			{
				$aliases{$ip} = $aliases{$dns} = $n;
			}

			$last = $id;
                }

                close (F);
        }
}

#
# Subnets have nodes..
# Areas have subnets..
# ASes have areas..
#
sub print_areas()
{
	my ($ar, $as);
	my $ar_n = 0;

	for $as (keys %ases)
	{
		die "Bad AS!" if $as  == '';

		print "AS: $as\n";

		for $ar (keys %{$ases{$as}})
		{
			$cnt = 0;

			# subnets in area
			for $n (keys %{$areas{$ar}})
			{
				for (@{$subnets{$n}})
				{
					$cnt++ if $$_->{type} == 'c_router';
				}
			}

			print "Area ", ++$ar_n, ": $ar, $cnt nodes \n" if $cnt > 1000;
		}
	}

	#$areas{$ar}{$sn} = $subnets{$sn};
	#$ases{$$n->{as}}{$ar} = $areas{$ar};
}
