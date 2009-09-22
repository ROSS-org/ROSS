#!/usr/bin/perl

my $nmachines = 2;
my $tab = 0;

`rm -f xml`;

open (F, ">xml") or die "Could not open xml: $!";

print F "<rossnet>\n";

print F "<as id=\'0\' frequency=\'1\'>\n";
print F "<area id=\'0\'>\n";
print F "<subnet id=\'0\'>\n";

for($i = 0; $i < $nmachines; $i++)
{
	print F "<node id=\'$i\' links=\'1\' type=\'c_router\'>\n";
	print F "<link src=\'$i\' addr=\'", $i+1, "\' bandwidth=\'1\' delay=\'1\' status=\'up\' />\n";

	print F "<stream port=\'2\'>\n";
	print F "<layer name=\'ip\' level=\'network\' />\n";
	print F "</stream>\n";

	print F "</node>\n";
}

print F "<node id=\'$i\' links=\'1\' type=\'c_router\'>\n";
print F "<link src=\'$i\' addr=\'", $i-1, "\' bandwidth=\'1\' delay=\'1\' status=\'up\' />\n";

print F "<stream port=\'2\'>\n";
print F "<layer name=\'ip\' level=\'network\' />\n";
print F "</stream>\n";

print F "</node>\n";

print F "</subnet>\n";
print F "</area>\n";
print F "</as>\n";
print F "</rossnet>\n";

close F;
