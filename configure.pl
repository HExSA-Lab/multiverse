#!/usr/bin/perl -w
use YAML;
use Data::Dumper;

my $infile = "funcs.yaml";

my $yaml = YAML::LoadFile($infile) or die "Could not open YAML file $infile $!\n";

my @funcs = @{$yaml->{functions}};

open (WRAPPER, ">", "wrap.mk");

print WRAPPER "LDFLAGS+= \\\n";

# remove no overrides
for (my $i = 0; $i < $#funcs+1; $i = $i+1) {
    if ($funcs[$i]->{override} ne "true") {
        splice (@funcs, $i, 1);
        $i = $i-1;
    } 
}

foreach my $func (@funcs) {

    print WRAPPER "    -Wl,--wrap,$func->{name}";

    if ($func == $funcs[$#funcs]) {
        print WRAPPER "\n";
    } else {
        print WRAPPER " \\\n";
    }

}

close WRAPPER;

