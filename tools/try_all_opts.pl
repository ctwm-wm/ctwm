#!/usr/bin/env perl
use strict;
use warnings;

use File::Temp qw//;
use File::Spec qw//;
use File::Basename qw(dirname);
use File::Path qw(remove_tree);
use Cwd qw(abs_path getcwd);
use IPC::Run3;

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
use Getopt::Long qw(:config no_ignore_case bundling);
my %opts;
my @opts = (
	'include|I=s@',  # Extra include path(s)
	'keep|k',        # Keep output dir
	'verbose|v',     # Verbosity
	'jobs|j=i',      # -j to pass to make
	'dryrun|d',      # Don't exec anything
);
GetOptions(\%opts, @opts);

# Extra include dirs given?
push @INCDIRS, @{$opts{include}} if $opts{include};


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
print "Checking over options...\n" if $opts{verbose};
my @use;
OCHECK: for my $k (@DO_OPTS)
{
	my %o = %{$OPTS{$k}};

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
			print "  $k: $e not found in path, skipping.\n" if $opts{verbose};
		}
		else
		{
			print "  $k: $e OK, adding.\n" if $opts{verbose};
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
				print"  $k: $i found in $d, adding.\n" if $opts{verbose};
				push @use, $k;
				next OCHECK;
			}
		}

		print "  $k: $i not found, skipping.\n" if $opts{verbose};
		next OCHECK;
	}

	# Everything else has no requirements we bother to check
	print "  $k: OK\n" if $opts{verbose};
	push @use, $k;
}

# Which aren't we using?
my @skip = sort grep { my $x = $_; !grep { $_ eq $x } @use } keys %OPTS;


# Summarize
print "Skipping options: " . join(' ', @skip) . "\n" if @skip;
print "Using options: " . join(' ', @use) . "\n" if @use;
die "No usable options!\n" unless @use;
print "Building from $mypath\n";


# Generate powerset
my @builds;
my $reset;
{
	# Make a cmake-y option definition
	my $mkopts = sub { return "-D$_[0]=@{[$_[1] ? 'ON ' : 'OFF']}"; };

	my $dbgshift = 2;
	my $_dbgret = sub {
		printf("%*s%s\n", $dbgshift, "", $_) for ("Rets:", @_);
	};

	# Build all subsets.  Each invocation returns a string of cmake -D's.
	my $bss;
	$bss = sub {
		$dbgshift++;

		#print "  bss(" . join(" ", @_) . ")\n";
		return () if @_ == 0;
		my $base = shift @_;
		my @base = ($mkopts->($base, 0), $mkopts->($base, 1));
		#$_dbgret->(@base) if @_ == 0;
		return @base if @_ == 0;

		my @subsubs = $bss->(@_);
		$dbgshift--;
		my @rets;
		for my $b (@base)
		{
			push @rets, "$b $_" for @subsubs;
		}
		#$_dbgret->(@rets);
		return @rets;
	};

	@builds = $bss->(@use);

	# Also build a reset string to pre-disable everything.  This is
	# useful when for some reason, one or more options can't be built, to
	# make sure they're turned off.
	$reset = join " ", map { $mkopts->($_) } @skip;
}

print("Builds: @{[scalar @builds]}\n",
		(map "  $_\n", @builds),
		"\n") if $opts{verbose};


# Create a tempdir
my $tmpbase = $ENV{TMPDIR} // "/tmp";
my $tmpdir = File::Temp->newdir("ctwm-opts-XXXXXXXX",
		DIR => $tmpbase, CLEANUP => !$opts{keep});
print "Testing in $tmpdir...\n";

my $sdn = 0;
my ($KEEPDIR, $tstdir);
my ($suc, $fail) = (0,0);
my @fails;
DOBUILDS: for my $bo (@builds)
{
	my $bret = one_build($bo, ++$sdn);

	if($bret->{ok})
	{
		# Succeeded; print success and clean up unless we're --keep'ing
		print $bret->{stdstr};
		$suc++;
		remove_tree($bret->{tstdir}) unless $opts{keep};
	}
	else
	{
		# Failed; print failures and mark things to not be cleaned up.
		print $bret->{stdstr};
		print $bret->{errstr};
		$fail++;
		$tmpdir->unlink_on_destroy(0) if $opts{keep};
	}
}


print "\n\n$suc succeeeded, $fail failed.\n";
if(@fails)
{
	print " Failed option combinations:\n";
	print "  $_\n" for @fails;
	print "\nBuild artifacts left in $tmpdir\n";
	exit 1;
}
exit 0;



sub one_build
{
	my ($opts, $sdn) = @_;
	my %ret = (
		ok  => 0,
		sig => 0,
		stdstr => '',
		errstr => '',
		txtdir => '',
	);

	$ret{tstdir} = $tstdir = "$tmpdir/@{[$sdn]}";
	$ret{stdstr} .= "  $sdn: $opts\n";
	mkdir $tstdir or die "mkdir($tstdir): $!";


	# Prep build
	my @cmd = ('cmake', split(/ /, $reset), split(/ /, $opts), $mypath);
	my ($stdout, $stderr);
	$ret{stdstr} .= "    @{[join ' ', @cmd]}\n" if $opts{verbose};
	chdir $tstdir;
	run3 \@cmd, undef, \$stdout, \$stderr unless $opts{dryrun};
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
		$ret{errstr} = "    cmake failed!\n---\n$stdout\n---\n$stderr\n---\n";
		return \%ret;
	}

	# And kick it off
	@cmd = ('make', '-C', $tstdir);
	push @cmd, "-j$opts{jobs}" if $opts{jobs};
	push @cmd, 'ctwm';

	$stdout = $stderr = undef;
	$ret{stdstr} .= "    @{[join ' ', @cmd]}\n" if $opts{verbose};
	run3 \@cmd, undef, \$stdout, \$stderr unless $opts{dryrun};
	if($? & 127)
	{
		$ret{sig} = $? & 127;
		$ret{errstr} = "make died from signal, giving up...\n";
		return \%ret;
	}
	if($? >> 8)
	{
		# Build failed  :(
		$ret{errstr} = "    make failed!\n---\n$stdout\n---\n$stderr\n---\n";
		return \%ret;
	}

	# OK
	$ret{ok} = 1;
	$ret{stdstr} .= "    OK.\n";
	return \%ret;
}
