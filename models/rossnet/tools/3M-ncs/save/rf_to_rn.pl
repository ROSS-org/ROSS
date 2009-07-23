#!/usr/bin/perl

use Node;
use UNIVERSAL qw(isa);
use FileHandle;

my (%g_isp_leaf, %ext_nodes, %aliases, %ases, %areas) = ();
my (@g_co, @g_ag_svr, @subnets, @g_isp_bb) = ();
my ($as_name, $nlinks, $nnodes, $n_tot_nodes, $namb, $ndisc, $n_ext, $g_next_bb) = 0;

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

#
# Link nodes to nodes.. all RF external links are to unknown (not in alias
# file) external routers.
#
&link_external_nodes();
&rr_access_network();
&enumerate_network();
&print_network();

#&print_non_network();

printf("\nTopology Statistics:\n\n");
printf("\t%-50s %-11d\n", "Total Links", $nlinks);
printf("\t%-50s %-11d\n", "Total Nodes", $n_tot_nodes);
printf("\t%-50s %-11d\n", "Nodes Used", $nnodes);
printf("\t%-50s %-11d\n", "Ambiguous", $namb);
printf("\t%-50s %-11d\n", "Disconnected", $ndisc);

exit(0);

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
	my $cnt_l = 0;
	my $bgp_routes = 0;

	$fh = new FileHandle "internet.xml", ">";
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
					next if !defined $n or !defined $n->{nid};

					$cnt_l += $n->print($fh, @{$ases{$k0}{ibgp}});
					$cnt_l += $n->print($fh);
					$bgp_routes += $n->{n_ext};

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
		$fh->printf("<connect src='$n->{nid}' dst='$n->{conn}->{nid}'/>\n");
	}

	$fh->printf("</rossnet>\n");

	print " done! Got $cnt nodes, $bgp_routes eBGP links\n";
}

