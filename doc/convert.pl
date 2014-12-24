#!/usr/bin/env perl
use strict;
use warnings;

# Usage: $this.pl < old.roff.ctwm.man > new.asciidoc.ctwm.txt

die "This script has served out its purpose.  For reference only.\n";

#
# This does the majority of the conversion from the existing
# part-pure-roff, part-man, part-ms, part-god-knows-what manpage, to an
# asciidoc source file.  It actually does a pretty good job; easily 95%
# of the work.
#
# There will be a fair bit of minor manual cleanup after it's run, but
# only a few isolated more major reworkings, of places it wasn't worth
# trying to extend this to properly handle itself.
#
# Currently known:
# - The sub-listing under WorkSpaces
# - The whole bindings/title buttons block.  This probably wants to be
#   organized slightly differently anyway.
# - Fixup font string in WorkSpaceFont
#


# Slurp
MAINLOOP: while(<STDIN>)
{
	# Custom .EX and .EE macros for defining examples
	if(/^\.EX/)
	{
		print proc_ex();

		# Back around to the top
		next MAINLOOP;
	}


	# Now std man macros
	# section headers -> lev1 headers
	s,^\.SH "?([^"]+)"?,\n== $1,;

	# Paragraphs are easy, they're just blank lines
	s,^.PP$,,;

	# .de defines macros, so we ignore everything until the end of the
	# definition
	if(/^\.de /)
	{
		1 while(<STDIN> !~ /^\.\./); # nada

		# And back around
		next;
	}

	# .ta sets tabstops.  Totally don't care.
	next if /^\.ta/;

	# .TH sets the title
	if(/^\.TH/)
	{
		# Hardcode
		print "= CTWM(1)\n\n";
		next;
	}


	# These .TP's are a little tougher.  They basically turn into
	# labelled lists, in asciidoc parlance.  .IP is another
	# almost-identical way of phrasing it in man (jeez), except
	# incompatible.  Naturally.
	if(/^\.(TP|IP )/)
	{
		# If it's .TP, the following line is the label.  If it's .IP, the
		# label is in the line, probably quotes, with possibly an indent
		# length hanging on the end.  Except when it's not; there are a
		# few cases of /^.IP$/, but I'm intentionally excluding them with
		# the space above.

		# Start with whitespace
		print "\n";

		my $lbl;
		if(/^\.TP/)
		{
			# Next line is the label
			chomp($lbl = <STDIN>);

			# Clean it up.  First translate away the .B altogether, and
			# the optional quotes.
			$lbl =~ s,^.B "?([^"]+)"?,$1,;

			# Do inline translation
			$lbl = ilcvt($lbl);
		}
		elsif(/^\.IP/)
		{
			chomp($lbl = $_);

			# Strip away the .IP leader, and the trailing indent number
			$lbl =~ s,^.IP "?(.+?)"? [48]$,$1,;

			# Some have \f[ont] stuff.  In fact, most have \fB'ing of the
			# label, which we don't want, but many have \fI'ing of args,
			# which we do.  Fortunately, I _think_ we can use spaces to
			# delimiate, because the \fB parts are always the first word.
			$lbl =~ s,\\fB([^\s]+)\\fP,$1,g;

			# The f.setpriority line does weird stuff.
			$lbl =~ s,\\\*,,g if $lbl =~ /^f\.setpriority/;

			# Do remaining inline conversion
			$lbl = ilcvt($lbl);

			# asciidoc doesn't get along with the "[++" -> "\[++"
			# conversion other things need here, so reverse it.
			$lbl =~ s,\\\[\+\+,[\+\+,g;
		}
		# Now we've got just the label itself ready to go.

		# Clean up any space
		$lbl =~ s,(^\s+|\s+$),,;

		print "${lbl}::\n";


		# Now we have lines of content.  Assume it ends as soon as we hit
		# another macro at the start.
		my $addnorm = 0;
		while(<STDIN>)
		{
			# Skip a comment and a weird (ms?) macro that show up before
			# the f.setpriority line.
			next if /^\\"/;
			next if /^\.ds/;

			# We get embedded .EX's sometimes
			if(/^\.EX/)
			{
				my $str = proc_ex();

				# Mess with newlines, and make continuations
				$str =~ s/(^\s+|\s+$)//g;
				$str = "+\n$str\n+\n";

				# Add [normal] before next para if there is one
				$addnorm = 1;

				# Output
				print $str;
				next;
			}

			# An embedded .IP means continue the indent in roff, which
			# means we just continue the list item here.
			if(/^\.IP$/)
			{
				print "+\n[normal]\n";
				$addnorm = 0;
				next;
			}

			# We consider this ended if we hit a macro line
			if(/^\./)
			{
				# If it's not another .TP/.IP, add a blank line
				print "\n" unless /^\.(TP|IP)/;
				redo MAINLOOP;
			}

			# Otherwise, do std inline processing, and output
			chomp;
			$_ = ilcvt($_);
			$_ = "  $_\n" if $_;
			if($_ && $addnorm)
			{
				$_ = "[normal]\n$_";
				$addnorm = 0;
			}
			print $_;
		}

		# NOTREACHED?
		next MAINLOOP;
	}


	# Do basic inline processing before outputting.
	$_ = ilcvt($_);
	print;
}


# Footer
print "\n\n\n// vi" . "m:ft=asciidoc:expandtab:\n";

exit;



# Basic conversion of inline stuff
sub ilcvt
{
	my ($l) = shift;

	# Font conversions
	# \fIsomething\fP makes it underlined.  This seems to be used a lot for
	# commands and filenames and the like, so decide to treat them as
	# monospace.
	# \fB makes bold.  This seems used for an awful lot, but often literal
	# text like config params etc.  That....  seems like it should generally
	# be monospace too, doesn't it?
	# Well, no, it also does stuff that should be bold, or emphasized.
	# But there's no way to tell from here.  And trying to match up the
	# \fP with the B or I it should go with is a losing battle, even if
	# they always matched, which they don't.  So punt.
	$l =~ s,\\f[IBP],++,g;

	# Convert various roff escapes
	$l =~ s,\\\(oq,``,g;
	$l =~ s,\\\(cq,'',g;
	$l =~ s,\\\(or,|,g;

	# Deal with a couple squirrely things asciidoc tries to take as magic
	$l =~ s,\[\+,\\\[+,g;
	$l =~ s,\(\(,\\((,g;

	# Turn all these backslashed hyphens into plain
	$l =~ s,\\-,-,g;

	# This happens a couple times, but asciidoc only cares once?
	#$l =~ s,(\+\+ctwm\+\+)'s,$1\\'s,g;

	# Convert comments
	$l =~ s,^\.\\",// ,;

	# Apparently, there's no way to escape hashes in asciidoc EXCEPT like
	# this?  How stupid.
	$l =~ s,#,&#35;,g;


	return $l;
}


# Process a .EX/.EE block
sub proc_ex
{
	# Assume we've already read the .EX
	my $r;

	# Replace with start of an example block
	$r .= "\n------\n";

	# Read lines and output until the end
	while((my $ln = <STDIN>) !~ /^\.EE/)
	{
		# Get rid of \fWhatever manipulations
		$ln =~ s,\\f[IBP],,g;

		# Dump escaping of -'s
		$ln =~ s,\\-,-,g;

		$r .= $ln;
	};

	# And close
	$r .= "------\n\n";


	# Done
	return $r;
}
