#!/usr/bin/perl

open (INPUT, "$ARGV[0]") or die "Unable to open $ARGV[0]: $!";
open (XML, ">$ARGV[1]") or die "Unable to open $ARGV[1]: $!";

@_ = <INPUT>;
my $nid = 0;

print XML <<OUT;
<rossnet>
	<as id='0' frequency='1'>
		<area id='0' name=''>
			<subnet id='0' name=''>
OUT

my %conn = ();

while ($_ = shift @_)
{
	chomp $_;

	if($_ =~ /^router/)
	{
		$nlinks = shift @_;
		chomp $nlinks;

		print XML "\t\t\t\t<node id='$nid' links='$nlinks' type='c_router'>\n";

		$iface = "";
		for($i = 0; $i < $nlinks; $i++)
		{
			$link = shift @_;
			chomp $link;

			($bw, $delay, $dst) = split(/ /, $link);

			print XML "\t\t\t\t\t<link src='$nid' addr='$dst' cost='1' bandwidth='$bw' delay='$delay' status='up'/>\n";
                        $iface .= "\t\t\t\t\t\t\t<interface src='$nid' addr='$dst'/>\n";
		}

		print XML <<OUT;
					<stream port='80'>
                                                <layer name='ip' level='network' />
                                        </stream>
                                </node>
OUT

		$nid++;
	}

	if($_ =~ /^host/)
	{
		($bw, $delay, $dst, $type, $from, $something) = split(/ /, shift @_);

		$conn{$from} = $dst;

		print XML <<OUT;
				<node id='$nid' links='1' type='c_host'>
					<link src='$nid' addr='$dst' cost='1' bandwidth='$bw' delay='$delay' status='up'/>
					<stream port='80'>
                                                <layer name='tcp' level='transport'>
							<mss>1500</mss>
							<recv_wnd>32</recv_wnd>
                                                </layer>
                                                <layer name='ip' level='network' />
                                        </stream>
                                </node>
OUT

		$nid++;
	}
}

print XML <<OUT;
			</subnet>
		</area>
	</as>
OUT

for(keys %conn)
{
	print XML <<OUT;
	<connect src='$_' dst='$conn{$_}' />
OUT
}

print XML "</rossnet>\n";

close (INPUT);
close (XML);