sub read_cch
{
	my $f = shift;
	my $as_name = shift;
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
		next if $_ !~ /.*IL.*/;

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
	
		shift @_; #$n->name(shift @_);
		$status = $n->connected();

		#print "\tName: ", $n->name(), "\n";
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
			next if(defined $n->{links}{"$as_name-$next"});

			$n->{links}{"$as_name-$next"} = 1;
			$cnt++;
		}

		$nlinks += $cnt;
		$n->{nlinks} += $cnt;

		die "No links on router!" if($n->{nlinks} == 0);

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
		$n->{level} =~ s/r//;
		$n->{as} = $as_name;

		$g_isp_leaf{$as_name . "-" . $id} = $n 
			if(0 == $n->{bb} && $n != defined $ext_nodes{"$as_name-$id"} &&
				$n->{nlinks} > 0);
		#$g_isp_leaf{$as_name . "-" . $id} = $n 
		#	if(0 == $n->{bb} && $n != defined $ext_nodes{"$as_name-$id"} &&
		#		$n->{nlinks2} == 1);

		if (1 == $n->{bb} && $n->{nlinks} > 0)
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

	return if($n->{in_network} == 1);

	$as = $n->{as};

	# get the subnet addr and put node into subnet
	$n->{ip} =~ m/(\d+\.\d+\.\d+)/;
	$sn = $as . '-' . $1;
	#push @{$subnets{$sn}}, $n if $n->{in_network} != 1;
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
# Round-robin allocate access level CO routers to ISP topology
#
sub rr_access_network()
{
	my ($n) = undefined;
	my $next = 0;

	print "Linking access network...";

	while(@g_co)
	{
		for $l (values %g_isp_leaf)
		{
			$co = pop @g_co;

			#print "LEAF $l->{gid}: ", $l->{ip}, " lvl: $l->{level} \n";
			$l->{ip} =~ m/(\d+\.\d+\.\d+)/;
			$co->{as} = $l->{as};
			$co->{ip} = $1 . '.' . $co->{gid};
			$co->{level} = $l->{level} + 1;
			#print "  CO $co->{gid}: ", $co->{ip}, " lvl: $co->{level} \n";

			$co->{links2}{$l->{gid}} = $l;
			$co->{nlinks2}++;

			$l->{links2}{$co->{gid}} = $co;
			$l->{nlinks2}++;

			&create_network($co);

			foreach $ct (values %{$co->{links2}})
			{
				next if $ct == $l;

				$ct->{as} = $l->{as};
				$ct->{ip} = $1 . '.' . $ct->{gid};
				$ct->{level} = $co->{level} + 1;
				#print "\tCT/OG $ct->{gid}: ", $ct->{ip}, "\n";
			
				&create_network($ct);

				foreach $ha (values %{$ct->{links2}})
				{
					next if $ha == $co;

					$ha->{as} = $l->{as};
					$ha->{ip} = $1 . '.' . $ha->{gid};
					$ha->{level} = $ct->{level} + 1;

					#print "\t\tHA $ha->{gid}: ", $ha->{ip}, "\n";

					&create_network($ha);

					foreach $ag (values %{$ha->{links2}})
					{
						next if $ag == $ct;

						$ag->{as} = $l->{as};
						$ag->{ip} = $1 . '.' . $ag->{gid};
						$ag->{level} = $ha->{level} + 1;

						#print "\t\t\tAG $ag->{gid}: ", $ag->{ip}, "\n";

						&create_network($ag);
					}
				}
			}

			last if(!@g_co);
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
							$d->{ext2}{$n->{gid}} = $n;
					  		$d->{n_ext2}++;
						}
					}

					&create_network($d);

					# $max, since adding the link is +1
					if(!defined $n->{ext2}{$d->{gid}} &&
					   ($n->{nlinks2} + $n->{n_ext2}) < $max)
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

				next if !defined $d;
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

sub random($$)
{
	my $low = shift;
	my $high = shift;

	return int(rand($high)) + $low;
}

sub create_access
{
	my ($n, $d) = undefined;
	my ($nco, $nct, $nha, $ncg, $nag) = 0;
	my (@g_ct, @g_og) = ();

	$n_tot_nodes = 0 if !defined $n_tot_nodes;

	# read in COs, connect to CTs
	open(F, 'co.dat') or die "Unable to open co.dat: $!";
	printf("%-50s %-15s\n", "Processing Access:", "co.dat");

	while(<F>)
	{
		#print "Next CO: $n_tot_nodes\n";

		$n = new Node($n_tot_nodes);
		$n->{type} = 'c_router';
		$n->{gid} = $n_tot_nodes++;
		#$n->name("CO_" . $nco++);

		($jj, $kk, $ll, @_) = split(/\s+/);

		next if $#_ == -1;

		#print "\tconnected to CT: ";
		for(@_)
		{
			#print " $_";

			$d = $g_ct[$_];

			if(!defined $d)
			{
				#print "\tnew CT: $_ ($n_tot_nodes)\n";
				$d = $g_ct[$_] = new Node($_);
				$d->{type} = 'c_router';
				$d->{gid} = $n_tot_nodes++;
				#$d->name("CT_" . $nct++);

				#push @g_ct, $d;
			} else
			{
				#print "\told CT: $_ ($d->{gid})\n";
			}

			$n->{links2}{$d->{gid}} = $d;
			$n->{nlinks2}++;

			$d->{links2}{$n->{gid}} = $n;
			$d->{nlinks2}++;
		}

		#print "\n";
		push @g_co, $n;
	}

	close(F);

	# read in HARs, connect to CTs
	open(F, 'har.dat') or die "Unable to open har.dat: $!";
	printf("%-50s %-15s\n", "Processing Access:", "har.dat");

	my @g_ha = ();

	while(<F>)
	{
		next if $_ =~ /^List/;
		next if $_ =~ /^Hous/;

		print "Configuring HAR: $#g_ha \n" if $#g_ha % 100000 == 0;

		$n = new Node($n_tot_nodes);
		$n->{type} = 'c_router';
		$n->{gid} = $n_tot_nodes++;
		#$n->name("HA_" . $nha++);

		$_ = (split(/\s+/))[1];
		#print "\tconnected to CT:";

		#for(@_)
		{
			#print " $_";

			$d = $g_ct[$_];

			if(!defined $d)
			{
				#print "\tnew CT: $_ ($n_tot_nodes)\n";
				$d = $g_ct[$_] = new Node($_);
				$d->{type} = 'c_router';
				$d->{gid} = $n_tot_nodes++;
				#$d->name("CT_" . $nct++);
			}

			$n->{links2}{$d->{gid}} = $d;
			$n->{nlinks2}++;

			$d->{links2}{$n->{gid}} = $n;
			$d->{nlinks2}++;
		}

		#print "\n";
		push @g_ha, $n;
	}

	close(F);

	# read in OGRs, connect to COs
	open(F, 'ogr.dat') or die "Unable to open ogr.dat: $!";
	printf("%-50s %-15s\n", "Processing Access:", "ogr.dat");

	while(<F>)
	{
		next if $_ =~ /^List/;
		next if $_ =~ /^Offi/;

		#print "Next OG: $n_tot_nodes \n";

		$n = new Node($n_tot_nodes);
		$n->{type} = 'c_router';
		$n->{gid} = $n_tot_nodes++;
		#$n->name("CG_" . $nog++);

		$_ = (split(/\s+/))[1];
		#print "\tconnected to CO:";

		#for(@_)
		{
			#print " $_";

			$d = $g_co[$_];

			if(!defined $d)
			{
				#print "\tnew CO: $_ ($n_tot_nodes)\n";
				$d = $g_co[$_] = new Node($_);
				$d->{type} = 'c_router';
				$d->{gid} = $n_tot_nodes++;
				#$d->name("CO_" . $nco++);

				#push @g_co, $d;
			} else
			{
				#print "\told CO: $_ ($d->{gid})\n";
			}

			$n->{links2}{$d->{gid}} = $d;
			$n->{nlinks2}++;

			$d->{links2}{$n->{gid}} = $n;
			$d->{nlinks2}++;
		}

		#print "\n";
		push @g_og, $n;
	}

	close(F);

	# read in Agents
	open(F, 'agents.dat') or die "Unable to open agents.dat: $!";
	printf("%-50s %-15s\n", "Processing Access:", "agents.dat");

	my ($h, $g, $f) = (0, 0, 0);

	$g_next_bb = 0;
	my @keys = keys %g_isp_bb;
	my $c;
	my $g_nag = 1;

	while(<F>)
	{
		last if $g_nag % 1000000 == 0;

		next if $_ =~ /^List/;
		next if $_ =~ /^Agen/;

		print "Configuring Agent: $g_nag \n" if $g_nag % 100000 == 0;

		$c = $n = new Node($n_tot_nodes);
		$n->{type} = 'c_host';
		$n->{gid} = $n_tot_nodes++;
		#$n->name("AG_" . $nag++);

		($_, $h, $g, $f) = split(/\s+/);
		#print "\tconnected to: $h-$g ($f)\n";

		$d = $g_ha[$h];

		die "Bad HA: $h\n" if !defined $d;

		$n->{links2}{$d->{gid}} = $d;
		$n->{nlinks2}++;

		$d->{links2}{$n->{gid}} = $n;
		$d->{nlinks2}++;

		$g_nag++;

		# here, $n is the TCP server 'c_host'
		$n = new Node($n_tot_nodes);
		$n->{type} = 'c_host';
		$n->{gid} = $n_tot_nodes++;
		#$n->name("AG_SVR_" . ($nag -1));
		$n->{conn} = $c;

		# either connected to an OGR ($g) or ISP BB router
		if($g != -1)
		{
			# get the OG router
			$d = $g_og[$g];

			die "Invalid OGR!" if !defined $d;
		} else
		{
			$g_next_bb = 0 if(++$g_next_bb > $#keys);

			$d = $g_isp_bb{$keys[$g_next_bb]};

			#print "\tconnected to: $d->{gid} \n";
			
			die "Invalid BB router!" if !defined $d;
		
			$d->{ip} =~ m/(\d+\.\d+\.\d+)/;
			$n->{as} = $d->{as};
			$n->{ip} = $1 . '.' . $n->{gid};
			$n->{level} = $d->{level} + 1;

			&create_network($n);
		}

		$d->{links2}{$n->{gid}} = $n;
		$d->{nlinks2}++;

		$n->{links2}{$d->{gid}} = $d;
		$n->{nlinks2}++;

		push @g_ag_svr, $n;
	}

	close(F);

	printf("\n");
	printf("\t%-42s %-15d \n", "Total CO", $#g_co + 1);
	printf("\t%-42s %-15d \n", "Total CT", $#g_ct + 1);
	printf("\t%-42s %-15d \n", "Total HA", $#g_ha + 1);
	printf("\t%-42s %-15d \n", "Total OG", $#g_og + 1);
	printf("\n");
	printf("\t%-42s %-15d \n", "Total TCP Clients", $g_nag);
	printf("\t%-42s %-15d \n", "Total TCP Server", $#g_ag_svr + 1);
	printf("\n");
}

sub read_aliases
{
        my $f, $l, $uid, $ip, $dns, $last;

	chomp @al;

        foreach $f (@al)
	{
		open (F, "$f") or die "Could not open file: $f: $!";
		$f =~ m/.*\/(.*)\.al/;
		printf("%-50s %-15d\n", "Processing Alias:", $1);

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
