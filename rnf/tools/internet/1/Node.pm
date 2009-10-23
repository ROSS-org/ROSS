package Node;

use UNIVERSAL qw(isa);
use FileHandle;

@delays = qw(.01 .02 .03 .005 .005 .005 .005 .005 .005 .005 .005);
@bandwidths = qw(9920000000 2480000000 620000000 155000000 45000000 1500000 1500000 500000 70000);

sub new
{
	my $class = shift;

	return bless { id => shift, plus => 0, bb => 0, disconnected => 0, ext => undef,
		       type => "c_router", in_network => 0, nlinks =>0, n_ext2 => 0
		     }, $class;
}

sub name
{
	my $n = shift;
	my $name = shift;

	$n->{name} = $name if $name ne "";

	$n->{disconnected} = 1 if $n->{name} eq "\@?";
	$n->{disconnected} = 2 if $n->{name} eq "\@T";

	return $n->{name};
}

sub connected
{
	my $n = shift;

	if($n->{disconnected} == 2)
	{
		return "ambiguous";
	}

	return $n->{disconnected} ? "false" : "true";
}

sub ext
{
	my $n = shift;
	my $var = shift;
	my $key = shift;

	if(isa($var, 'Node'))
	{
		if($n->{ext}{$key} != 1)
		{
			my $x = $n->{ext}{$key};

			# May be done already by other, alias node (same node)
			next if $x->{ip} eq $var->{ip};

			print STDERR "key: $key \n";
			print STDERR "keys: ", join(" ", keys %{$n->{ext}}), "\n";
			print STDERR "vals: ", join(" ", values %{$n->{ext}}), "\n";
			print STDERR "not 1: $var->{as}-$var->{id}\n";
			print STDERR "is: $x->{as}-$x->{id} \n";
			print STDERR "was: ", $n->{ext}{"$var->{as}-$var->{id}"}, "\n";
			exit 0;
		}

		$n->{ext}{$key} = $var;
		return $var;
	} else
	{
		print "Error: not a Node! $var \n";
		exit(0);
	}
}

sub print
{
	my $n = shift;
	my $fh = shift;

	my $x, $bw, $delay;
	my ($have_bgp, $have_ospf) = (0, 0);
	my ($bgp, $ospf, $ip, $top, $links) = ("", "", "", "", "");
	my $nlinks = 0;
	my %d;

	#$nlinks = $n->{nlinks2} + $n->{n_ext2};

	# Setup IP layer XML
	$ip = "\t\t\t\t\t\t<layer name='ip' level='network' />\n";

	#$fh->printf("\t\t\t\t<node id='$n->{nid}' loc='$n->{name}' links='$nlinks' type='c_router' name='$n->{ip}' lvl='$n->{level}'>\n");
	#$fh->printf("\t\t\t\t<node id='$n->{nid}' links='$nlinks' type='c_router'>\n");
	# Internal links go to OSPF nodes..
	$ospf = "\t\t\t\t\t<stream port='23'>\n";
	$ospf .= "\t\t\t\t\t\t<layer name='ospf' level='network'>\n";

	%d = %{$n->{links2}};
	foreach $x (values %{$n->{links2}})
	{
		$bw = $bandwidths[$n->{level}];
		$delay = $delays[$n->{level}];

		#$fh->printf("\t\t\t\t\t<link src='$n->{nid}' addr='$x->{nid}' bandwidth='$bw' delay='$delay' status='%s' />\n", ($n->connected eq "true" ? "up" : "down"));
		#$fh->printf("\t\t\t\t\t<link src='$n->{nid}' addr='$x->{nid}' bandwidth='$bw' delay='$delay' cost='1' status='up' />\n");
		$links .= "\t\t\t\t\t<link src='$n->{nid}' addr='$x->{nid}' bandwidth='$bw' delay='$delay' cost='1' status='up' />\n";
		$nlinks++;

		#$ospf .= "\t\t\t\t\t\t\t<interface src='$n->{nid}' addr='$x->{nid}'/> \n";
		$ospf .= "\t\t\t\t\t\t\t<interface src='$n->{nid}' addr='$x->{nid}'/> \n";

		$have_ospf = 1;
	}

	# External links go to BGP nodes..
	if($n->{n_ext2} > 0)
	{
		$bgp = "\t\t\t\t\t<stream port='169'>\n";
		$bgp .= "\t\t\t\t\t\t<layer name='bgp' level='network'>\n";

		foreach $x (values %{$n->{ext2}})
		{
			# If not a node, then this ext link did not go to
			# a node/router in any of our ISPs

			if(isa($x, 'Node'))
			{
				$bw = $bandwidths[$n->{level}];
				$delay = $delays[$n->{level}];

				$bgp .= "\t\t\t\t\t\t\t<neighbor addr='$x->{nid}' />\n";

				if(!defined $n->{links2}{$x->{gid}})
				{
					#$fh->printf("\t\t\t\t\t<link src='%d' addr='%d' bandwidth='$bw' delay='$delay' cost='1' status='up' />\n", $n->{nid}, $x->{nid});
					$links .= "\t\t\t\t\t<link src='$n->{nid}' addr='$x->{nid}' bandwidth='$bw' delay='$delay' cost='1' status='up' />\n";
					$nlinks++;

					# here $ospf .= "\t\t\t\t\t\t\t<interface src='$n->{nid}' addr='$x->{nid}'/>\n";
					#$ospf .= "h_int='2' h_bw='.0001' rdead_int='2' />\n";
					$have_ospf = 1;
				}
			}
		}

		$have_bgp = 1;
	}

	$n->{level} =~ /r(\d)/;

	$top = "\t\t\t\t<node id='$n->{nid}' links='$nlinks' type='c_router' lvl='$1'>\n";

	$ospf .= "\t\t\t\t\t\t</layer>\n";
	$ospf .= $ip;
	$ospf .= "\t\t\t\t\t</stream>\n";

	$bgp .= "\t\t\t\t\t\t</layer>\n";
	$bgp .= $ip;
	$bgp .= "\t\t\t\t\t</stream>\n";

	$fh->printf("%s", $top);
	$fh->printf("%s", $links);
	$fh->printf("%s", $ospf) if $have_ospf == 1;
	$fh->printf("%s", $bgp) if $have_bgp == 1;

	$fh->printf("\t\t\t\t</node>\n");
	#printf("\t\t\t\t</node>\n");

	return $fh;
}

1;
