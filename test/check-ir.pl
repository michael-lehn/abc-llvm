#!/usr/bin/perl
#
# Checks properties of a function in an LLVM IR file (used by
# test/run-tests.sh for the tests in test/ir).
#
# usage: perl check-ir.pl FILE.ll FUNCTION SIZE-ATTRIBUTES OPTIMIZED
#
#   SIZE-ATTRIBUTES  the size optimization attributes of FUNCTION: "none" or a
#                    space separated list of "optsize" and "minsize"
#   OPTIMIZED        "yes": the body of FUNCTION contains no alloca (local
#                    variables were promoted to registers); "no": it does
#
# Prints a description of each mismatch and exits with status 1 if there is
# one.

use strict;
use warnings;

my ($file, $function, $size, $optimized) = @ARGV;
die "usage: $0 FILE.ll FUNCTION SIZE-ATTRIBUTES OPTIMIZED\n"
    unless defined $optimized;

open my $fh, '<', $file or die "$file: $!\n";
my @line = <$fh>;
close $fh;

my (@body, @groups, %group);
for (my $i = 0; $i < @line; ++$i) {
    if ($line[$i] =~ /^define .*@\Q$function\E\(.*\)(.*)\{\s*$/) {
	# attribute group references after the parameter list, e.g. "#0"
	@groups = $1 =~ /#(\d+)/g;
	while (++$i < @line && $line[$i] !~ /^\}/) {
	    push @body, $line[$i];
	}
    }
    $group{$1} = $2 if $line[$i] =~ /^attributes #(\d+) = \{(.*)\}/;
}

my @error;
if (!@body) {
    push @error, "no definition of function '$function' found";
} else {
    my %attr;
    for my $g (@groups) {
	$attr{$_} = 1 for grep { /^(optsize|minsize)$/ } split ' ', $group{$g} // '';
    }
    my $got = join(' ', sort keys %attr) || 'none';
    my $want = join(' ', sort split ' ', $size) || 'none';
    push @error, "size attributes of '$function': got '$got', expected '$want'"
        if $got ne $want;

    my $has_alloca = grep { /= alloca / } @body;
    if ($optimized eq 'yes' && $has_alloca) {
	push @error, "'$function' is not optimized (contains alloca)";
    } elsif ($optimized eq 'no' && !$has_alloca) {
	push @error, "'$function' is optimized (no alloca), expected -O0 code";
    } elsif ($optimized !~ /^(yes|no)$/) {
	push @error, "OPTIMIZED must be 'yes' or 'no', got '$optimized'";
    }
}

print "$_\n" for @error;
exit(@error ? 1 : 0);
