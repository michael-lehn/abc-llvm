#!/usr/bin/perl
#
# Validates test IDs and issue references (run by test/run-tests.sh).
#
# usage: perl validate-tests.pl TESTDIR [UNIT-TEST-BINARY...]
#
# Checks:
#  - every test has exactly one stable ID (// TEST: Tnnnn) and IDs are unique
#    (this includes the IDs of CHECK_KNOWN_BUG checks in unit tests)
#  - XFAIL tests refer to an existing OPEN BUG issue, DESIGN tests to an
#    existing OPEN DESIGN issue; design tests record what is OBSERVED
#  - issue IDs in TESTDIR/bugs_and_open_design_questions.md are unique and
#    the "Covered by tests" lists match the tests (ID, kind, classification)
#  - directives are used only where they apply (ERROR in test/fail, DESIGN
#    and OBSERVED in test/design, the IR test directives in test/ir, ARGS
#    also in test/run and test/fail), at most one XFAIL per test, required directives are
#    present
#  - driver tests (test/driver/*.sh) have one '# TEST:' and at most one
#    '# XFAIL:' line
#  - test/examples.list has one 'TEST:' line, existing DIR and EXCLUDE
#    directories, and XFAIL lines for ABC programs below a DIR
#  - .c and .out companion files belong to a test in test/run
#  - every unit test binary has a source file TESTDIR/unit/NAME.cpp
#  - issue references (#nnnn) in README.md exist
#
# Prints one line per error and exits with status 1 if there are errors.

use strict;
use warnings;

my $dir = shift // 'test';
my $registry = "$dir/bugs_and_open_design_questions.md";
my @errors;
my %test;    # id -> { file, kind, class, issue }

sub add_test
{
    my ($id, %info) = @_;
    if ($id !~ /^T\d{4}$/) {
	push @errors, "$info{file}: invalid test ID '$id'";
    } elsif (exists $test{$id}) {
	push @errors, "$info{file}: test ID $id already used by $test{$id}{file}";
    } else {
	$test{$id} = \%info;
    }
}

my %kind_of_dir = (run => 'runtime', fail => 'reject', design => 'design',
                   ir => 'ir');

