#!/usr/bin/env perl
use strict;
use warnings;

use Data::Dumper;
use File::Temp qw//;
use File::Spec qw//;
use File::Basename qw(dirname);
use File::Path qw(remove_tree);
use Cwd qw(abs_path getcwd);
use IPC::Run3;
use Parallel::ForkManager;

# Try a matrix of all build options.  The various req's are intended to
# be quick&dirty tests to see if it's worth trying an option on the
# system.
my %OPTS = (
	USE_XPM => {
		desc => 'XPM support',
		req_i => 'X11/xpm.h',
	},
	USE_JPEG => {
		desc => 'JPEG support',
		req_i => 'jpeglib.h',
	},
	USE_M4 =>  {
		desc => 'M4 support',
		req_e => 'm4',
	},
	USE_RPLAY =>  {
		desc => 'librplay sound support',
		req_i => 'rplay.h',
	},
	# USE_SREGEX is no longer actually an _option_
	USE_EWMH =>  {
		desc => 'EWMH support',
	},
);

# Default include paths to check
my @INCDIRS = qw( /usr/include /usr/local/include );

# Assume normal $PATH is where executables should be looked for.

# Build from this tree
my @mypath = File::Spec->splitdir(abs_path(dirname($0)));
while(@mypath && ! -r "@{[File::Spec->catfile(@mypath, 'ctwm.h')]}")
{
	pop @mypath;
}
die "Didn't find ctwm.h!" unless @mypath;
my $mypath = File::Spec->catdir(@mypath);
my $ORIGDIR = getcwd();



# Command line args
my %CLOPTS = parse_clargs();
$CLOPTS{jobs} //= 1;

# Extra include dirs given?
push @INCDIRS, @{$CLOPTS{include}} if $CLOPTS{include};


# Allow spec'ing just a subset on the command line.
my @DO_OPTS;
if(@ARGV)
{
	# Command-line given
	for my $o (@ARGV)
	{
		die "Unknown option '$o'\n" unless $OPTS{$o};
		push @DO_OPTS, $o;
	}
}
else
{
	@DO_OPTS = keys %OPTS;
}
die "No options to work with" unless @DO_OPTS;
@DO_OPTS = sort @DO_OPTS;



# OK, now see which options are usable.
print "Checking over options...\n" if $CLOPTS{verbose};
my @use = check_opts(@DO_OPTS);

# Which aren't we using?
my @skip = sort grep { my $x = $_; !grep { $_ eq $x } @use } keys %OPTS;


# Summarize
print "Skipping options: " . join(' ', @skip) . "\n" if @skip;
print "Using options: " . join(' ', @use) . "\n" if @use;
die "No usable options!\n" unless @use;
print "Building from $mypath\n";



# Build our list of options
my @builds;
if($CLOPTS{all})
{
	# Generate powerset
	my $dbgshift = 2;
	my $_dbgret = sub {
		printf("%*s%s\n", $dbgshift, "", "Rets:");
		printf("%*s%s\n", $dbgshift, "", Dumper \@_);
	};

	# Build all subsets.  Each invocation returns an array of hashes of
	# the build options groupings.
	my $bss;
	$bss = sub {
		$dbgshift++;
		#print "  bss(" . join(" ", @_) . ")\n";

		# Nothing left?  We're done.
		return () if @_ == 0;

		# Else pull the first thing off the list and make its pair.
		my $base = shift @_;
		my @base = ( {$base => 0}, {$base => 1} );
		#$_dbgret->(@base) if @_ == 0;

		# Was that the last?  Then we're done.
		return @base if @_ == 0;

		# Else there's more.  Recurse.
		my @subsubs = $bss->(@_);
		$dbgshift--;

		# Pair up each of our @base with each of the returned.
		my @rets;
		for my $b (@base)
		{
			for my $s (@subsubs)
			{
				push @rets, {%$b, %$s};
			}
		}
		#$_dbgret->(@rets);
		return @rets;
	};

	@builds = $bss->(@use);
}
else
{
	# Just try on/off on each individually
	push @builds, {$_ => 0}, {$_ => 1} for @use;
}

