#
# Handle stuff related to building the manual in various ways
#


# It's written in asciidoc, so first see if we can find that.  If we can,
# we'll use it to [re]build the manual.  If not, we may have pregen'd
# versions we can use.  If neither, well, the user's in trouble...
find_program(ASCIIDOC asciidoc)
find_program(A2X a2x)


#
# Setup some vars for the various steps in the process
#

# The original source
set(ADOC_SRC ${CMAKE_CURRENT_SOURCE_DIR}/doc/ctwm.1.adoc)

# Where we build stuff
set(MANPAGE_TMPDIR ${CMAKE_CURRENT_BINARY_DIR}/mantmp)

# Where we copy the source to during rewrites, and then do the actual
# build from.
set(ADOC_TMPSRC ${MANPAGE_TMPDIR}/ctwm.1.adoc)

# Where the end products wind up
set(MANPAGE ${CMAKE_CURRENT_BINARY_DIR}/ctwm.1)
set(MANHTML ${CMAKE_CURRENT_BINARY_DIR}/ctwm.1.html)

# Flags for what we have/what we'll build
set(HAS_MAN 0)
set(HAS_HTML 0)
set(CAN_BUILD_MAN 0)


#
# The build process
#

# If we have asciidoc, use it
if(ASCIIDOC AND A2X)
	set(HAS_MAN 1)
	set(CAN_BUILD_MAN 1)

	message(STATUS "Found asciidoc (${A2X}) for building manpage")

	# Setup a temp dir under the build for our processing
	file(MAKE_DIRECTORY ${MANPAGE_TMPDIR})

	# We hop through a temporary file to process in definitions for e.g.
	# $ETCDIR.  Can't do that here though since the directories aren't
	# defined yet, so we write the target for $ADOC_TMPSRC lower down.

	# We have to jump through a few hoops here, because a2x gives us no
	# control whatsoever over where the output file goes or what it's
	# named.  Thanks, guys.  So since $ADOC_TMPSRC is in $MANPAGE_TMPDIR,
	# the build output will also be there.  Copy it into place
	set(MANPAGE_TMP ${MANPAGE_TMPDIR}/ctwm.1)
	add_custom_command(OUTPUT ${MANPAGE}
		DEPENDS ${ADOC_TMPSRC}
		COMMAND ${A2X} --doctype manpage --format manpage ${ADOC_TMPSRC}
		COMMAND cp ${MANPAGE_TMP} ${MANPAGE}
		COMMENT "Processing ${ADOC_TMPSRC} -> ${MANPAGE}"
	)

	# Should do HTML at some point, but asciidoc is so slow (~5 secs) I'm
	# not bothering.  When we switch to asciidoctor (~.2 secs), I'll
	# revisit.

else()
	# No ability to generate it ourselves.  See if we have prebuilt.
	find_file(MAN_PRESRC ctwm.1
		PATHS ${CMAKE_CURRENT_SOURCE_DIR}/doc
		NO_DEFAULT_PATH)

	if(MAN_PRESRC)
		# Got it, so use that
		set(HAS_MAN 1)
		message(STATUS "No asciidoc/a2x found, using prebuilt manpage.")
		# Later target handles the rewrites to $MAN_PRESRC -> $MANPAGE
	else()
		# Ruh ro
		message(WARNING "Can't find asciidoc/a2x, and no prebuilt manpage available.")
	endif(MAN_PRESRC)

	# Install prebuilt HTML if we have it generated too.
	find_file(HTML_PRESRC ctwm.1.html
		PATHS ${CMAKE_CURRENT_SOURCE_DIR}/doc
		NO_DEFAULT_PATH)

	if(HTML_PRESRC)
		set(HAS_HTML 1)
		message(STATUS "Installing prebuilt HTML manual.")
		# Later target does rewrite/move
	endif(HTML_PRESRC)
endif(ASCIIDOC AND A2X)