for my $sub (sort keys %kind_of_dir) {
    for my $file (sort glob("$dir/$sub/*.abc")) {
	open my $fh, '<', $file or die "$file: $!";
	my (@id, @xfail, @design, @observed, @error, %ir);
	while (<$fh>) {
	    push @id, $1 if m{^// TEST: *(\S+)};
	    push @xfail, $1 if m{^// XFAIL: *(\S*)};
	    push @design, $1 if m{^// DESIGN: *(\S*)};
	    push @observed, $1 if m{^// OBSERVED: *(\S*)};
	    push @error, $1 if m{^// ERROR: *(.*)};
	    push @{ $ir{$1} }, $2
	        if m{^// (ARGS|FUNCTION|SIZE-ATTRIBUTES|OPTIMIZED|IR-MATCH):(.*)};
	}
	close $fh;
	if (@id != 1) {
	    push @errors, "$file: expected exactly one '// TEST:' line";
	    next;
	}
	my $kind = $kind_of_dir{$sub};
	my $class = 'PASS';
	my $issue;
	push @errors, "$file: more than one '// XFAIL:' line" if @xfail > 1;
	if ($sub eq 'design') {
	    $class = 'DESIGN';
	    push @errors, "$file: design test needs '// DESIGN: #nnnn'"
	        if @design != 1;
	    push @errors, "$file: design test needs "
	        . "'// OBSERVED: ACCEPTED' or '// OBSERVED: REJECTED'"
	        if @observed != 1 || $observed[0] !~ /^(ACCEPTED|REJECTED)$/;
	    push @errors, "$file: design test can not be XFAIL" if @xfail;
	    $issue = $design[0];
	} else {
	    push @errors, "$file: '// DESIGN:' only allowed in test/design"
	        if @design || @observed;
	    if (@xfail) {
		$class = 'XFAIL';
		$issue = $xfail[0];
	    }
	}
	if ($sub eq 'fail') {
	    push @errors, "$file: reject test needs exactly one '// ERROR: regex'"
	        if @error != 1 || $error[0] !~ /\S/;
	} elsif (@error) {
	    push @errors, "$file: '// ERROR:' only allowed in test/fail";
	}
	if ($sub eq 'ir') {
	    push @errors, "$file: IR test needs exactly one '// ARGS:' line"
	        if @{ $ir{ARGS} // [] } != 1;
	    my @fn = grep { $ir{$_} } qw(FUNCTION SIZE-ATTRIBUTES OPTIMIZED);
	    push @errors, "$file: IR test needs FUNCTION, SIZE-ATTRIBUTES and "
	        . "OPTIMIZED together (each once) or none of them"
	        if @fn && (@fn != 3 || grep { @{ $ir{$_} } != 1 } @fn);
	    push @errors, "$file: IR test needs FUNCTION or IR-MATCH"
	        unless @fn || $ir{'IR-MATCH'};
	    if (@fn) {
		my $size = join ' ', sort split ' ',
		    $ir{'SIZE-ATTRIBUTES'}[0] // '';
		push @errors, "$file: SIZE-ATTRIBUTES must be 'none', 'optsize' "
		    . "or 'optsize minsize'"
		    unless $size =~ /^(none|optsize|minsize optsize)$/;
		push @errors, "$file: OPTIMIZED must be 'yes' or 'no'"
		    unless ($ir{OPTIMIZED}[0] // '') =~ /^\s*(yes|no)\s*$/;
	    }
	} elsif (($sub eq 'fail' || $sub eq 'run') && keys %ir == 1
	         && $ir{ARGS}) {
	    # additional options of a runtime or reject test
	    push @errors, "$file: more than one '// ARGS:' line"
	        if @{ $ir{ARGS} } > 1;
	} elsif (%ir) {
	    push @errors, "$file: '// " . join(':', sort keys %ir)
	        . ":' only allowed in test/ir"
	        . ($ir{ARGS} && keys %ir == 1 ? ", test/run and test/fail" : "");
	}
	add_test($id[0], file => $file, kind => $kind, class => $class,
	         issue => $issue);
    }

    # companion files: NAME.c and NAME.out belong to test/run/NAME.abc
    for my $file (sort glob("$dir/$sub/*.c"), sort glob("$dir/$sub/*.out")) {
	(my $abc = $file) =~ s/\.(c|out)$/.abc/;
	if ($sub ne 'run') {
	    push @errors, "$file: companion files are only supported in test/run";
	} elsif (!-f $abc) {
	    push @errors, "$file: orphaned companion file (no $abc)";
	}
    }
}

# unit test binaries (arguments) need a source file test/unit/NAME.cpp
for my $exe (@ARGV) {
    (my $base = $exe) =~ s{.*/}{};
    push @errors, "unit test binary $exe has no source file $dir/unit/$base.cpp"
        unless -f "$dir/unit/$base.cpp";
}

# driver tests (test/driver) declare their ID with '# TEST: Tnnnn' and a
# known bug with '# XFAIL: #nnnn'
for my $file (sort glob("$dir/driver/*")) {
    if ($file !~ /\.sh$/) {
	push @errors, "$file: driver tests must be '.sh' scripts";
	next;
    }
    open my $fh, '<', $file or die "$file: $!";
    my (@id, @xfail);
    while (<$fh>) {
	push @id, $1 if m{^# TEST: *(\S+)};
	push @xfail, $1 if m{^# XFAIL: *(\S*)};
    }
    close $fh;
    if (@id != 1) {
	push @errors, "$file: expected exactly one '# TEST:' line";
	next;
    }
    push @errors, "$file: more than one '# XFAIL:' line" if @xfail > 1;
    add_test($id[0], file => $file, kind => 'driver',
             class => @xfail ? 'XFAIL' : 'PASS', issue => $xfail[0]);
}

# tool checks (test/tool) declare their ID with '# TEST: Tnnnn'
for my $file (sort glob("$dir/tool/*")) {
    if ($file !~ /\.(pl|sh)$/) {
	push @errors, "$file: tool checks must be '.pl' or '.sh' scripts";
	next;
    }
    open my $fh, '<', $file or die "$file: $!";
    my @id = map { m{^# TEST: *(\S+)} ? $1 : () } <$fh>;
    close $fh;
    if (@id != 1) {
	push @errors, "$file: expected exactly one '# TEST:' line";
    } else {
	add_test($id[0], file => $file, kind => 'tool', class => 'PASS');
    }
}

# examples (test/examples.list): one test ID for all examples, known failures
# with their own test ID, issue and file below a DIR and not below an EXCLUDE
{
    my $file = "$dir/examples.list";
    my $top = "$dir/..";
    open my $fh, '<', $file or die "$file: $!";
    my (@id, @dirs, @exclude, @xfail);
    while (<$fh>) {
	next if /^\s*(#|$)/;
	if (/^TEST: *(\S+)\s*$/) {
	    push @id, $1;
	} elsif (/^DIR: *(\S+)\s*$/) {
	    push @dirs, $1;
	} elsif (/^EXCLUDE: *(\S+)\s*$/) {
	    push @exclude, $1;
	} elsif (/^XFAIL: *(\S+) +(\S+) +(\S+)\s*$/) {
	    push @xfail, [$1, $2, $3];
	} else {
	    push @errors, "$file:$.: malformed line";
	}
    }
    close $fh;
    if (@id != 1) {
	push @errors, "$file: expected exactly one 'TEST:' line";
    } else {
	add_test($id[0], file => $file, kind => 'example', class => 'PASS');
    }
    for my $d (@dirs, @exclude) {
	push @errors, "$file: directory '$d' does not exist" unless -d "$top/$d";
    }
    for my $d (@dirs) {
	push @errors, "$file: DIR '$d' is below another DIR"
	    if grep { $_ ne $d && index("$d/", "$_/") == 0 } @dirs;
    }
    for my $e (@exclude) {
	push @errors, "$file: EXCLUDE '$e' is not below a DIR"
	    unless grep { index("$e/", "$_/") == 0 } @dirs;
    }
    my %seen;
    for my $x (@xfail) {
	my ($tid, $iid, $path) = @$x;
	push @errors, "$file: $path has more than one XFAIL line"
	    if $seen{$path}++;
	push @errors, "$file: $path does not exist or is not a regular file"
	    unless -f "$top/$path" && !-l "$top/$path";
	push @errors, "$file: $path is not an ABC program below a DIR"
	    unless $path =~ /\.abc$/
	    && grep { index($path, "$_/") == 0 } @dirs;
	push @errors, "$file: $path is below an EXCLUDE directory"
	    if grep { index($path, "$_/") == 0 } @exclude;
	add_test($tid, file => $file, kind => 'example', class => 'XFAIL',
	         issue => $iid);
    }
}

for my $file (sort glob("$dir/unit/*.cpp")) {
    open my $fh, '<', $file or die "$file: $!";
    my $text = do { local $/; <$fh> };
    close $fh;
    my @id = $text =~ m{^// TEST: *(\S+)}mg;
    if (@id != 1) {
	push @errors, "$file: expected exactly one '// TEST:' line";
    } else {
	add_test($id[0], file => $file, kind => 'unit', class => 'PASS');
    }
    while ($text =~ m{CHECK_KNOWN_BUG\(\s*"([^"]*)"\s*,\s*"([^"]*)"}g) {
	add_test($1, file => $file, kind => 'unit', class => 'XFAIL',
	         issue => $2);
    }
}

# parse registry
my %issue;    # id -> { title, type, status, covered => [ [test, kind, class] ] }
open my $rh, '<', $registry or die "$registry: $!";
my $current;
while (<$rh>) {
    if (/^Issue #(\d{4}): *(.+)$/) {
	$current = $1;
	push @errors, "$registry: issue #$current defined twice"
	    if exists $issue{$current};
	$issue{$current} = { title => $2 };
    } elsif (/^Issue /) {
	push @errors, "$registry:$.: malformed issue header";
    } elsif (defined $current && /^Type: *(\S+)/) {
	push @errors, "$registry: #$current: invalid type '$1'"
	    if $1 !~ /^(BUG|DESIGN)$/;
	$issue{$current}{type} = $1;
    } elsif (defined $current && /^Status: *(\S+)/) {
	push @errors, "$registry: #$current: invalid status '$1'"
	    if $1 !~ /^(OPEN|RESOLVED|WONTFIX)$/;
	$issue{$current}{status} = $1;
    } elsif (defined $current && /^Covered by tests: *(.*)$/) {
	my $list = $1;
	$issue{$current}{covered} = [];
	next if $list eq 'none';
	for my $item (split /,\s*(?=#T)/, $list) {
	    if ($item =~ /^#(T\d{4}) \((runtime|reject|design|ir|unit|driver|tool|example), (PASS|XFAIL|DESIGN)\)$/) {
		push @{ $issue{$current}{covered} }, [$1, $2, $3];
	    } else {
		push @errors, "$registry: #$current: malformed test reference '$item'";
	    }
	}
    }
}
close $rh;

for my $id (sort keys %issue) {
    for my $field (qw(type status covered)) {
	push @errors, "$registry: #$id: missing '$field'"
	    unless defined $issue{$id}{$field};
    }
}

# tests -> issues
for my $id (sort keys %test) {
    my $t = $test{$id};
    next if $t->{class} eq 'PASS';
    my $ref = $t->{issue} // '';
    if ($ref !~ /^#(\d{4})$/) {
	push @errors, "$t->{file}: $id: $t->{class} needs an issue reference '#nnnn', got '$ref'";
	next;
    }
    my $i = $issue{$1};
    if (!$i) {
	push @errors, "$t->{file}: $id: unknown issue $ref";
	next;
    }
    my $want = $t->{class} eq 'XFAIL' ? 'BUG' : 'DESIGN';
    push @errors, "$t->{file}: $id: $t->{class} test refers to $ref of type "
        . ($i->{type} // '?') . ", expected $want"
        if ($i->{type} // '') ne $want;
    push @errors, "$t->{file}: $id: $t->{class} test refers to $ref with status "
        . ($i->{status} // '?') . ", expected OPEN"
        if ($i->{status} // '') ne 'OPEN';
    push @errors, "$t->{file}: $id: not listed in 'Covered by tests' of $ref"
        unless grep { $_->[0] eq $id } @{ $i->{covered} // [] };
}

# issues -> tests
for my $iid (sort keys %issue) {
    for my $c (@{ $issue{$iid}{covered} // [] }) {
	my ($tid, $kind, $class) = @$c;
	my $t = $test{$tid};
	if (!$t) {
	    push @errors, "$registry: #$iid: unknown test #$tid";
	    next;
	}
	push @errors, "$registry: #$iid: #$tid is a $t->{kind} test, not $kind"
	    if $t->{kind} ne $kind;
	push @errors, "$registry: #$iid: #$tid is $t->{class}, not $class"
	    if $t->{class} ne $class;
	push @errors, "$registry: #$iid: #$tid refers to "
	    . ($t->{issue} // 'no issue') . ", not #$iid"
	    if $class ne 'PASS' && ($t->{issue} // '') ne "#$iid";
    }
}

# issue references in the README must exist
my $readme = "$dir/../README.md";
if (open my $rm, '<', $readme) {
    while (<$rm>) {
	while (/(?<![\w#&])#(\d{4})\b/g) {
	    push @errors, "$readme:$.: unknown issue #$1" unless $issue{$1};
	}
    }
    close $rm;
}

print "$_\n" for @errors;
exit(@errors ? 1 : 0);