print("Builds to run: @{[scalar @builds]}\n");

if($CLOPTS{verbose})
{
	print((map "  @{[mk_build_str($_)]}\n", @builds), "\n");
}


# Create a tempdir
my $tmpbase = $ENV{TMPDIR} // "/tmp";
my $tmpdir = File::Temp->newdir("ctwm-opts-XXXXXXXX",
		DIR => $tmpbase, CLEANUP => !$CLOPTS{keep});
print "Testing in $tmpdir...\n";


# Now setup the actual running
my ($suc, $fail) = (0,0);
my @fails;

my $fm = Parallel::ForkManager->new($CLOPTS{jobs}, $tmpdir);
$fm->run_on_finish(sub{
	my ($pid, $excode, $ident, $exsig, $coredump, $bret) = @_;
	unless(defined $bret && ref $bret eq 'HASH')
	{
		warn "Child $ident didn't return anything!";
		$fail++;
		push @fails, "(unknown: $ident)";
		return;
	}

	# If we died from a signal, give up totally
	if($bret->{sig})
	{
		print $bret->{errstr};
		die "$ident: Signal $bret->{sig} in child, dying...";
	}

	if($bret->{ok})
	{
		# Succeeded; print success and clean up unless we're --keep'ing
		print map { "    $ident: $_\n" } @{$bret->{stdstr}};

		$suc++;
		remove_tree($bret->{tstdir}) unless $CLOPTS{keep};
	}
	else
	{
		# Failed; print failures and mark things to not be cleaned up.
		print map { "    $ident: $_\n" } @{$bret->{stdstr}};
		print $bret->{errstr};

		$fail++;
		push @fails, $bret->{bstr};
		$tmpdir->unlink_on_destroy(0) unless $CLOPTS{keep};
	}
	return;
});

my $sdn = 0;
$fm->start_child(++$sdn, sub { return one_build($_, $sdn, \%CLOPTS); })
		for @builds;
$fm->wait_all_children();


print "\n\n$suc succeeeded, $fail failed.\n";
if(@fails)
{
	print " Failed option combinations:\n";
	print "  $_\n" for @fails;
	print "\nBuild artifacts left in $tmpdir\n";
	exit 1;
}
exit 0;




#
# Remainder are the funcs used above.
#



# Command-line parsing
sub parse_clargs
{
	use Getopt::Long qw(:config no_ignore_case bundling);

	my @clopts = (
		'include|I=s@',  # Extra include path(s)
		'keep|k',        # Keep output dir
		'verbose|v',     # Verbosity
		'jobs|j=i',      # Parallel jobs to run
		'all|a',         # Try all combos rather than all options
		'dryrun|d',      # Don't exec anything
	);
	my %opts;
	GetOptions(\%opts, @clopts);

	return %opts;
}


# Check over the list of options we're being asked for, make sure they're
# all options we know about, and weed out any that we don't think we can
# do on the current system.
sub check_opts
{
	my @do_opts = @_;
	my @use;

	OCHECK: for my $k (@do_opts)
	{
		my $_o = $OPTS{$k};

		# Should be impossible; command-line chosen opts are checked
		# before we get here...
		die "Unexpected option: $k" unless $_o;
		my %o = %$_o;

		if($o{req_e})
		{
			# Requires an executable
			my $e = $o{req_e};

			# File::Which is one way, but it's not in core, and it's easy
			# enough to hack a version, so...
			my $rstr = `which $e`;
			if($? >> 8)
			{
				# Non-zero exit; not found
				print "  $k: $e not found in path, skipping.\n" if $CLOPTS{verbose};
			}
			else
			{
				print "  $k: $e OK, adding.\n" if $CLOPTS{verbose};
				push @use, $k;
			}
			next OCHECK;
		}

		if($o{req_i})
		{
			# Requires an include file
			my $i = $o{req_i};

			for my $d (@INCDIRS)
			{
				if(-r "$d/$i")
				{
					print"  $k: $i found in $d, adding.\n" if $CLOPTS{verbose};
					push @use, $k;
					next OCHECK;
				}
			}

			print "  $k: $i not found, skipping.\n" if $CLOPTS{verbose};
			next OCHECK;
		}

		# Everything else has no requirements we bother to check
		print "  $k: OK\n" if $CLOPTS{verbose};
		push @use, $k;
	}

	return @use;
}


