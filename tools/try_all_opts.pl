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

# Build from the tree we're running from
my @mypath = File::Spec->splitdir(abs_path(dirname($0)));
while(@mypath && ! -r "@{[File::Spec->catfile(@mypath, 'ctwm.h')]}")
{
	pop @mypath;
}
die "Didn't find ctwm.h!" unless @mypath;
my $mypath = File::Spec->catdir(@mypath);


# Parse command line args
my %CLOPTS = parse_clargs();

# Default and adjust globals
$CLOPTS{jobs} //= 1;
push @INCDIRS, @{$CLOPTS{include}} if $CLOPTS{include};


# Run the subset given on the command line, or everything.
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



# Check which options we think can build on this system.
print "Checking over options...\n" if $CLOPTS{verbose};
my @use = check_opts(@DO_OPTS);

# Which aren't we using?
my @skip = sort grep { my $x = $_; !grep { $_ eq $x } @use } keys %OPTS;


# Summarize
print "Skipping options: " . join(' ', @skip) . "\n" if @skip;
print "Using options: " . join(' ', @use) . "\n" if @use;
die "No usable options!\n" unless @use;
print "Building from $mypath\n";



# Get the built-out list of configurations to try.
my @builds = mk_build_option_matrix();
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


# Now, actually running them.
my ($suc, $fail) = (0,0);
my @fails;
my @fullres;

my $fm = Parallel::ForkManager->new($CLOPTS{jobs}, $tmpdir);
$fm->run_on_finish(sub { return one_build_finish(@_); });

my $sdn = 0;
$fm->start_child(++$sdn, sub { return do_one_build($_, $sdn, \%CLOPTS); })
		for @builds;
$fm->wait_all_children();


# Summarize results.
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
		'help|h',        # Show help and exit
	);
	my %opts;
	GetOptions(\%opts, @clopts);

	# Help output
	if($opts{help})
	{
		print <<"_EOF_";
$0 [--options] [BUILD_FLAGS]

  If BUILD_FLAGS is given, the given flags will be the ones tried in the
  appropriate combinations.  With no BUILD_FLAGS, all configured options
  will be tried.

  Options taking args:
    --include  -I  Extra include dirs to search for guessing which options
                     can be tried on this system.
    --jobs     -j  Number of parallel jobs to run.

  Flags:
    --all      -a  Try all combinations of options on/off, rather than all
                     options individually on/off.
    --dryrun   -d  Don't actually do builds, just show what would be done.
    --keep     -k  Keep temp dir around.
    --verbose  -v  More verbose output.
    --help     -h  This message.

_EOF_
		exit(0);
	}

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


# Work up the list of what build option configs we want to try.
sub mk_build_option_matrix
{
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

	return @builds;
}


# Process the results of a single build (inside P:FM's world)
sub one_build_finish
{
	my ($pid, $excode, $ident, $exsig, $coredump, $bret) = @_;
	unless(defined $bret && ref $bret eq 'HASH')
	{
		warn "Child $ident didn't return anything!";
		$fail++;
		push @fails, "(unknown: $ident)";
		return;
	}

	# Stash up full output
	$fullres[$ident] = $bret;

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
}


# Do a single build (inside a P::FM child)
sub do_one_build
{
	my ($opts, $sdn, $clopts) = @_;
	my ($stdout, $stderr);
	my %ret = (
		# Summary/state bits
		ok  => 0,
		sig => 0,
		tstdir => '',
		bstr => mk_build_str($opts),

		# Normal success messages
		stdstr => [],

		# Error message in string form
		errstr => '',

		# Structured info about steps
		detail => {
			cmake => {
				ok     => 0,
				cmd    => [],
				stdout => '',
				stderr => '',
			},
			make => {
				ok     => 0,
				stdout => '',
				stderr => '',
			},
		},
	);

	my $tstdir = $ret{tstdir} = "$tmpdir/@{[$sdn]}";
	mkdir $tstdir or die "mkdir($tstdir): $!";

	print "  $sdn: @{[join ' ', $ret{bstr}]}\n";


	# Run the cmake step
	my @cmopts;
	push @cmopts, mk_reset_str($opts);
	push @cmopts, mk_build_strs($opts);
	my @cmd = ('cmake', @cmopts, $mypath);
	push @{$ret{stdstr}}, "@{[join ' ', @cmd]}" if $clopts->{verbose};
	$ret{detail}{cmake}{cmd} = [@cmd];

	# Have to chdir for cmake; make can just use -C
	my $origdir = getcwd();
	chdir $tstdir;
	$? = 0; # Be sure it's clean for dryrun case

	$stdout = \$ret{detail}{cmake}{stdout};
	$stderr = \$ret{detail}{cmake}{stderr};
	run3 \@cmd, undef, $stdout, $stderr unless $clopts->{dryrun};

	chdir $origdir;

	# Died from a signal?  Stop right away.
	if($? & 127)
	{
		$ret{sig} = $? & 127;
		$ret{errstr} = "cmake died from signal, giving up...\n";
		return \%ret;
	}

	# Failed in some way?
	if($? >> 8)
	{
		$ret{errstr} = "cmake failed!\n---\n$$stdout\n---\n$$stderr\n---\n";
		return \%ret;
	}

	# OK!
	$ret{detail}{cmake}{ok} = 1;


	# OK, cmake step worked.  Now try the build.
	@cmd = ('make', '-C', $tstdir, 'ctwm');

	push @{$ret{stdstr}}, "@{[join ' ', @cmd]}" if $clopts->{verbose};
	$? = 0; # Be sure it's clean for dryrun case

	$stdout = \$ret{detail}{make}{stdout};
	$stderr = \$ret{detail}{make}{stderr};
	run3 \@cmd, undef, $stdout, $stderr unless $clopts->{dryrun};

	# Signal?
	if($? & 127)
	{
		$ret{sig} = $? & 127;
		$ret{errstr} = "make died from signal, giving up...\n";
		return \%ret;
	}

	# Fail?
	if($? >> 8)
	{
		$ret{errstr} = "make failed!\n---\n$$stdout\n---\n$$stderr\n---\n";
		return \%ret;
	}

	# OK!
	$ret{detail}{make}{ok} = 1;


	# If we get here, the build completed fine!
	$ret{ok} = 1;
	push @{$ret{stdstr}}, "OK.";
	return \%ret;
}