# Various ways we generate cmake-y option defs
sub mkopts { return "-D$_[0]=@{[$_[1] ? 'ON ' : 'OFF']}"; }
sub mk_build_strs
{
	my $opts = shift;
	die "Bad coder!  Bad!" unless ref $opts eq 'HASH';
	return map { mkopts($_, $opts->{$_}) } sort keys %$opts;
}
sub mk_build_str
{
	my $opts = shift;
	die "Bad coder!  Bad!" unless ref $opts eq 'HASH';
	return join(' ', mk_build_strs($opts));
}


# Build a reset string to pre-disable everything but the option[s] we
# care about.  This is somewhat useful in ensuring a deterministic
# minimal build excepting the requisite pieces.
sub mk_reset_str
{
	my $skip = shift;
	die "Bad coder!  Bad!" unless ref $skip eq 'HASH';
	my @notskip = grep { !defined($skip->{$_}) } keys %OPTS;
	return map { mkopts($_, 0) } sort @notskip;
}



sub one_build
{
	my ($opts, $sdn, $clopts) = @_;
	my %ret = (
		ok  => 0,
		sig => 0,
		stdstr => [],
		errstr => '',
		tstdir => '',
		bstr => mk_build_str($opts),
	);

	my $tstdir = $ret{tstdir} = "$tmpdir/@{[$sdn]}";
	mkdir $tstdir or die "mkdir($tstdir): $!";

	print "  $sdn: @{[join ' ', $ret{bstr}]}\n";


	# Prep build
	my @cmopts;
	push @cmopts, mk_reset_str($opts);
	push @cmopts, mk_build_strs($opts);
	my @cmd = ('cmake', @cmopts, $mypath);
	my ($stdout, $stderr);
	push @{$ret{stdstr}}, "@{[join ' ', @cmd]}" if $clopts->{verbose};
	chdir $tstdir;
	$? = 0 if $clopts->{dryrun}; # So it's known
	run3 \@cmd, undef, \$stdout, \$stderr unless $clopts->{dryrun};
	chdir $ORIGDIR;
	if($? & 127)
	{
		$ret{sig} = $? & 127;
		$ret{errstr} = "cmake died from signal, giving up...\n";
		return \%ret;
	}
	if($? >> 8)
	{
		# Er, cmake failed..
		$ret{errstr} = "cmake failed!\n---\n$stdout\n---\n$stderr\n---\n";
		return \%ret;
	}

	# And kick it off
	@cmd = ('make', '-C', $tstdir, 'ctwm');

	$stdout = $stderr = undef;
	push @{$ret{stdstr}}, "@{[join ' ', @cmd]}" if $clopts->{verbose};
	run3 \@cmd, undef, \$stdout, \$stderr unless $clopts->{dryrun};
	if($? & 127)
	{
		$ret{sig} = $? & 127;
		$ret{errstr} = "make died from signal, giving up...\n";
		return \%ret;
	}
	if($? >> 8)
	{
		# Build failed  :(
		$ret{errstr} = "make failed!\n---\n$stdout\n---\n$stderr\n---\n";
		return \%ret;
	}

	# OK
	$ret{ok} = 1;
	push @{$ret{stdstr}}, "OK.";
	return \%ret;
}
